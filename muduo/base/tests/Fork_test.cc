#include "muduo/base/CurrentThread.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{
  __thread int x = 0; // __thread是进程共享的，当fork之后会复制之前的值
}

void print()
{
  printf("pid=%d tid=%d x=%d\n", getpid(), muduo::CurrentThread::tid(), x);
}

//        fork
//      /      /
//   parent   child
//            /   /
//        child  grandchild

int main()
{
  printf("parent %d\n", getpid());
  print();  // x == 0

  x = 1;
  print();  // x == 1

  pid_t p = fork();
  if (p == 0)
  {
    printf("chlid %d\n", getpid());
    // child
    print();  // x == 1

    x = 2;
    print();  // x == 2

    if (fork() == 0)
    {
      printf("grandchlid %d\n", getpid()); 

      print();  // x == 2
      x = 3;
      print();  // x == 3
    }
  }
  else
  {
    // parent
    print();  // x == 1
  }
}
