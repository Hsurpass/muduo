// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <atomic>
#include <functional>
#include <vector>

#include <boost/any.hpp>

#include "muduo/base/Mutex.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/TimerId.h"

/*
系统启动时，都打开了哪些文件描述符：
0,1,2
3: pollerFd
4: timerfd
5: eventfd
6: listenfd
7: idlefd
*/

namespace muduo
{
    namespace net
    {

        class Channel;
        class Poller;
        class TimerQueue;

        ///
        /// Reactor, at most one per thread.  每个线程最多有一个EVENTLOOP
        ///
        /// This is an interface class, so don't expose too much details.
        class EventLoop : noncopyable
        {
        public:
            typedef std::function<void()> Functor;

            EventLoop();
            ~EventLoop(); // force out-line dtor, for std::unique_ptr members.

            ///
            /// Loops forever.
            ///
            /// Must be called in the same thread as creation of the object.
            ///
            void loop();

            /// Quits loop.
            ///
            /// This is not 100% thread safe, if you call through a raw pointer,
            /// better to call through shared_ptr<EventLoop> for 100% safety.
            void quit();

            ///
            /// Time when poll returns, usually means data arrival.
            ///
            Timestamp pollReturnTime() const { return pollReturnTime_; }

            int64_t iteration() const { return iteration_; }

            /// Runs callback immediately in the loop thread.
            /// It wakes up the loop, and run the cb.
            /// If in the same loop thread, cb is run within the function.
            /// Safe to call from other threads.
            void runInLoop(Functor cb);
            /// Queues callback in the loop thread.
            /// Runs after finish pooling.
            /// Safe to call from other threads.
            void queueInLoop(Functor cb);

            size_t queueSize() const;

            // timers

            ///
            /// Runs callback at 'time'.
            /// Safe to call from other threads.
            ///
            TimerId runAt(Timestamp time, TimerCallback cb);
            ///
            /// Runs callback after @c delay seconds.
            /// Safe to call from other threads.
            ///
            TimerId runAfter(double delay, TimerCallback cb);
            ///
            /// Runs callback every @c interval seconds.
            /// Safe to call from other threads.
            ///
            TimerId runEvery(double interval, TimerCallback cb);
            ///
            /// Cancels the timer.
            /// Safe to call from other threads.
            ///
            void cancel(TimerId timerId);

            // internal usage
            void wakeup();
            void updateChannel(Channel *channel); // 在Poller中添加或者更新通道
            void removeChannel(Channel *channel); // 从Poller中移除通道
            bool hasChannel(Channel *channel);

            // pid_t threadId() const { return threadId_; }
            void assertInLoopThread()
            {
                if (!isInLoopThread())
                {
                    abortNotInLoopThread();
                }
            }
            bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
            // bool callingPendingFunctors() const { return callingPendingFunctors_; }
            bool eventHandling() const { return eventHandling_; }

            void setContext(const boost::any &context)
            {
                context_ = context;
            }

            const boost::any &getContext() const
            {
                return context_;
            }

            boost::any *getMutableContext()
            {
                return &context_;
            }

            static EventLoop *getEventLoopOfCurrentThread();

        private:
            void abortNotInLoopThread();
            void handleRead(); // waked up
            void doPendingFunctors();

            void printActiveChannels() const; // DEBUG

            typedef std::vector<Channel *> ChannelList;

            bool looping_; /* atomic */       // 是否处于循环状态
            std::atomic<bool> quit_;          // 是否处于退出状态
            bool eventHandling_; /* atomic */ // 事件处理状态
            bool callingPendingFunctors_;     /* atomic */
            int64_t iteration_;
            const pid_t threadId_;     // 当前对象所属线程id
            Timestamp pollReturnTime_; // 调用poll函数返回的时间戳
            std::unique_ptr<Poller> poller_;
            std::unique_ptr<TimerQueue> timerQueue_;
            int wakeupFd_;
            // unlike in TimerQueue, which is an internal class,
            // we don't expose Channel to client.
            std::unique_ptr<Channel> wakeupChannel_;
            boost::any context_;

            // scratch variables
            ChannelList activeChannels_;    // Poller返回的活动通道
            Channel *currentActiveChannel_; // 当前正在处理的活动通道

            mutable MutexLock mutex_;
            std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_);
        };

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_EVENTLOOP_H
