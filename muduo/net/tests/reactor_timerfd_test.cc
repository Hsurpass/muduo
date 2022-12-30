#include "muduo/net/EventLoop.h"
#include "muduo/net/Channel.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

EventLoop* g_loop;
int timerfd;

void timeOut(Timestamp reveiveTime)
{
    printf("Time out\n");
    uint64_t howMany;
    ssize_t n = read(timerfd, &howMany, sizeof(howMany));
    printf("read howMany:%ld, n:%ld\n", howMany, n);
    
    // g_loop->quit();
}

int main()
{
    EventLoop loop;
    g_loop = &loop;

    timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    net::Channel timerChannel(&loop, timerfd);
    timerChannel.setReadCallback(std::bind(timeOut, _1));
    timerChannel.enableReading();

    struct itimerspec howlong;
    bzero(&howlong, sizeof(howlong));
    howlong.it_value.tv_sec = 3;
    howlong.it_interval.tv_sec = 2;
    timerfd_settime(timerfd, 0, &howlong, NULL);

    loop.loop();

    return 0;
}

