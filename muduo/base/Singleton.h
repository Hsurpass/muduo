// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include "muduo/base/noncopyable.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h> // atexit
#include <stdio.h>

namespace muduo
{

    namespace detail
    {
        // This doesn't detect inherited member functions!
        // http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions

        /*
          has_no_destroy 实现了在编译期间判断泛型T中是否存在no_destroy方法。
          这个实现的原理追究起来也就是对模板编程中的SFINAE(Substition Failure is not a error)(匹配失败不是错误)应用。

          分析下流程：
            1.当我们写入detail::has_no_destroy<T>::value 这段代码的时候，会触发sizeof(test<T>(0))的计算。
            2.首先匹配模板 template <typename C> static char test(decltype(&C::no_destroy)); 只有在C有no_destroy成员的时候才能匹配。
              如果匹配成功，那么test函数的返回类型也就为char，否则根据SFINAE原理，test函数的返回类型就是int32_t。
              (这个地方我一开始理解test是一个变量，这是不对的，sizeof关键字能够在编译期确定类型大小，所以无需提供函数体也能得到返回值大小。）
            3.再通过判断test返回值类型的大小，就可以推断出T类中是否存在no_destroy成员函数。

          为什么使用test(0)而不是test(1)?
          因为0可以被解释为空指针，这可以被重载的test接受。虽然1是一个int，只能被第二个重载的test接受，但这意味着value将始终为false。

        */
        template <typename T>
        struct has_no_destroy
        {
            template <typename C>
            static char test(decltype(&C::no_destroy));

            template <typename C>
            static int32_t test(...);

            const static bool value = sizeof(test<T>(0)) == 1;
        };
    } // namespace detail

    template <typename T>
    class Singleton : noncopyable
    {
    public:
        Singleton() = delete;
        ~Singleton() = delete;

        // 即使多个线程都调用了instance()函数
        // pthread_once还是能保证init()函数只被执行一次
        // 这种方式比使用锁的效率高
        static T &instance()
        {
            pthread_once(&ponce_, &Singleton::init);
            assert(value_ != NULL);
            return *value_;
        }

        /*
          这句话其实就是定义了一个固定大小的char型数组，数组名为type_must_be_complete，数组大小是多少呢？
          是sizeof(T)==0 ? -1 : 1，若sizeof(T)非0，这个表达式的值为1，即typedef了一个大小为1的char型数组，否则定义一个大小为-1的数组。
          数组大小还能为负数？当然不能，于是就会报错，而且是编译期错误，于是就将一个动态运行时错误在编译时就发现了。

          接下来解释sizeof什么时候返回0. C/C++语言本身似乎没有这种情况，但有些编译器会作一些扩展，比如GCC对于incomplete type使用sizeof时，
          会返回0.那什么又叫做incomplete type呢，就是那些声明了，但没有定义的类型，例如：
          A a;
          extern A a;

          C++标准允许通过一个 delete 表达式删除指向不完全类的指针。
          如果该类有一个非平凡的析构函数，或者有一个类相关的 delete 操作符，那么其行为就是无定义的。
          因此编译器作了这种扩展，以将这种未定义的行为转为编译期错误，帮助程序员们及早发现。
          函数的第二行语句的作用据说是为了防止编译器优化，因为编译器检测到typedef的类型未被使用到的话可能就会将其优化掉，
          因而第二行语句使用了这个类型，告诉编译器这个typedef是有用的，不能优化掉。至于(void)强制类型转换，
          是为了消除编译器对未使用值的警告。
        */
        static void destroy()
        {
            // 非完全类型 让其在编译阶段报错
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            T_must_be_complete_type dummy;
            (void)dummy;

            delete value_;
            value_ = NULL;
        }

    private:
        static void init()
        {
            value_ = new T();

            int n = detail::has_no_destroy<T>::value;
            printf("detail::has_no_destroy<T>::value:%d\n", n);
            if (!detail::has_no_destroy<T>::value)
            {
                /* Register a function to be called when `exit' is called.  */
                // atexit()函数用来注册程序正常终止时要被调用的函数
                // 在一个程序中最多可以用atexit()注册32个处理函数
                // 这些处理函数的调用顺序与其注册的顺序相反
                // 即最先注册的最后调用，最后注册的最先调用
                ::atexit(destroy); // 程序退出时，自动销毁
            }
        }

    private:
        static pthread_once_t ponce_;
        static T *value_;
    };

    // 使用初值为PTHREAD_ONCE_INIT的ponce变量保证init()函数在本进程执行序列中仅执行一次，且能保证线程安全。
    // （我们还能用互斥锁的方式来实现线程安全，但效率没有pthread_once高）
    template <typename T>
    pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

    template <typename T>
    T *Singleton<T>::value_ = NULL;

} // namespace muduo

#endif // MUDUO_BASE_SINGLETON_H
