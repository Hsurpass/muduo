// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "muduo/net/Connector.h"

#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/SocketsOps.h"

#include <errno.h>

using namespace muduo;
using namespace muduo::net;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
  : loop_(loop),
    serverAddr_(serverAddr),
    connect_(false),
    state_(kDisconnected),
    retryDelayMs_(kInitRetryDelayMs)  // 初始值0.5秒
{
  LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
  LOG_DEBUG << "dtor[" << this << "]";
  assert(!channel_);
}

// 可以跨线程调用
void Connector::start()
{
  connect_ = true;
  loop_->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

void Connector::startInLoop()
{
  loop_->assertInLoopThread();
  assert(state_ == kDisconnected);
  if (connect_)
  {
    connect();
  }
  else
  {
    LOG_DEBUG << "do not connect";
  }
}

// 可能被其他线程调用
void Connector::stop()
{
  connect_ = false;
  loop_->queueInLoop(std::bind(&Connector::stopInLoop, this)); // FIXME: unsafe
  // FIXME: cancel timer
}

void Connector::stopInLoop()
{
  loop_->assertInLoopThread();
  if (state_ == kConnecting)
  {
    setState(kDisconnected);
    int sockfd = removeAndResetChannel();
    retry(sockfd);  // 这里并非要重连，只是调用sockets::close(sockfd)，因为connect_被置为false了。
  }
}

void Connector::connect()
{
  int sockfd = sockets::createNonblockingOrDie(serverAddr_.family()); // 创建非阻塞socket
  int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno)
  {
    case 0:
    case EINPROGRESS:   // 表示正在连接
    case EINTR:
    case EISCONN:     // 连接成功
      connecting(sockfd);
      break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      retry(sockfd);    // 重连
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
      sockets::close(sockfd); // 不能重连, 关闭sockfd
      break;

    default:
      LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
      sockets::close(sockfd);
      // connectErrorCallback_();
      break;
  }
}

// 不能跨线程调用
void Connector::restart()
{
  loop_->assertInLoopThread();
  setState(kDisconnected);
  retryDelayMs_ = kInitRetryDelayMs;  // 重置重连时间
  connect_ = true;
  startInLoop();
}

void Connector::connecting(int sockfd)
{
  setState(kConnecting);
  assert(!channel_);
  channel_.reset(new Channel(loop_, sockfd)); // 重置channel_
  channel_->setWriteCallback(std::bind(&Connector::handleWrite, this)); // FIXME: unsafe
  channel_->setErrorCallback(std::bind(&Connector::handleError, this)); // FIXME: unsafe

  // channel_->tie(shared_from_this()); is not working,
  // as channel_ is not managed by shared_ptr
  channel_->enableWriting();  // 关注可写事件
}

// 该函数可能是其他线程调用的，resetChannel如果不放在当前loop线程执行，在其他线程可能被置空，然后当前线程执行removeAndResetChannel就会崩溃
// 就是说resetChannel不放在loop线程中执行不是线程安全的。
int Connector::removeAndResetChannel()
{
  channel_->disableAll(); // 移除所有事件
  channel_->remove(); // 从poller中移除fd
  int sockfd = channel_->fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  // 原因1.不能在这里重置channel_，因为可能正在调用channel::handleEvent-->Connector::handleWrite, 所以就加入到loop_这个队列中, 在下一轮重置。
  // 原因2.stop --> stopInLoop 这个就属于在当前IO线程中执行pendingFunctors_时，Functor函数中又调用了queueInLoop 的情况。
  loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // FIXME: unsafe
  return sockfd;
}

void Connector::resetChannel()
{
  channel_.reset(); // 置空
}

void Connector::handleWrite()
{
  LOG_TRACE << "Connector::handleWrite " << state_;

  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel(); // 触发了可写事件，要么是连接成功了，要么是发生了错误；从poller中移除，并将channel置空，停止监听写事件并停止监听fd(因为可写事件会一直触发)。
    // socket可写并不意味着连接一定成功
    // 还需要用getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen)再次确认一下
    int err = sockets::getSocketError(sockfd);
    if (err)  // 有错误
    {
      LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err << " " << strerror_tl(err);
      retry(sockfd);  // 重连
    }
    else if (sockets::isSelfConnect(sockfd))  // 自连接
    {
      LOG_WARN << "Connector::handleWrite - Self connect";
      retry(sockfd);  // 重连
    }
    else  // 连接成功，则调用cb
    {
      setState(kConnected);
      if (connect_)
      {
        newConnectionCallback_(sockfd);
      }
      else
      {
        sockets::close(sockfd);
      }
    }
  }
  else
  {
    // what happened?
    assert(state_ == kDisconnected);
  }
}

void Connector::handleError()
{
  LOG_ERROR << "Connector::handleError state=" << state_;
  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
    retry(sockfd);
  }
}

// 采用back-off策略重连,即重连时间逐渐延长, 0.5s, 1s, 2s, ...直至30秒
void Connector::retry(int sockfd)
{
  sockets::close(sockfd); // 关闭现有的fd
  setState(kDisconnected);// 设置为未连接
  if (connect_)
  {
    LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
             << " in " << retryDelayMs_ << " milliseconds. ";
    
    // 添加一个单次定时任务，返回错误重连，直到达到最大重连时间。
    loop_->runAfter(retryDelayMs_/1000.0, std::bind(&Connector::startInLoop, shared_from_this()) );
    retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
  }
  else
  {
    LOG_DEBUG << "do not connect";
  }
}

