#pragma once
#include <memory.h>
#include <sys/socket.h>
#include <unistd.h>
#include <functional>
#include <stdlib.h>
#include <cstdio>
#include <errno.h>
#include "Function.h"

class CProcess {
public:
    CProcess() {
        m_func = NULL;
        memset(pipes, 0, sizeof(pipes));
    }
    ~CProcess() {
        if (m_func != NULL) {
            delete m_func;
            m_func = NULL;
        }
    }
    template<typename _FUNCTION_, typename... _ARGS_>
    int SetEntryFunction(_FUNCTION_ func, _ARGS_... args) {
        m_func = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
        return 0;
    }

    int CreateSubProcess() {
        if (m_func == NULL) return -1;
        int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, pipes);
        if (ret == -1) return -2;
        pid_t pid = fork();
        if (pid < 0) return -3;
        if (pid == 0) {
            // 子进程
            close(pipes[1]);//关闭写管道
            pipes[1] = 0;
            ret = (*m_func)();
            exit(0);
        }
        // 主进程
        close(pipes[0]);//关闭读管道
        pipes[0] = 0;
        m_pid = pid;
        return 0;
    }

    int SendFD(int fd) {
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        iovec iov[2];
        memset(iov, 0, sizeof(iov));
        char buf[2][10] = { "Edoyun", "EPlayer" };
        iov[0].iov_base = buf[0];
        iov[0].iov_len = sizeof(buf[0]);
        iov[1].iov_base = buf[1];
        iov[1].iov_len = sizeof(buf[1]);
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;

        //下面的数据才是重要的，发送文件描述符
        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));
        if (cmsg == NULL) return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        printf("fd = %d\n", fd);
        *((int*)CMSG_DATA(cmsg)) = fd;
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;
        printf("fd = %d\n", fd);
        ssize_t ret = sendmsg(pipes[1], &msg, 0);
        free(cmsg);
        if (ret == -1) {
            printf("errno:%d msg:%s\n", errno, strerror(errno));
            return -2;
        }
        return 0;
    }

    int RecvFD(int& fd) {
        struct msghdr msg;
        iovec iov[2];
        char buf[][10] = { "","" };
        iov[0].iov_base = buf[0];
        iov[0].iov_len = sizeof(buf[0]);
        iov[1].iov_base = buf[1];
        iov[1].iov_len = sizeof(buf[1]);
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;

        cmsghdr* cmsg = (cmsghdr*)calloc(1, CMSG_LEN(sizeof(int)));
        if (cmsg == NULL)return -1;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        msg.msg_control = cmsg;
        msg.msg_controllen = cmsg->cmsg_len;
        ssize_t ret = recvmsg(pipes[0], &msg, 0);
        free(cmsg);
        if (ret == -1) {
            return -2;
        }
        fd = *((int*)CMSG_DATA(cmsg));
        return 0;
    }
private:
    CFunctionBase* m_func;
    pid_t m_pid;
    int pipes[2];
};