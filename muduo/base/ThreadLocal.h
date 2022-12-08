// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include "muduo/base/Mutex.h"
#include "muduo/base/noncopyable.h"

#include <pthread.h>

namespace muduo
{

  // 非POD类型使用这个
  template <typename T>
  class ThreadLocal : noncopyable
  {
  public:
    ThreadLocal()
    {
      MCHECK(pthread_key_create(&pkey_, &ThreadLocal::destructor)); // 线程结束的时候自动调用ThreadLocal::destructor
    }

    ~ThreadLocal()
    {
      MCHECK(pthread_key_delete(pkey_));  // 删除key（只是销毁key,不负责销毁数据）
    }

    // 返回实际数据
    // 先判定指针是否为空，为空就创建该数据并设定，不为空则直接返回保存的数据
    T &value()
    {
      T *perThreadValue = static_cast<T *>(pthread_getspecific(pkey_));
      if (!perThreadValue)
      {
        T *newObj = new T();
        MCHECK(pthread_setspecific(pkey_, newObj));
        perThreadValue = newObj;
      }
      return *perThreadValue;
    }

  private:
    static void destructor(void *x)
    {
      T *obj = static_cast<T *>(x);

      // 非完全类型 让其在编译阶段报错
      typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
      T_must_be_complete_type dummy;
      (void)dummy;

      delete obj;
    }

  private:
    pthread_key_t pkey_; // 不同的线程同key不同value，解决了__thread 只能修饰POD类型的缺陷。
  };

} // namespace muduo

#endif // MUDUO_BASE_THREADLOCAL_H
