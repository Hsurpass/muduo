// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include "muduo/base/Mutex.h"
#include "muduo/base/Timestamp.h"
#include "muduo/net/Callbacks.h"
#include "muduo/net/Channel.h"

// TimerQueue数据结构的选择，能快速根据当前时间找到已到期的定时器，也要高效的添加和删除Timer,因此可以使用二叉搜索树
namespace muduo
{
  namespace net
  {

    class EventLoop;
    class Timer;
    class TimerId;

    ///
    /// A best efforts timer queue.
    /// No guarantee that the callback will be on time.
    ///
    class TimerQueue : noncopyable
    {
    public:
      explicit TimerQueue(EventLoop *loop);
      ~TimerQueue();

      ///
      /// Schedules the callback to be run at given time,
      /// repeats if @c interval > 0.0.
      ///
      /// Must be thread safe. Usually be called from other threads.
      // 一定是线程安全的，可以跨线程调用，通常情况下被其他线程调用
      TimerId addTimer(TimerCallback cb, Timestamp when, double interval);

      void cancel(TimerId timerId);

    private:
      // FIXME: use unique_ptr<Timer> instead of raw pointers.
      // This requires heterogeneous comparison lookup (N3465) from C++14
      // so that we can find an T* in a set<unique_ptr<T>>.
      typedef std::pair<Timestamp, Timer *> Entry;
      typedef std::set<Entry> TimerList;  // 按照pair的规则排序，先比较时间戳再比较指针
      typedef std::pair<Timer *, int64_t> ActiveTimer;
      typedef std::set<ActiveTimer> ActiveTimerSet; // 按照pair的operator<排序，先比较指针再比较时间戳。

      // 以下成员函数只可能在其所属的I/O线程中调用，因而不必加锁。
      // 服务器性能杀手之一是锁竞争，所以要尽可能少用锁
      void addTimerInLoop(Timer *timer);
      void cancelInLoop(TimerId timerId);
      // called when timerfd alarms
      void handleRead();
      // move out all expired timers
      std::vector<Entry> getExpired(Timestamp now);
      void reset(const std::vector<Entry> &expired, Timestamp now);

      bool insert(Timer *timer);

      EventLoop *loop_;
      const int timerfd_;
      Channel timerfdChannel_;
      // Timer list sorted by expiration
      TimerList timers_;

      ActiveTimerSet activeTimers_;
      bool callingExpiredTimers_; /* atomic */
      // for cancel()
      ActiveTimerSet cancelingTimers_;
    };

  } // namespace net
} // namespace muduo
#endif // MUDUO_NET_TIMERQUEUE_H
