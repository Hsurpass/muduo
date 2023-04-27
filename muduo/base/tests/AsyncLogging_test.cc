#include "muduo/base/AsyncLogging.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Timestamp.h"

#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>

off_t kRollSize = 500 * 1000 * 1000;    // 500M

muduo::AsyncLogging *g_asyncLog = NULL;

void asyncOutput(const char *msg, int len)
{
    g_asyncLog->append(msg, len);
}

void bench(bool longLog)
{
    muduo::Logger::setOutput(asyncOutput);

    int cnt = 0;
    const int kBatch = 1000;
    muduo::string empty = " ";
    muduo::string longStr(3000, 'X');
    longStr += " ";

    for (int t = 0; t < 30; ++t)
    {
        muduo::Timestamp start = muduo::Timestamp::now();
        for (int i = 0; i < kBatch; ++i)
        {
            LOG_INFO << "Hello 0123456789"
                     << " abcdefghijklmnopqrstuvwxyz "
                     << (longLog ? longStr : empty)
                     << cnt;
            ++cnt;
        }
        muduo::Timestamp end = muduo::Timestamp::now();
        printf("%f\n", timeDifference(end, start) * 1000000 / kBatch);
        // 把下面这两行去掉，可以查看消息堆积的效果
        // 加剧消息堆积
        struct timespec ts = {0, 500 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
}

int main(int argc, char *argv[])
{
    {
        // set max virtual memory to 2GB. 设置虚拟内存
        size_t kOneGB = 1000 * 1024 * 1024;
        rlimit rl = {2 * kOneGB, 2 * kOneGB};
        setrlimit(RLIMIT_AS, &rl);
    }

    printf("pid = %d\n", getpid());

    char name[256] = {'\0'};
    strncpy(name, argv[0], sizeof name - 1);
    muduo::AsyncLogging log(::basename(name), kRollSize);
    log.start();
    g_asyncLog = &log;

    bool longLog = argc > 1;
    bench(longLog); // argc 大于 1写入长日志：./asynclogging_test 1
}
