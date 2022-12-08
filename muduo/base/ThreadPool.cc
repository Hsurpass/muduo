// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/ThreadPool.h"

#include "muduo/base/Exception.h"
#include "muduo/base/Logging.h"

#include <assert.h>
#include <stdio.h>

using namespace muduo;

ThreadPool::ThreadPool(const string &nameArg)
    : mutex_(),
      notEmpty_(mutex_),
      notFull_(mutex_),
      name_(nameArg),
      maxQueueSize_(0),
      running_(false)
{
}

ThreadPool::~ThreadPool()
{
  if (running_)
  {
    stop();
  }
}

void ThreadPool::start(int numThreads)
{
  assert(threads_.empty());

  running_ = true;
  threads_.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i)
  {
    char id[32];
    snprintf(id, sizeof id, "%d", i + 1);
    threads_.emplace_back(new muduo::Thread(
        std::bind(&ThreadPool::runInThread, this), name_ + id));
    threads_[i]->start();
  }

  if (numThreads == 0 && threadInitCallback_) // numThreads == 0没有子线程，则threadInitCallback_就在主线程执行
  {
    threadInitCallback_();
  }
}

void ThreadPool::stop()
{
  {
    MutexLockGuard lock(mutex_);
    running_ = false;
    notEmpty_.notifyAll();
    notFull_.notifyAll();
  }

  for (auto &thr : threads_)
  {
    thr->join();
  }
}

size_t ThreadPool::queueSize() const
{
  MutexLockGuard lock(mutex_);
  return queue_.size();
}

// 在生产线程跑
void ThreadPool::run(Task task)
{
  if (threads_.empty()) // 如果线程队列是空的(没创建线程)就直接执行
  {
    printf("threads_ is empty, direct run\n");
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    while (isFull() && running_)
    {
      notFull_.wait(); // 生产者队列 为满阻塞
      LOG_INFO << "[ThreadPool::run] notFull wait finished";
    }
    if (!running_)
    {
      LOG_INFO << "[ThreadPool::run] Thread pool is not running!";
      return;
    }
    assert(!isFull());

    queue_.push_back(std::move(task));
    notEmpty_.notify();
  }
}

// 在消费线程跑
ThreadPool::Task ThreadPool::take()
{
  MutexLockGuard lock(mutex_);
  // always use a while-loop, due to spurious wakeup
  while (queue_.empty() && running_) // (消费者)
  {
    notEmpty_.wait(); // 为空阻塞
    printf("[take] notEmpty wait finished\n");
  }

  Task task;
  if (!queue_.empty())
  {
    task = queue_.front();
    queue_.pop_front();

    if (maxQueueSize_ > 0)
    {
      notFull_.notify();
    }
  }

  return task;
}

bool ThreadPool::isFull() const
{
  mutex_.assertLocked();
  return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
  try
  {
    if (threadInitCallback_)
    {
      threadInitCallback_();
    }

    while (running_)  // 如果线程池中途退出了，正在执行的任务继续执行不受影响，下次while时退出
    {
      Task task(take());
      if (task)
      {
        task();
      }
      else
      {
        printf("task is false\n");
      }
      
    }
  }
  catch (const Exception &ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
    abort();
  }
  catch (const std::exception &ex)
  {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    abort();
  }
  catch (...)
  {
    fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
    throw; // rethrow
  }
}
