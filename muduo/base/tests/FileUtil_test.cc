#include "muduo/base/FileUtil.h"

#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace muduo;

int main()
{
    string result;
    int64_t size = 0;

    int err = FileUtil::readFile("/proc/self", 1024, &result, &size);   // /proc/self是一个目录，所以读取会失败
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
    err = FileUtil::readFile("/proc/self", 1024, &result, NULL);
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
    err = FileUtil::readFile("/proc/self/cmdline", 1024, &result, &size);
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
    err = FileUtil::readFile("/dev/null", 1024, &result, &size);    // 读取不到数据
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
    err = FileUtil::readFile("/dev/zero", 1024, &result, &size);    // 读到1024个0字符
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
    result.clear();
    err = FileUtil::readFile("/notexist", 1024, &result, &size);    // 2 0 0 
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
    err = FileUtil::readFile("/dev/zero", 102400, &result, &size);
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
    err = FileUtil::readFile("/dev/zero", 102400, &result, NULL);
    printf("%d %zd %" PRIu64 "\n", err, result.size(), size);
}
