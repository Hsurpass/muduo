#pragma once

#include "muduo/net/Acceptor.h"

#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

void newConnectionCb(int connfd, const InetAddress &peerAddr)
{
    printf("newConnectionCb, pid:%d, tid:%d, peerAddr:%s\n", getpid(), CurrentThread::tid(), peerAddr.toIpPort().c_str() );
    ssize_t n = write(connfd, "how are you?\n", 13);
    n = close(connfd);

}

// telnet 127.0.0.1 8888
void test_acceptor()
{
    net::InetAddress listenAddr(8888);
    EventLoop loop;

    net::Acceptor acceptor(&loop, listenAddr);
    acceptor.listen();

    loop.loop();
}

int main()
{
    test_acceptor();

    return 0;
}