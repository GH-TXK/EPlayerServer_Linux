#pragma once
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>

class Buffer :public std::string
{
public:
	Buffer() :std::string() {}
	Buffer(size_t size) :std::string() { resize(size); }
	operator char* () { return (char*)c_str(); }
	operator char* () const { return (char*)c_str(); }
	operator const char* ()const { return c_str(); }
};

enum SockAttr {
	SOCK_ISSERVER = 1,//是否服务器，1表示是，0表示客户端
	SOCK_ISBLOK = 2,//是否阻塞， 1表示阻塞， 0表示非阻塞
};

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
	sockaddr_in addr_un;
	Buffer ip;
	short port;
	int attr;
};

class CSocketBase
{
public:
	//传递析构操作
	virtual ~CSocketBase() {
		m_status = 3;
		if (m_socket != -1)
		{
			int fd = m_socket;
			m_socket = -1;
			close(fd);
		}
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
	virtual int Close() = 0;
protected:
	//套接字描述符， 默认是-1
	int m_socket;
	//状态0初始化未完成， 1初始化完成， 2连接完成， 3已经关闭
	int m_status;
};
