#include "muduo/base/Singleton.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/ThreadLocal.h"
#include "muduo/base/Thread.h"

#include <stdio.h>
#include <unistd.h>

class Test : muduo::noncopyable
{
public:
  Test()
  {
    printf("Test(), tid=%d, constructing %p\n", muduo::CurrentThread::tid(), this);
  }

  ~Test()
  {
    printf("~Test(), tid=%d, destructing %p, name:%s\n", muduo::CurrentThread::tid(), this, name_.c_str());
  }

  const muduo::string &name() const { return name_; }
  void setName(const muduo::string &n) { name_ = n; }

private:
  muduo::string name_;
};

// muduo::ThreadLocal<Test> 是singleton的(全局变量且只有一个)，不同的线程不同的value(value地址也不一样)
// #define STL muduo::Singleton<muduo::ThreadLocal<Test> >::instance().value()
#define TSDInstance muduo::Singleton<muduo::ThreadLocal<Test> >::instance()
#define STL TSDInstance.value()

void print()
{
  printf("[print] tid=%d, TSDInstance:%p, STL:%p, name=%s\n",
         muduo::CurrentThread::tid(),
         &TSDInstance,
         &STL,
         STL.name().c_str());
}

void threadFunc(const char *changeTo)
{
  print();
  STL.setName(changeTo);
  sleep(3);
  print();
}

/*
Test(), tid=12303, constructing 0x7fffc0a3be90
[print] tid=12303, TSDInstance:0x7fffc0a3be70, STL:0x7fffc0a3be90, name=main one
begin execute callback.
Test(), tid=12304, constructing 0x7f773c000b20
[print] tid=12304, TSDInstance:0x7fffc0a3be70, STL:0x7f773c000b20, name=
begin execute callback.
Test(), tid=12305, constructing 0x7f7734000b20
[print] tid=12305, TSDInstance:0x7fffc0a3be70, STL:0x7f7734000b20, name=
[print] tid=12304, TSDInstance:0x7fffc0a3be70, STL:0x7f773c000b20, name=thread1
~Test(), tid=12304, destructing 0x7f773c000b20, name:thread1
[print] tid=12305, TSDInstance:0x7fffc0a3be70, STL:0x7f7734000b20, name=thread2
~Test(), tid=12305, destructing 0x7f7734000b20, name:thread2
[print] tid=12303, TSDInstance:0x7fffc0a3be70, STL:0x7fffc0a3be90, name=main one
~Thread(), this:0x7fffc8d50820
~Thread(), this:0x7fffc8d50750
~Test(), tid=12303, destructing 0x7fffc0a3be90, name:main one
*/
int main()
{
  STL.setName("main one");  // constructing
  print();

  muduo::Thread t1(std::bind(threadFunc, "thread1")); // 
  muduo::Thread t2(std::bind(threadFunc, "thread2")); // 
  t1.start();
  t2.start();
  t1.join();

  print();
  t2.join();
  
  pthread_exit(0);
}
