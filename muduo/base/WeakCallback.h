// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_WEAKCALLBACK_H
#define MUDUO_BASE_WEAKCALLBACK_H

#include <functional>
#include <memory>

namespace muduo
{
  /*
    object_保存了一个CLASS类型的对象，function_保存了一个不定参数的函数对象，
    并重载了operator(), 用于安全的将object_作为function_的参数来调用。
  */
  // A barely usable WeakCallback

  template <typename CLASS, typename... ARGS>
  class WeakCallback
  {
  public:
    WeakCallback(const std::weak_ptr<CLASS> &object,
                 const std::function<void(CLASS *, ARGS...)> &function)
        : object_(object), function_(function)
    {
    }

    // Default dtor, copy ctor and assignment are okay

    void operator()(ARGS &&...args) const
    {
      std::shared_ptr<CLASS> ptr(object_.lock()); // 使用weak_ptr可以保证安全的调用回调函数，也就是调用成员函数时，类对象还存在。
      if (ptr)
      {
        function_(ptr.get(), std::forward<ARGS>(args)...);
      }
      // else
      // {
      //   LOG_TRACE << "expired";
      // }
    }

  private:
    std::weak_ptr<CLASS> object_;
    std::function<void(CLASS *, ARGS...)> function_;
  };

  template <typename CLASS, typename... ARGS>
  WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS> &object,
                                                void (CLASS::*function)(ARGS...))
  {
    return WeakCallback<CLASS, ARGS...>(object, function);
  }

  template <typename CLASS, typename... ARGS>
  WeakCallback<CLASS, ARGS...> makeWeakCallback(const std::shared_ptr<CLASS> &object,
                                                void (CLASS::*function)(ARGS...) const)
  {
    return WeakCallback<CLASS, ARGS...>(object, function);
  }

} // namespace muduo

#endif // MUDUO_BASE_WEAKCALLBACK_H
