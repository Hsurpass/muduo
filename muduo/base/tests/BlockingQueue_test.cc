#include "muduo/base/BlockingQueue.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Thread.h"

#include <memory>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>

class Test
{
public:
  Test(int numThreads) : latch_(numThreads)
  {
    for (int i = 0; i < numThreads; ++i)
    {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i);
      threads_.emplace_back(new muduo::Thread(
          std::bind(&Test::threadFunc, this), muduo::string(name)));
    }

    for (auto &thr : threads_)
    {
      thr->start();
    }
  }

  // 一个生产者多个消费者
  void run(int times)
  {
    printf("[run] waiting for count down latch\n");
    latch_.wait();
    printf("[run] all threads started\n");
    for (int i = 0; i < times; ++i)
    {
      char buf[32];
      snprintf(buf, sizeof buf, "hello %d", i);
      queue_.put(buf);
      printf("[run] tid=%d, put data = %s, size = %zd\n", muduo::CurrentThread::tid(), buf, queue_.size());
    }
  }

  void joinAll()
  {
    for (size_t i = 0; i < threads_.size(); ++i)
    {
      queue_.put("stop");
    }

    for (auto &thr : threads_)
    {
      thr->join();
    }
  }

private:
  void threadFunc()
  {
    printf("[threadFunc] tid=%d, %s started\n", muduo::CurrentThread::tid(), muduo::CurrentThread::name());

    latch_.countDown();
    bool running = true;
    while (running) // 不断的取任务，处理，当队列中是stop时，退出
    {
      std::string d(queue_.take());
      printf("[threadFunc] tid=%d, get data = %s, size = %zd\n", muduo::CurrentThread::tid(), d.c_str(), queue_.size());
      running = (d != "stop");
    }

    printf("[threadFunc] tid=%d, %s stopped\n", muduo::CurrentThread::tid(), muduo::CurrentThread::name());
  }

  muduo::BlockingQueue<std::string> queue_;
  muduo::CountDownLatch latch_;
  std::vector<std::unique_ptr<muduo::Thread>> threads_;
};

void testMove()
{
  muduo::BlockingQueue<std::unique_ptr<int>> queue;
  queue.put(std::unique_ptr<int>(new int(42)));
  std::unique_ptr<int> x = queue.take();  // constructor
  printf("took %d, queue size:%zd\n", *x, queue.size()); // 42, 0
  *x = 123;
  queue.put(std::move(x));
  printf("queue size:%zd\n", queue.size()); // 1

  std::unique_ptr<int> y = queue.take();
  printf("took %d, queue size:%zd\n", *y, queue.size()); // 123 0
}

void testOneProducerMutilConsumer()
{
  Test t(2);
  t.run(10);
  t.joinAll();
}

int main()
{
  printf("[main] pid=%d, tid=%d\n", ::getpid(), muduo::CurrentThread::tid());

  testOneProducerMutilConsumer();
  // testMove();

  printf("[main] number of created threads %d\n", muduo::Thread::numCreated());
}
