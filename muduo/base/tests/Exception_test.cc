#include "muduo/base/CurrentThread.h"
#include "muduo/base/Exception.h"
#include <functional>
#include <vector>
#include <stdio.h>

class Bar
{
public:
  void test(std::vector<std::string> names = {})
  {
#if 1
    printf("Stack:\n%s\n", muduo::CurrentThread::stackTrace(true).c_str());
    // printf("Stack:\n%s\n", muduo::CurrentThread::stackTrace(false).c_str());
#endif

#if 0
    [] {
      printf("Stack inside lambda:\n%s\n", muduo::CurrentThread::stackTrace(true).c_str());
    }();
#endif

#if 0
    std::function<void()> func([] {
      printf("Stack inside std::function:\n%s\n", muduo::CurrentThread::stackTrace(true).c_str());
    });
    func();
#endif

#if 0
    auto func = std::bind(&Bar::callback, this);
    func();
#endif

    throw muduo::Exception("oops"); // call Exception constructor
  }

private:
  void callback()
  {
    printf("Stack inside std::bind:\n%s\n", muduo::CurrentThread::stackTrace(true).c_str());
  }
};

void foo()
{
  Bar b;
  b.test();
}

int main()
{
  try
  {
    foo();
  }
  catch (const muduo::Exception &ex)
  {
    printf("reason: %s\n", ex.what());
    printf("stack trace:\n%s\n", ex.stackTrace());
  }
}
