#pragma once
#include "Logger.h"
#include "CServer.h"
#include <map>
#include "HttpParser.h"
#include "MysqlClient.h"
#include "Crypto.h"
#include "jsoncpp/json.h"

DECLARE_TABLE_CLASS(edoyunLogin_user_mysql, _mysql_table_)
DECLARE_MYSQL_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")  //QQ号
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_phone, DEFAULT, "VARCHAR", "(11)", "18888888888", "")  //手机
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_name, NOT_NULL, "TEXT", "", "", "")    //姓名
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_nick, NOT_NULL, "TEXT", "", "", "")    //昵称
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_wechat_id, DEFAULT, "TEXT", "", "NULL", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_address, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_province, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_country, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_age, DEFAULT | CHECK, "INTEGER", "", "18", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_male, DEFAULT, "BOOL", "", "1", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_flags, DEFAULT, "TEXT", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_experience, DEFAULT, "REAL", "", "0.0", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_level, DEFAULT | CHECK, "INTEGER", "", "0", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_class_priority, DEFAULT, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_REAL, user_time_per_viewer, DEFAULT, "REAL", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_career, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_password, NOT_NULL, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_birthday, NONE, "DATETIME", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_describe, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_education, NONE, "TEXT", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_INT, user_register_time, DEFAULT, "DATETIME", "", "CURRENT_TIMESTAMP", "")
DECLARE_TABLE_CLASS_EDN()

/*
* 1. 客户端的地址问题
* 2. 连接回调的参数问题
* 3. 接收回调的参数问题
*/
#define ERR_RETURN(ret, err) if(ret!=0){TRACEE("ret= %d errno = %d msg = [%s]", ret, errno, strerror(errno));return err;}

#define WARN_CONTINUE(ret) if(ret!=0){TRACEW("ret= %d errno = %d msg = [%s]", ret, errno, strerror(errno));continue;}

