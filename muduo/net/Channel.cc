// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"

#include <sstream>

#include <poll.h>

using namespace muduo;
using namespace muduo::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop *loop, int fd__)
    : loop_(loop),
      fd_(fd__),
      events_(0),
      revents_(0),
      index_(-1),
      logHup_(true),
      tied_(false),
      eventHandling_(false),
      addedToLoop_(false)
{
}

Channel::~Channel()
{
  assert(!eventHandling_);
  assert(!addedToLoop_);
  if (loop_->isInLoopThread())
  {
    assert(!loop_->hasChannel(this));
  }
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
  tie_ = obj;
  tied_ = true;
}

void Channel::update()
{
  addedToLoop_ = true;
  loop_->updateChannel(this);
}

// 调用这个函数之前确保调用disableAll
void Channel::remove()
{
  assert(isNoneEvent());
  addedToLoop_ = false;
  loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
  std::shared_ptr<void> guard;

  if (tied_)
  {
    guard = tie_.lock();  // use_count == 2
    if (guard)
    {
      handleEventWithGuard(receiveTime);
    } 
  }//use_count == 1
  else
  {
    handleEventWithGuard(receiveTime);
  }
}

/*
  客户端主动close，服务端收到POLLIN事件
  服务端主动close, 客户端read==0，然后close, 服务端收到POLLIN&POLLHUP事件
*/
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
  eventHandling_ = true;
  LOG_TRACE << reventsToString();
  
  // if(revents_ & POLLIN)
  //   LOG_DEBUG << "POLLINPOLLINPOLLINPOLLINPOLLIN";
  // if(revents_ & POLLHUP)
  //   LOG_DEBUG << "POLLHUPPOLLHUPPOLLHUPPOLLHUP";

  // 这个判断的意思是：当有POLLHUP事件的时候一定有POLLIN事件被触发时，不可能只存在POLLHUP，而没有POLLIN
  // 
  // POLLHUP: Hung up, output only
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
  {
    if (logHup_)
    {
      LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
    }
    if (closeCallback_)
      closeCallback_();
  }

  /* Invalid polling request.  */
  if (revents_ & POLLNVAL)   //POLLNVAL:文件描述符没有打开或者不是一个合法的fd
  {
    LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
  }

  if (revents_ & (POLLERR | POLLNVAL))
  {
    if (errorCallback_)
      errorCallback_();
  }

  /*
  POLLRDHUP:对端关闭或者处于半连接状态
  Stream socket peer closed connection, or shut down writing
              half of connection.  (This flag is especially useful for
              writing simple code to detect peer shutdown when using
              edge-triggered monitoring.)*/
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
  {
    if (readCallback_)
      readCallback_(receiveTime);
  }

  if (revents_ & POLLOUT)
  {
    if (writeCallback_)
      writeCallback_();
  }
  
  eventHandling_ = false;
}

string Channel::reventsToString() const
{
  return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const
{
  return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev)
{
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN)
    oss << "IN ";
  if (ev & POLLPRI)
    oss << "PRI ";
  if (ev & POLLOUT)
    oss << "OUT ";
  if (ev & POLLHUP)
    oss << "HUP ";
  if (ev & POLLRDHUP)
    oss << "RDHUP ";
  if (ev & POLLERR)
    oss << "ERR ";
  if (ev & POLLNVAL)
    oss << "NVAL ";

  return oss.str();
}
