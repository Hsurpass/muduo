// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include "muduo/base/noncopyable.h"
#include "muduo/base/StringPiece.h"
#include "muduo/base/Types.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/InetAddress.h"

#include <memory>

#include <boost/any.hpp>

/*
  1.当连接到来，创建一个TcpConnection对象，立刻用shared_ptr来管理，引用计数为1
  在Channel中维护一个weak_ptr(tie_),将这个shared_ptr对象赋值给tie_，引用计数仍为1
  2.当连接关闭，在handleEvent,将tie_提升得到一个shared_ptr对象，引用计数就变成了2
*/

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo
{
  namespace net
  {

    class Channel;
    class EventLoop;
    class Socket;

    ///
    /// TCP connection, for both client and server usage.
    ///
    /// This is an interface class, so don't expose too much details.
    class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
    {
    public:
      /// Constructs a TcpConnection with a connected sockfd
      ///
      /// User should not create this object.
      TcpConnection(EventLoop *loop,
                    const string &name,
                    int sockfd,
                    const InetAddress &localAddr,
                    const InetAddress &peerAddr);
      ~TcpConnection();

      EventLoop *getLoop() const { return loop_; }
      const string &name() const { return name_; }
      const InetAddress &localAddress() const { return localAddr_; }
      const InetAddress &peerAddress() const { return peerAddr_; }
      bool connected() const { return state_ == kConnected; }
      bool disconnected() const { return state_ == kDisconnected; }
      // return true if success.
      bool getTcpInfo(struct tcp_info *) const;
      string getTcpInfoString() const;

      // void send(string&& message); // C++11
      void send(const void *message, int len);
      void send(const StringPiece &message);
      // void send(Buffer&& message); // C++11
      void send(Buffer *message); // this one will swap data
      
      void shutdown();            // NOT thread safe, no simultaneous calling
      // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
      void forceClose();
      void forceCloseWithDelay(double seconds);
      void setTcpNoDelay(bool on);
      
      // reading or not
      void startRead();
      void stopRead();
      bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop

      void setContext(const boost::any &context) { context_ = context; }
      const boost::any &getContext() const { return context_; }
      boost::any *getMutableContext() { return &context_; }

      void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
      void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
      void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
      void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
      {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
      }

      /// Advanced interface
      Buffer *inputBuffer() { return &inputBuffer_; }
      Buffer *outputBuffer() { return &outputBuffer_; }

      /// Internal use only.
      void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

      // called when TcpServer accepts a new connection
      void connectEstablished(); // should be called only once
      // called when TcpServer has removed me from its map
      void connectDestroyed(); // should be called only once

    private:
      enum StateE
      {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
      };
      void handleRead(Timestamp receiveTime);
      void handleWrite();
      void handleClose();
      void handleError();
      // void sendInLoop(string&& message);
      void sendInLoop(const StringPiece &message);
      void sendInLoop(const void *message, size_t len);
      void shutdownInLoop();
      // void shutdownAndForceCloseInLoop(double seconds);
      void forceCloseInLoop();
      void setState(StateE s) { state_ = s; }
      const char *stateToString() const;
      void startReadInLoop();
      void stopReadInLoop();

      EventLoop *loop_;
      const string name_;
      StateE state_; // FIXME: use atomic variable
      bool reading_;
      // we don't expose those classes to client.
      std::unique_ptr<Socket> socket_;
      std::unique_ptr<Channel> channel_;
      const InetAddress localAddr_;
      const InetAddress peerAddr_;

      ConnectionCallback connectionCallback_;
      MessageCallback messageCallback_;
      /*
        大流量场景：
        不断生成数据，然后发送conn->send();
        如果对等方接收不及时, 受到滑动窗口的控制，内核发送缓冲区不足；
        这个时候，就会将用户数据添加到应用层发送缓冲区(outputbuffer);可能会撑爆outputbuffer。
        
        解决方法就是：调整发送频率，关注WriteCompleteCallback，所有的数据都发送完，WriteCompleteCallback回调，然后继续发送。
        低流量:
          通常不需要关注

        数据发送完毕回调函数，即所有的用户数据都已拷贝到内核缓冲区时回调该函数
        outputbuffer被清空也会回调该函数，可以理解为低水位标回调函数
      */
      WriteCompleteCallback writeCompleteCallback_;// 低水位标回调
      HighWaterMarkCallback highWaterMarkCallback_;// 高水位标回调，超过高水位标时，为了防止内存被撑爆，可以断开连接
      CloseCallback closeCallback_;
      
      size_t highWaterMark_;  // 高水位标
      Buffer inputBuffer_;    // 应用层接收缓冲区
      Buffer outputBuffer_; // FIXME: use list<Buffer> as output buffer.  // 应用层发送缓冲区
      /*
        可变类型解决方案:
          void*: 这种方法不是类型安全的
          boost::any: 任意类型的类型安全存储以及安全的取回
                      在标准库容器中存放不同类型的方法，比如说vector<boost::any>
      */
      boost::any context_;  // 绑定一个未知类型的上下文对象
      // FIXME: creationTime_, lastReceiveTime_
      //        bytesReceived_, bytesSent_
    };

    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

  } // namespace net
} // namespace muduo

#endif // MUDUO_NET_TCPCONNECTION_H
