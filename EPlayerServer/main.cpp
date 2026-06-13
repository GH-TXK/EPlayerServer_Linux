#include <fcntl.h>
#include "Process.h"
#include "Logger.h"



int CreateLogServer(CProcess* proc) {
    CLoggerServer server;
    int ret = server.Start();
    if (ret != 0) {
        printf("%s(%d):<%s> pid=%d errno:%d msg:%s ret:%d\n",
            __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno), ret);
    }
    int fd = 0;
    while (true) {
        ret = proc->RecvFD(fd);
        printf("%s(%d):<%s> fd=%d\n", __FILE__, __LINE__, __FUNCTION__, fd);
        if (fd <= 0)break;
    }
    ret = server.Close();
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    return 0;
}

int CreateClientServer(CProcess* proc) {
    printf("%s(%d)<%s>:pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    int fd = -1;
	char buf[10] = { 0 };
    int ret = proc->RecvFD(fd);
    //sleep(1);
	if (ret != 0)return -1;
    printf("%s(%d)<%s>:pid = %d, fd = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), fd);
    lseek(fd, 0, SEEK_SET);
    ssize_t readcount = read(fd, buf, sizeof(buf));
    close(fd);
    printf("%s(%d)<%s>:pid = %d,ret = %d,  buf = %s\n", __FILE__, __LINE__, __FUNCTION__,getpid(), readcount, buf);
    return 0;
}

int LogTest()
{
    char buffer[] = "hello edoyun! 冯老师";
    usleep(100 * 100);
    TRACEI("here is log %d %c %f %g %s 哈哈 嘻嘻 易道云", 10, 'A', 1.0f, 2.0, buffer);
    DUMPD((void*)buffer, (size_t)sizeof(buffer));
    //LOGE << 100 << " " << 'S' << " " << 0.12345f << " " << 1.23456789 << " " << buffer << " 易道云编程";
    return 0;
}

int main()
{
    CProcess logServer, clientServer;
    logServer.SetEntryFunction(CreateLogServer, &logServer);
    int ret = logServer.CreateSubProcess();
    if (ret != 0)return -1;
    LogTest();
    clientServer.SetEntryFunction(CreateClientServer, &clientServer);
    ret = clientServer.CreateSubProcess();
    if (ret != 0)return -2;
    usleep(100 * 000);
    int fd = open("./text.txt",
        O_RDWR | O_CREAT | O_APPEND,
        0644);   // 文件权限：rw-r--r--

    if (fd == -1) {
        printf("%s(%d)<%s>:pid = %d,errno:%d msg:%s\n", __FILE__, __LINE__, __FUNCTION__, errno, strerror(errno));
    }
    else {
        printf("%s(%d)<%s>:pid = %d,open success fd=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), fd);
    }
    ret = clientServer.SendFD(fd);
    if (ret != 0)printf("%s(%d)<%s>:pid = %d,ret = %d, errno:%d msg:%s\n", __FILE__, __LINE__, __FUNCTION__,ret, errno, strerror(errno));
    write(fd, "Edoyun", 6);
    close(fd);
    logServer.SendFD(-1);
    (void)getchar();
    printf("%s(%d)<%s>:pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    if (ret != 0)return -3;
    return 0;
}