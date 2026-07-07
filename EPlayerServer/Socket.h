#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "Public.h"


enum SockAttr {
	SOCK_ISSERVER = 1,//是否服务器，1表示是，0表示客户端
	SOCK_ISNONBLOCK = 2,//是否阻塞， 1表示阻塞， 0表示非阻塞
	SOCK_ISUDP = 4,//是否为UDP， 1表示udp， 0表示tcp
	SOCK_ISIP = 8,//是否为IP协议， 1表示IP协议，0表示本地套接字
};

#define INET_ADDRSTRLEN 16
class CSockParam {
public:
	CSockParam() {
		memset(&addr_in, 0, sizeof(addr_in));
		memset(&addr_un, 0, sizeof(addr_un));
		port = -1;
		attr = 0;
	}
	CSockParam(const Buffer& ip, short port, int attr) {
		this->ip = ip;
		this->port = port;
		this->attr = attr;
		addr_in.sin_family = AF_INET;
		addr_in.sin_port = port;
		addr_in.sin_addr.s_addr = inet_addr(ip);
	}
	CSockParam(const sockaddr_in* addrin, int attr) {
		this->ip = inet_ntop(AF_INET, &(addrin->sin_addr), ip_str, INET_ADDRSTRLEN);
		this->port = ntohs(addrin->sin_port);
		this->attr = attr;
		memcpy(&addr_in, addrin, sizeof(addr_in));
	}
	CSockParam(const Buffer& path, int attr) {
		ip = path;
		addr_un.sun_family = AF_UNIX;
		strcpy(addr_un.sun_path, path);
		this->attr = attr;
	}
	~CSockParam() {};
	CSockParam(const CSockParam& param) {
		ip = param.ip;
		port = param.port;
		attr = param.attr;
		memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
		memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
	}
public:
	CSockParam& operator=(const CSockParam& param)
	{
		if (this != &param) {
			ip = param.ip;
			port = param.port;
			attr = param.attr;
			memcpy(&addr_in, &param.addr_in, sizeof(addr_in));
			memcpy(&addr_un, &param.addr_un, sizeof(addr_un));
		}
		return *this;
	}
	sockaddr* addrin() { return (sockaddr*)&addr_in; }
	sockaddr* addrun() { return (sockaddr*)&addr_un; }

public:
	sockaddr_in addr_in;
	sockaddr_un addr_un;
	Buffer ip;
	short port;
	int attr;
	char ip_str[INET_ADDRSTRLEN];
};

class CSocketBase
{
public:
	CSocketBase() {
		m_socket = -1;
		m_status = 0;//初始化完成
	}
	//传递析构操作
	virtual ~CSocketBase() {
		Close();
	}
public:
	//初始化 服务器 套接字创建、bind、listen  客户端 套接字创建
	virtual int Init(const CSockParam& param) = 0;
	//连接 服务器 accept 客户端 connect  对于udp这里可以忽略
	virtual int Link(CSocketBase** pClient = NULL) = 0;
	//发送数据
	virtual int Send(const Buffer& data) = 0;
	//接收数据
	virtual int Recv(Buffer& data) = 0;
	//关闭连接
	virtual int Close() {
		m_status = 3;
		if (m_socket != -1)
		{
			if ((m_param.attr & SOCK_ISSERVER)&& //服务器
				((m_param.attr & SOCK_ISIP) == 0)) {//非IP
				unlink(m_param.ip);
			}
			int fd = m_socket;
			m_socket = -1;
			close(fd);
		}
		return 0;
	}
	virtual operator int() { return m_socket; }
	virtual operator int()const { return m_socket; }
	virtual operator const sockaddr_in* ()const { return &m_param.addr_in; }
	virtual operator sockaddr_in* () { return &m_param.addr_in; }
protected:

	//套接字描述符， 默认是-1
	int m_socket;
	//状态0初始化未完成， 1初始化完成， 2连接完成， 3已经关闭
	int m_status;
	//初始化参数
	CSockParam m_param;
};

