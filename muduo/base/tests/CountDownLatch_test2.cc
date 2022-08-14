#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Thread.h"
#include <iostream>
#include <vector>
#include <memory>
#include <unistd.h>
#include <functional>
#include <algorithm>

using namespace std;
using namespace std::placeholders;
using namespace muduo;

// 主线程等待所有子线程减为1
class Test
{
public:
    Test(int threadsNum):m_latch(threadsNum)
    {
        for (int i = 0; i < threadsNum; i++)
        {
            char name[32] = {0};
            sprintf(name, "work thread %d", i);
            m_threads.emplace_back(new muduo::Thread( std::bind(&Test::threadFunc, this), muduo::string(name)) );
        }
        std::for_each(m_threads.begin(), m_threads.end(), std::bind(&muduo::Thread::start, _1) );
    }

    void wait()
    {
        m_latch.wait();
    }

    void joinAll()
    {
        std::for_each(m_threads.begin(), m_threads.end(), std::bind(&muduo::Thread::join, _1) );
    }
private:
    void threadFunc()
    {
        printf("tid=%d, %s started\n", CurrentThread::tid(), CurrentThread::name());
        m_latch.countDown();
        sleep(2);
        printf("tid=%d, %s stop\n", CurrentThread::tid(), CurrentThread::name());

    }

    CountDownLatch m_latch;
    std::vector<unique_ptr<muduo::Thread>> m_threads;
};

int main()
{
    printf("[main] pid:%d, tid:%d\n", getpid(), CurrentThread::tid());
    Test t(3);
    t.wait();
    printf("[main] pid:%d, tid:%d, %s wait finished\n", getpid(), CurrentThread::tid(), CurrentThread::name());
    t.joinAll();

    return 0;
}