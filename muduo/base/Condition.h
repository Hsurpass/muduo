// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include "muduo/base/Mutex.h"

#include <pthread.h>

namespace muduo
{

  class Condition : noncopyable
  {
  public:
    explicit Condition(MutexLock &mutex) : mutex_(mutex)
    {
      MCHECK(pthread_cond_init(&pcond_, NULL));
    }

    ~Condition()
    {
      MCHECK(pthread_cond_destroy(&pcond_));
    }

    void wait()
    {
      MutexLock::UnassignGuard ug(mutex_);  // holder_ 置为0说明当前线程不持有锁了， 离开作用域的时候holder_重新赋值表明又持有锁
      MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex()));
    }

    // returns true if time out, false otherwise.
    bool waitForSeconds(double seconds);

    void notify()
    {
      MCHECK(pthread_cond_signal(&pcond_));
    }

    void notifyAll()
    {
      MCHECK(pthread_cond_broadcast(&pcond_));
    }

  private:
    MutexLock &mutex_;  // 注意，这里是引用
    pthread_cond_t pcond_;
  };

} // namespace muduo

#endif // MUDUO_BASE_CONDITION_H
