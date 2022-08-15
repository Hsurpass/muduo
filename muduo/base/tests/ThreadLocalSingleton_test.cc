#include "muduo/base/ThreadLocalSingleton.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Thread.h"

#include <stdio.h>

class Test : muduo::noncopyable
{
public:
  Test()
  {
    printf("Test() tid=%d, constructing %p\n", muduo::CurrentThread::tid(), this);
  }

  ~Test()
  {
    printf("~Test() tid=%d, destructing %p %s\n", muduo::CurrentThread::tid(), this, name_.c_str());
  }

  const muduo::string &name() const { return name_; }
  void setName(const muduo::string &n) { name_ = n; }

private:
  muduo::string name_;
};

void threadFunc(const char *changeTo)
{
  printf("[threadFunc] tid=%d, instance:%p, name=%s\n",
         muduo::CurrentThread::tid(),
         &muduo::ThreadLocalSingleton<Test>::instance(),
         muduo::ThreadLocalSingleton<Test>::instance().name().c_str());

  muduo::ThreadLocalSingleton<Test>::instance().setName(changeTo);
  
  printf("[threadFunc] tid=%d, instance:%p, name=%s\n",
         muduo::CurrentThread::tid(),
         &muduo::ThreadLocalSingleton<Test>::instance(),
         muduo::ThreadLocalSingleton<Test>::instance().name().c_str());

  // no need to manually delete it
  // muduo::ThreadLocalSingleton<Test>::destroy();
}

/*
Test() tid=14021, constructing 0x7fffca2a2e70
begin execute callback.
begin execute callback.

Test() tid=14022, constructing 0x7fd69c000b20
[threadFunc] tid=14022, instance:0x7fd69c000b20, name=
[threadFunc] tid=14022, instance:0x7fd69c000b20, name=thread1

Test() tid=14023, constructing 0x7fd694000b20
[threadFunc] tid=14023, instance:0x7fd694000b20, name=
[threadFunc] tid=14023, instance:0x7fd694000b20, name=thread2

~Test() tid=14023, destructing 0x7fd694000b20 thread2
~Test() tid=14022, destructing 0x7fd69c000b20 thread1

[main] tid=14021, instance:0x7fffca2a2e70, name=main one
~Thread(), this:0x7fffd257dd50
~Thread(), this:0x7fffd257dc80
~Test() tid=14021, destructing 0x7fffca2a2e70 main one
*/

int main()
{
  muduo::ThreadLocalSingleton<Test>::instance().setName("main one");

  muduo::Thread t1(std::bind(threadFunc, "thread1"));
  muduo::Thread t2(std::bind(threadFunc, "thread2"));
  t1.start();
  t2.start();
  t1.join();

  printf("[main] tid=%d, instance:%p, name=%s\n",
         muduo::CurrentThread::tid(),
         &muduo::ThreadLocalSingleton<Test>::instance(),
         muduo::ThreadLocalSingleton<Test>::instance().name().c_str());
  t2.join();

  pthread_exit(0);
}
