// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/FileUtil.h"
#include "muduo/base/Logging.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace muduo;

/*
  muduo如何减少文件IO的诀窍就在这里 在setbuffer与writev之前选择了setbuffer,
  两者均可以减少磁盘IO，setbuffer可以先把数据写到一个用户态缓冲区中, 然后flush写入
  writev则可提供一次写入多块数据的功能
*/
FileUtil::AppendFile::AppendFile(StringArg filename)
    : fp_(::fopen(filename.c_str(), "ae")), // 'e' for O_CLOEXEC
      writtenBytes_(0)
{
    assert(fp_);
    ::setbuffer(fp_, buffer_, sizeof buffer_);  // 缓冲区超过 sizeof(buffer_) 大小也会自动刷新的
    // posix_fadvise POSIX_FADV_DONTNEED ?
}

FileUtil::AppendFile::~AppendFile()
{
    ::fclose(fp_);
}

void FileUtil::AppendFile::append(const char *logline, const size_t len)
{
    size_t written = 0;

    while (written != len)
    {
        // remain>0表示没写完， 需要继续写，直到写完
        size_t remain = len - written;
        size_t n = write(logline + written, remain);
        if (n != remain)
        {
            int err = ferror(fp_);// 没发生错误，继续写
            if (err)    // 发生错误时，break
            {
                fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
                break;
            }
        }
        written += n;
    }

    writtenBytes_ += written;
}

void FileUtil::AppendFile::flush()
{
    ::fflush(fp_);
}

/*
  这也是一个很有意思的函数，因为写文件内核中有默认的锁fwrite_unlocked是fwrite的不加锁版本，
  至于这里为什么不加锁呢，原因是这里的写入因为上层函数(logfile, asynclogging)中的锁,已经是线程安全的了
*/
size_t FileUtil::AppendFile::write(const char *logline, size_t len)
{
    // #undef fwrite_unlocked   
    return ::fwrite_unlocked(logline, 1, len, fp_); // 不加锁的方式写入，效率会高一点
}

FileUtil::ReadSmallFile::ReadSmallFile(StringArg filename)
    : fd_(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)),
      err_(0)
{
    buf_[0] = '\0';
    if (fd_ < 0)
    {
        err_ = errno;
    }
}

FileUtil::ReadSmallFile::~ReadSmallFile()
{
    if (fd_ >= 0)
    {
        ::close(fd_); // FIXME: check EINTR
    }
}

// return errno
template <typename String>
int FileUtil::ReadSmallFile::readToString(int maxSize,
                                          String *content,
                                          int64_t *fileSize,
                                          int64_t *modifyTime,
                                          int64_t *createTime)
{
    static_assert(sizeof(off_t) == 8, "_FILE_OFFSET_BITS = 64");
    assert(content != NULL);

    int err = err_;
    if (fd_ >= 0)
    {
        content->clear();   // 清空

        if (fileSize)
        {
            struct stat statbuf;
            if (::fstat(fd_, &statbuf) == 0)
            {
                if (S_ISREG(statbuf.st_mode))
                {
                    *fileSize = statbuf.st_size;
                    content->reserve(static_cast<int>(std::min(implicit_cast<int64_t>(maxSize), *fileSize)));
                }
                else if (S_ISDIR(statbuf.st_mode))
                {
                    err = EISDIR;
                }
                
                if (modifyTime)
                {
                    *modifyTime = statbuf.st_mtime;
                }
                if (createTime)
                {
                    *createTime = statbuf.st_ctime;
                }
            }
            else
            {
                err = errno;
            }
        }

        while (content->size() < implicit_cast<size_t>(maxSize))
        {
            size_t toRead = std::min(implicit_cast<size_t>(maxSize) - content->size(), sizeof(buf_));
            ssize_t n = ::read(fd_, buf_, toRead);

            if (n > 0)
            {
                content->append(buf_, n);
            }
            else
            {
                if (n < 0)
                {
                    err = errno;
                }
                break;
            }
        }
    }
    return err;
}

int FileUtil::ReadSmallFile::readToBuffer(int *size)
{
    int err = err_;
    if (fd_ >= 0)
    {
        ssize_t n = ::pread(fd_, buf_, sizeof(buf_) - 1, 0);
        if (n >= 0)
        {
            if (size)
            {
                *size = static_cast<int>(n);
            }
            buf_[n] = '\0';
        }
        else
        {
            err = errno;
        }
    }
    return err;
}

template int FileUtil::readFile(StringArg filename,
                                int maxSize,
                                string *content,
                                int64_t *, int64_t *, int64_t *);

template int FileUtil::ReadSmallFile::readToString(
    int maxSize,
    string *content,
    int64_t *, int64_t *, int64_t *);