class CEdoyunPlayerServer :
	public CBusiness
{
public:
	CEdoyunPlayerServer(unsigned count) :CBusiness() {
		m_count = count;
	}
	~CEdoyunPlayerServer() {
		m_epoll.Close();
		m_pool.Close();
		for (auto it : m_mapClients) {
			if (it.second) {
				delete it.second;
			}
		}
		m_mapClients.clear();
	}
	virtual int BusinessProcess(CProcess* proc) {
		using namespace std::placeholders;
		int ret = 0;
		m_db = new CMysqlClient();
		if (m_db == NULL) {
			TRACEE("no more memory!");
			return -1;
		}
		KeyValue args;
		args["host"] = "192.168.58.128";
		args["user"] = "admin_remote";
		args["password"] = "txk417123";
		args["port"] = 3306;
		args["db"] = "edoyun";
		ret = m_db->Connect(args);
		ERR_RETURN(ret, -2);
		edoyunLogin_user_mysql user;
		ret = m_db->Exec(user.Create());
		ERR_RETURN(ret, -3);
		ret = setConnectedCallback(&CEdoyunPlayerServer::Connected, this, _1);
		ERR_RETURN(ret, -4);
		ret = setRecvCallback(&CEdoyunPlayerServer::Received, this, _1, _2);
		ERR_RETURN(ret, -5);
		ret = m_epoll.Create(m_count);
		ERR_RETURN(ret, -6);
		ret = m_pool.Start(m_count);
		ERR_RETURN(ret, -7);
		for (unsigned i = 0; i < m_count; i++) {
			ret = m_pool.AddTask(&CEdoyunPlayerServer::ThreadFunc, this);
			ERR_RETURN(ret, -8);
		}
		int sock = 0;
		sockaddr_in addrin;
		while (m_epoll != -1) {
			ret = proc->RecvSocket(sock, &addrin);
			if (ret < 0 || (sock == 0))break;
			CSocketBase* pClient = new CSocket(sock);
			if (pClient == NULL)continue;
			ret = pClient->Init(CSockParam(&addrin, SOCK_ISIP));
			WARN_CONTINUE(ret);
			ret = m_epoll.Add(sock, EpollData((void*)pClient));
			if (m_connectedcallback) {
				(*m_connectedcallback)(pClient);
			}
			WARN_CONTINUE(ret);
		}
		return 0;
	}
private:
	int Connected(CSocketBase* pClient) {
		//TODO:客户端连接处理 简单打印一下客户端信息
		sockaddr_in* paddr = *pClient;
		TRACEI("client connected addr %s port:%d", inet_ntoa(paddr->sin_addr), paddr->sin_port);
		return 0;
	}
	int Received(CSocketBase* pClient, const Buffer& data) {
		//TODO:主要业务，在此处理
		//HTTP 解析
		int ret = 0;
		Buffer response = "";
		ret = HttpParser(data);
		//验证结果的反馈
		if (ret != 0) {//验证失败
			TRACEE("http parser failed!%d", ret);
		}
		response = MakeResponse(ret);
		ret = pClient->Send(response);
		if (ret != 0) {
			TRACEE("http response failed!%d [%s]", ret, (char*)response);
		}
		else {
			TRACEI("http response success!%d", ret);
		}
		return 0;
	}
private:
	int HttpParser(const Buffer& data) {
		CHttpParser parser;
		size_t size = parser.Parser(data);
		if (size == 0 || (parser.Errno() != 0)) {
			TRACEE("size %llu errno:%u", size, parser.Errno());
			return -1;
		}
		TRACEI("parser.Method() 返回的值是: %d", parser.Method());
		if (parser.Method() == HTTP_GET) {
			//get 处理
			UrlParser url("https://192.168.1.100" + parser.Url());
			int ret = url.Parser();
			if (ret != 0) {
				TRACEE("ret = %d url[%s]", ret, "https://192.168.1.100" + parser.Url());
				return -2;
			}
			Buffer uri = url.Uri();

			TRACEI("解析出的 uri 是: [%s]", (char*)uri);
			if (uri == "/favicon.ico") {
				TRACEI("收到 favicon.ico 请求，忽略");
				return 0; // 或者返回特定的成功码，表示正常处理完毕
			}
			if (uri == "login") {
				//处理登录
				Buffer time = url["time"];
				Buffer salt = url["salt"];
				Buffer user = url["user"];
				Buffer sign = url["sign"];
				TRACEI("time %s salt %s user %s sign %s", (char*)time, (char*)salt, (char*)user, (char*)sign);
				//数据库的查询
				edoyunLogin_user_mysql dbuser;
				Result result;
				Buffer sql = dbuser.Query("user_name=\"" + user + "\"");
				TRACEI("到172，sql");
				ret = m_db->Exec(sql, result, dbuser);
				// ====== 新增：结果集全面体检日志 ======
				if (!result.empty()) {
					auto user1 = result.front();
					TRACEI("===== 数据库结果集体检开始 =====");
					TRACEI("结果集总行数: %d", (int)result.size());

					// 遍历 Fields 这个 map，看看里面到底装了什么
					for (auto& pair : user1->Fields) {
						PField field = pair.second;
						if (field != nullptr) {
							// 打印：Map的Key、对象内部的Name、nType、以及Value.String的内容
							TRACEI("字段 -> MapKey=[%s], Name=[%s], nType=%d, StringPtr=%p",
								(char*)pair.first,
								(char*)field->Name,
								field->nType,
								field->Value.String);
						}
						else {
							TRACEE("字段 -> MapKey=[%s], 但 PField 指针本身是 nullptr!", (char*)pair.first);
						}
					}
					TRACEI("===== 数据库结果集体检结束 =====");
				}
				else {
					TRACEE("数据库查询结果为空！");
				}
				// ================================
				if (ret != 0) {
					TRACEE("sql=%s ret=%d", (char*)sql, ret);
					return -3;
				}
				if (result.size() == 0) {
					TRACEE("no result sql=%s ret=%d", (char*)sql, ret);
					return -4;
				}
				if (result.size() != 1) {
					TRACEE("more than one sql=%s ret=%d", (char*)sql, ret);
					return -5;
				}
				TRACEI("到186");
				auto user1 = result.front();
				Buffer pwd; // 先声明一个空的 Buffer

				
				// 1. 使用 find() 安全查找
				auto it = user1->Fields.find("user_password");
				if (it != user1->Fields.end()) {
					PField field = it->second;
					if (field != nullptr) {
						// 【关键日志】：打印这个 nType=0 的对象的 Name！
						TRACEE("找到 user_password 字段，但 Value.String 为空！Name=[%s], nType=%d",
							(char*)field->Name, field->nType);
					}
					else {
						TRACEE("Fields 里的 user_password 指针本身是 nullptr！");
					}
				}
				else {
					TRACEE("Fields 里根本没有 user_password 这个键！");
				}
				TRACEI("password = %s", (char*)pwd);
				//登录请求的验证
				const char* MD5_KEY = "*&^%$#@b.v+h-b*g/h@n!h#n$d^ssx,.kl<kl";
				Buffer md5str = time + MD5_KEY + pwd + salt;
				Buffer md5 = Crypto::MD5(md5str);
				TRACEI("md5 = %s", (char*)md5);
				if (md5 == sign) {
					TRACEI("md5 == sign\n", (char*)pwd);
					return 0;
				}
				return -6;
			}
		}
		else if (parser.Method() == HTTP_POST) {
			//post 处理
		}
		return -7;
	}
	Buffer MakeResponse(int ret) {
		Json::Value root;
		root["status"] = ret;
		if (ret != 0) {
			root["message"] = "登录失败，可能是用户名或者密码错误！";
		}
		else {
			root["message"] = "success";
		}
		Buffer json = root.toStyledString();
		Buffer result = "HTTP/1.1 200 OK\r\n";
		time_t t;
		time(&t);
		tm* ptm = localtime(&t);
		char temp[64] = "";
		strftime(temp, sizeof(temp), "%a, %d %b %G %T GMT\r\n", ptm);
		Buffer Date = Buffer("Date: ") + temp;
		Buffer Server = "Server: Edoyun/1.0\r\nContent-Type: text/html; charset=utf-8\r\nX-Frame-Options: DENY\r\n";
		snprintf(temp, sizeof(temp), "%d", json.size());
		Buffer Length = Buffer("Content-Length: ") + temp + "\r\n";
		Buffer Stub = "X-Content-Type-Options: nosniff\r\nReferrer-Policy: same-origin\r\n\r\n";
		result += Date + Server + Length + Stub + json;
		TRACEI("response: %s", (char*)result);
		return result;
	}
private:
	int ThreadFunc() {
		int ret = 0;
		EPEvents events;
		while (m_epoll != -1) {
			ssize_t size = m_epoll.WaitEvents(events);
			if (size < 0)break;
			if (size > 0) {
				for (ssize_t i = 0; i < size; i++)
				{
					if (events[i].events & EPOLLERR) {
						break;
					}
					else if (events[i].events & EPOLLIN) {
						CSocketBase* pClient = (CSocketBase*)events[i].data.ptr;
						if (pClient) {
							Buffer data;
							ret = pClient->Recv(data);
							TRACEI("recv data size %d", ret);
							if (ret <= 0) {
								TRACEW("ret= %d errno = %d msg = [%s]", ret, errno, strerror(errno));
								m_epoll.Del(*pClient);
								continue;
							}
							if (m_recvcallback) {
								(*m_recvcallback)(pClient, data);
							}
						}
					}
				}
			}
		}
		return 0;
	}
private:
	CEpoll m_epoll;
	std::map<int, CSocketBase*> m_mapClients;
	CThreadPool m_pool;
	unsigned m_count;
	CDatabaseClient* m_db;
};