// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include "muduo/base/Types.h"

namespace muduo
{
  namespace CurrentThread
  {
    // internal 同名不同值
    extern __thread int t_cachedTid;
    extern __thread char t_tidString[32];
    extern __thread int t_tidStringLength;
    extern __thread const char *t_threadName;
    void cacheTid();
    
    // 这个指令是gcc引入的，作用是允许程序员将最有可能执行的分支告诉编译器。这个指令的写法为：__builtin_expect(EXP, N)。意思是：EXP==N的概率很大。
    // 一般的使用方法是将__builtin_expect指令封装为likely和unlikely宏。这两个宏的写法如下.
    // #define likely(x) __builtin_expect(!!(x), 1) //x很可能为真       
    // #define unlikely(x) __builtin_expect(!!(x), 0) //x很可能为假
    // 首先要明确：
    // if(likely(value))  //等价于 if(value)
    // if(unlikely(value))  //也等价于 if(value)
    // https://www.jianshu.com/p/2684613a300f
    inline int tid()
    {
      if (__builtin_expect(t_cachedTid == 0, 0))  // t_cachedTid == 0 这个表达式很可能为0(false), 也就是t_cachedTid很可能不为0
      {
        cacheTid();
      }
      return t_cachedTid;
    }

    inline const char *tidString() // for logging
    {
      return t_tidString;
    }

    inline int tidStringLength() // for logging
    {
      return t_tidStringLength;
    }

    inline const char *name()
    {
      return t_threadName;
    }

    bool isMainThread();

    void sleepUsec(int64_t usec); // for testing

    string stackTrace(bool demangle);
  } // namespace CurrentThread
} // namespace muduo

#endif // MUDUO_BASE_CURRENTTHREAD_H
