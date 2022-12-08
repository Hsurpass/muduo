#include "muduo/base/ThreadPool.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Logging.h"

#include <stdio.h>
#include <unistd.h> // usleep

void print()
{
  printf("tid=%d\n", muduo::CurrentThread::tid());
}

void printString(const std::string &str)
{
  LOG_INFO << str;
  usleep(100 * 1000);
}

void test(int maxSize)
{
  LOG_WARN << "Test ThreadPool with max queue size = " << maxSize;
  muduo::ThreadPool pool("MainThreadPool");
  pool.setMaxQueueSize(maxSize);
  pool.start(2);

  LOG_WARN << "Adding";
  pool.run(print);
  pool.run(print);

  for (int i = 0; i < 0; ++i)
  {
    char buf[32];
    snprintf(buf, sizeof buf, "task %d", i);
    pool.run(std::bind(printString, std::string(buf)));
  }
  LOG_WARN << "Done";

  sleep(3);
  muduo::CountDownLatch latch(1);
  pool.run(std::bind(&muduo::CountDownLatch::countDown, &latch));
  latch.wait();
  pool.stop();
}

/*
 * Wish we could do this in the future.
void testMove()
{
  muduo::ThreadPool pool;
  pool.start(2);

  std::unique_ptr<int> x(new int(42));
  pool.run([y = std::move(x)]{ printf("%d: %d\n", muduo::CurrentThread::tid(), *y); });
  pool.stop();
}
*/

void longTask(int num)
{
  LOG_INFO << "longTask " << num;
  muduo::CurrentThread::sleepUsec(10000000);
}

void test2()
{
  LOG_WARN << "Test ThreadPool by stoping early.";
  muduo::ThreadPool pool("ThreadPool");
  pool.setMaxQueueSize(5);
  pool.start(3);

  // 只有一把锁，生产线程和消费线程之间会抢锁
  muduo::Thread thread1([&pool]() {
    for (int i = 0; i < 10; ++i)
    {
      pool.run(std::bind(longTask, i));
      printf("[test2] thread1 i:%d\n", i);
    }
  },
  "thread1");
  
  thread1.start();

  muduo::CurrentThread::sleepUsec(5000000);
  LOG_WARN << "stop pool";
  pool.stop(); // early stop

  thread1.join();
  // run() after stop()
  pool.run(print);
  LOG_WARN << "test2 Done";
}

int main()
{
  // test(0);
  // test(1);
  // test(5);
  // test(10);
  // test(50);
  test2();
}
