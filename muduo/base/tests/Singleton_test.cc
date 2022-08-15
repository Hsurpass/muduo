#include "muduo/base/Singleton.h"
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
    printf("~Test() tid=%d, destructing %p, name:%s\n", muduo::CurrentThread::tid(), this, name_.c_str());
  }

  const muduo::string &name() const { return name_; }
  void setName(const muduo::string &n) { name_ = n; }

private:
  muduo::string name_;
};

class TestNoDestroy : muduo::noncopyable
{
public:
  // Tag member for Singleton<T>
  void no_destroy();

  TestNoDestroy()
  {
    printf("TestNoDestroy(), tid=%d, constructing TestNoDestroy %p\n", muduo::CurrentThread::tid(), this);
  }

  ~TestNoDestroy()
  {
    printf("~TestNoDestroy(), tid=%d, destructing TestNoDestroy %p\n", muduo::CurrentThread::tid(), this);
  }
};

void threadFunc()
{
  printf("[threadFunc] tid=%d, instance:%p, name=%s\n",
         muduo::CurrentThread::tid(),
         &muduo::Singleton<Test>::instance(),
         muduo::Singleton<Test>::instance().name().c_str());

  muduo::Singleton<Test>::instance().setName("only one, changed");
}

void test_singleton_Test()
{
  muduo::Singleton<Test>::instance().setName("only one");

  muduo::Thread t1(threadFunc);
  t1.start();
  t1.join();

  printf("[main] tid=%d, instance:%p, name=%s\n",
         muduo::CurrentThread::tid(),
         &muduo::Singleton<Test>::instance(),
         muduo::Singleton<Test>::instance().name().c_str());
}

void test_No_Destroy()
{
  // TestNoDestroy& tnd = muduo::Singleton<TestNoDestroy>::instance();
  muduo::Singleton<TestNoDestroy>::instance();
  printf("[main] with valgrind, you should see %zd-byte memory leak.\n", sizeof(TestNoDestroy));
  // tnd.~TestNoDestroy();
  muduo::Singleton<TestNoDestroy>::destroy();
}


int main()
{
  // test_singleton_Test();
  test_No_Destroy();
}