class CSocket
	:public CSocketBase
{
public:
	CSocket() :CSocketBase() {}
	CSocket(int sock) :CSocketBase() {
		m_socket = sock;
		m_status = 0;
	}
	//传递析构操作
	virtual ~CSocket() {
		Close();
	}
public:
	//初始化， 服务器套接字创建， bind， listen， 客户端套接字创建
	//virtual int Init(const CSockParam& param)
	//{
	//	// 已初始化过，直接返回
	//	if (m_status != 0)
	//		return 0;

	//	m_param = param;

	//	// 情况 1：普通初始化（客户端或服务器）
	//	if (m_socket == -1)
	//	{
	//		int type = (m_param.attr & SOCK_ISUDP) ? SOCK_DGRAM : SOCK_STREAM;
	//		m_socket = socket(PF_LOCAL, type, 0);
	//		if (m_socket == -1) return -2;

	//		m_status = 1; // 已初始化但未连接
	//	}
	//	else
	//	{
	//		// 情况 2：accept() 得到的新 fd
	//		m_status = 2; // 已连接
	//	}

	//	int ret = 0;

	//	// 服务器 bind + listen
	//	if (m_param.attr & SOCK_ISSERVER)
	//	{
	//		//unlink(m_param.ip); // 删除旧 socket 文件

	//		ret = bind(m_socket, m_param.addrun(), sizeof(sockaddr_un));
	//		if (ret == -1) return -3;

	//		ret = listen(m_socket, 32);
	//		if (ret == -1) return -4;

	//		m_status = 1; // server ready
	//	}

	//	// 非阻塞
	//	if (m_param.attr & SOCK_ISNONBLOCK)
	//	{
	//		int option = fcntl(m_socket, F_GETFL);
	//		if (option == -1) return -5;

	//		option |= O_NONBLOCK;
	//		ret = fcntl(m_socket, F_SETFL, option);
	//		if (ret == -1) return -6;
	//	}

	//	return 0;
	//}

	virtual int Init(const CSockParam& param) {
		//printf("inInit!!!\n");
		if (m_status != 0)return -1;
		m_param = param;
		int type = (m_param.attr & SOCK_ISUDP) ? SOCK_DGRAM : SOCK_STREAM;
		if (m_socket == -1) {
			if (param.attr & SOCK_ISIP) {
				m_socket = socket(PF_INET, type, 0);
			}
			else
			{
				m_socket = socket(PF_LOCAL, type, 0);
			}
		}
		else
			m_status = 2;//accept来的套接字已经处于连接状态
		if (m_socket == -1)return -2;
		int ret = 0;
		if (m_param.attr & SOCK_ISSERVER) {
			//printf("%s(%d)<%s>:pid = %d,isserver=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid(), m_socket);
			if(param.attr & SOCK_ISIP)
				ret = bind(m_socket, m_param.addrin(), sizeof(sockaddr_in));
			else
				ret = bind(m_socket, m_param.addrun(), sizeof(sockaddr_un));
			if (ret == -1)return -3;
			ret = listen(m_socket, 32);
			if (ret == -1)return -4;
		}
		if (m_param.attr & SOCK_ISNONBLOCK) {
			int option = fcntl(m_socket, F_GETFL);
			if (option == -1)return -5;
			option |= O_NONBLOCK;
			ret = fcntl(m_socket, F_SETFL, option);
			if (ret == -1)return -6;
		}
		if (m_status == 0)
			m_status = 1;
		return 0;
	}

	//连接服务器accept客户端connect对于udp这里可以忽略
	virtual int Link(CSocketBase** pClient = NULL)
	{

		if (m_status <= 0 || (m_socket == -1)) {
			//printf("%s(%d):[%s]m_socket=%d, m_status = %d\n", __FILE__, __LINE__, __FUNCTION__, m_socket, m_status);
			return -1;
		}
		int ret = 0;
		if (m_param.attr & SOCK_ISSERVER)
		{
			if (pClient == NULL)return -2;
			CSockParam param;
			int fd = -1;
			socklen_t len = 0;
			if (m_param.attr & SOCK_ISIP) {
				param.attr |= SOCK_ISIP;
				len = sizeof(sockaddr_in);
				fd = accept(m_socket, param.addrin(), &len);
			}
			else
			{
				len = sizeof(sockaddr_un);
				fd = accept(m_socket, param.addrun(), &len);
			}
			if (fd == -1)return -3;
			*pClient = new CSocket(fd);
			if (*pClient == NULL)return -4;
			printf("%s(%d)<%s>:pid = %d,socket=%d,serverLink!!!\n", __FILE__, __LINE__, __FUNCTION__, getpid(), m_socket);
			ret = (*pClient)->Init(param);
			if (ret != 0)
			{
				delete(*pClient);
				*pClient = NULL;
				return -5;
			}
		}
		else
		{
			if (m_param.attr & SOCK_ISIP)
				ret = connect(m_socket, m_param.addrin(), sizeof(sockaddr_in));
			else
				ret = connect(m_socket, m_param.addrun(), sizeof(sockaddr_un));
			if (ret != 0)return -6;
		}
		m_status = 2;
		return 0;
	}
	//发送数据
	virtual int Send(const Buffer& data)
	{
		if (m_status < 2 || (m_socket == -1))return -1;
		ssize_t index = 0;
		while (index < (ssize_t)data.size()) {
			ssize_t len = write(m_socket, (char*)data + index, data.size() - index);
			printf("len = %d,write is finish!!!\n", len);
			if (len == 0)return -2;
			if (len < 0)return -3;
			index += len;
		}
		return 0;
	}
	//接收数据大于零，表示接收成功，小于表示失败，等于0表示没有收到数据，但没有错误
	virtual int Recv(Buffer& data)
	{
		if (m_status < 2 || (m_socket == -1))return -1;
		ssize_t len = read(m_socket, data, data.size());
		if (len > 0)
		{
			data.resize(len);
			return (int)len;//收到数据
		}
		if (len < 0)
		{
			if (errno == EINTR || (errno == EAGAIN))
			{
				data.clear();
				return 0;//没有收到数据
			}
			return -2;//发送错误
		}
		return -3;//套接字被关闭
	}
	//关闭连接
	virtual int Close() {
		return CSocketBase::Close();
	}
};
