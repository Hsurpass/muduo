// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include "muduo/base/noncopyable.h"

#include <assert.h>
#include <pthread.h>

namespace muduo
{
  template <typename T>
  class ThreadLocalSingleton : noncopyable
  {
  public:
    ThreadLocalSingleton() = delete;
    ~ThreadLocalSingleton() = delete;

    // 不需要用线程安全的方式去创建，因为每个线程都有t_value_指针，只需要按照普通方式实现， 如果指针为空就创建对象.
    static T &instance()
    {
      if (!t_value_)
      {
        t_value_ = new T();
        deleter_.set(t_value_);
      }

      return *t_value_;
    }

    static T *pointer()
    {
      return t_value_;
    }

  private:
    // 如果是不完全类型的话， 在编译器就会报错  
    static void destructor(void *obj)
    {
      assert(obj == t_value_);

      typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
      T_must_be_complete_type dummy;
      (void)dummy;

      delete t_value_;
      t_value_ = 0;
    }

    class Deleter
    {
    public:
      Deleter()
      {
        pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
      }

      ~Deleter()
      {
        pthread_key_delete(pkey_);
      }

      void set(T *newObj)
      {
        assert(pthread_getspecific(pkey_) == NULL);
        pthread_setspecific(pkey_, newObj);
      }

      pthread_key_t pkey_;
    };

    /*
      t_value_类型是T*， 加上了一个__thread关键字，表示这个指针每一个线程都有一份， 
      deleter_用来销毁指针所指向的对象
    */
    static __thread T *t_value_;
    static Deleter deleter_;
  };

  template <typename T>
  __thread T *ThreadLocalSingleton<T>::t_value_ = 0;

  template <typename T>
  typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

} // namespace muduo
#endif // MUDUO_BASE_THREADLOCALSINGLETON_H
