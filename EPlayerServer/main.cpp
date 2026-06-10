#include <fcntl.h>
#include "Process.h"



int CreateLogServer(CProcess* proc) {
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


int main()
{
    CProcess logServer, clientServer;
    logServer.SetEntryFunction(CreateLogServer, &logServer);
    int ret = logServer.CreateSubProcess();
    if (ret != 0)return -1;
    clientServer.SetEntryFunction(CreateClientServer, &clientServer);
    ret = clientServer.CreateSubProcess();
    if (ret != 0)return -2;
    usleep(100 * 000);
    int fd = open("./text.txt", O_RDWR | O_CREAT | O_APPEND);
    printf("%s(%d)<%s>:pid = %d, fd = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), fd);
    ret = clientServer.SendFD(fd);
    printf("send fd ret = %d\n", ret);
    if (ret != 0)printf("errno:%d msg:%s\n", errno, strerror(errno));
    write(fd, "Edoyun", 6);
    close(fd);
    
    printf("%s(%d)<%s>:pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    if (ret != 0)return -3;
    return 0;
}