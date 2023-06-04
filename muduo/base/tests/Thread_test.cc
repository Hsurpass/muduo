#include "muduo/base/Thread.h"
#include "muduo/base/CurrentThread.h"

#include <string>
#include <stdio.h>
#include <unistd.h>

void mysleep(int seconds)
{
  timespec t = {seconds, 0};
  nanosleep(&t, NULL);
}

void threadFunc()
{
  printf("threadFunc, tid=%d\n", muduo::CurrentThread::tid());
}

void threadFunc2(int x)
{
  printf("threadFunc3, tid=%d, x=%d\n", muduo::CurrentThread::tid(), x);
}

void threadFunc3()
{
  printf("threadFunc3, tid=%d\n", muduo::CurrentThread::tid());
  mysleep(3);
}

class Foo
{
public:
  explicit Foo(double x)
      : x_(x)
  {
  }

  void memberFunc()
  {
    printf("memberFunc, tid=%d, Foo::x_=%f\n", muduo::CurrentThread::tid(), x_);
  }

  void memberFunc2(const std::string &text)
  {
    printf("memberFunc2, tid=%d, Foo::x_=%f, text=%s\n", muduo::CurrentThread::tid(), x_, text.c_str());
  }

private:
  double x_;
};

void test_thread1()
{
  muduo::Thread t1(threadFunc);
  t1.start();
  printf("t1.tid=%d\n", t1.tid());
  t1.join();
}

void test_thread2()
{
  muduo::Thread t2(std::bind(threadFunc2, 42), "thread for free function with argument");
  t2.start();
  printf("t2.tid=%d\n", t2.tid());
  t2.join();
}

void test_thread3()
{
  Foo foo(87.53);

  muduo::Thread t3(std::bind(&Foo::memberFunc, &foo), "thread for member function without argument");
  t3.start();
  t3.join();
}

void test_thread4()
{
  Foo foo(100.53);

  muduo::Thread t4(std::bind(&Foo::memberFunc2, std::ref(foo), std::string("Shuo Chen")));
  t4.start();
  t4.join();
}

// 子线程比主线程退出的晚，主线程没join-----> 子线程detach
void test_thread5()
{
  {
    muduo::Thread t5(threadFunc3);
    t5.start();
    // t5 may destruct eariler than thread creation.
  }

  mysleep(10);
}

// 子线程比主线程退出的早，主线程没join-----> 子线程detach
void test_thread6()
{
  {
    muduo::Thread t6(threadFunc3);
    t6.start();
    mysleep(5);
    // t6 destruct later than thread creation.
  }
}

int main()
{
  printf("pid=%d, tid=%d\n", ::getpid(), muduo::CurrentThread::tid());

  // test_thread1();
  // test_thread2();
  // test_thread3();
  // test_thread4();
  test_thread5();
  // test_thread6();

  printf("number of created threads %d\n", muduo::Thread::numCreated());
}
