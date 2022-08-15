#include "muduo/base/ThreadLocal.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Thread.h"

#include <stdio.h>

class Test : muduo::noncopyable
{
public:
  Test()
  {
    printf("Test(), tid=%d, constructing %p\n", muduo::CurrentThread::tid(), this);
  }

  ~Test()
  {
    printf("~Test(), tid=%d, destructing %p %s\n", muduo::CurrentThread::tid(), this, name_.c_str());
  }

  const muduo::string &name() const { return name_; }
  void setName(const muduo::string &n) { name_ = n; }

private:
  muduo::string name_;
};

// 全局变量 同名不同value
muduo::ThreadLocal<Test> testObj1;
muduo::ThreadLocal<Test> testObj2;

void print()
{
  printf("[print] tid=%d, obj1 %p name=%s\n",
         muduo::CurrentThread::tid(),
         &testObj1.value(),
         testObj1.value().name().c_str());

  printf("[print] tid=%d, obj2 %p name=%s\n",
         muduo::CurrentThread::tid(),
         &testObj2.value(),
         testObj2.value().name().c_str());
}

void threadFunc()
{
  print();
  testObj1.value().setName("changed 1");
  testObj2.value().setName("changed 42");
  print();
}

// 每次构造的value的地址是不一样的
int main()
{
  testObj1.value().setName("main one"); // testObj1 constructing
  print();                              // tid=%d, obj1 %p name=main one
                                        // testObj2 constructing
                                        // tid=%d, obj1 %p name=

  muduo::Thread t1(threadFunc);         // testObj1 constructing --> tid=%d, obj1 %p name=
  t1.start();                           // testObj2 constructing --> tid=%d, obj1 %p name=
  t1.join();                            // tid=%d, obj1 %p name=changed 1
                                        // tid=%d, obj1 %p name=changed 42
                                        // testObj1 destructing
                                        // testObj2 destructing

  testObj2.value().setName("main two"); 
  print();                              // tid=%d, obj1 %p name=main one
                                        // tid=%d, obj1 %p name=main two
                                        // testObj1 destructing
                                        // testObj2 destructing
                                        
  pthread_exit(0);
}
