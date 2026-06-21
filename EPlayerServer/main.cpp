#include "EdoyunPlayerServer.h"



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

int old_test()
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
    if (ret != 0)printf("%s(%d)<%s>:pid = %d,ret = %d, errno:%d msg:%s\n", __FILE__, __LINE__, __FUNCTION__, ret, errno, strerror(errno));
    write(fd, "Edoyun", 6);
    close(fd);
    CThreadPool pool;
    ret = pool.Start(4);
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    if (ret != 0)printf("errno:%d msg:%s\n", errno, strerror(errno));
    ret = pool.AddTask(LogTest);
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    if (ret != 0)printf("errno:%d msg:%s\n", errno, strerror(errno));
    ret = pool.AddTask(LogTest);
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    if (ret != 0)printf("errno:%d msg:%s\n", errno, strerror(errno));
    ret = pool.AddTask(LogTest);
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    if (ret != 0)printf("errno:%d msg:%s\n", errno, strerror(errno));
    ret = pool.AddTask(LogTest);
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    if (ret != 0)printf("errno:%d msg:%s\n", errno, strerror(errno));
    (void)getchar();
    pool.Close();
    logServer.SendFD(-1);
    (void)getchar();
    printf("%s(%d)<%s>:pid = %d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    if (ret != 0)return -3;
    return 0;
}

int Main()
{
    int ret = 0;
    CProcess proclog;
    ret = proclog.SetEntryFunction(CreateLogServer, &proclog);
    ERR_RETURN(ret, -1);
    ret = proclog.CreateSubProcess();
    ERR_RETURN(ret, -2);
    CEdoyunPlayerServer business(2);
    CServer server;
    ret = server.Init(&business);
    ERR_RETURN(ret, -3);
    ret = server.Run();
    ERR_RETURN(ret, -4);
    return 0;
}

#include "HttpParser.h"

int http_test()
{
    Buffer str = "GET /favicon.ico HTTP/1.1\r\n"
        "Host: 0.0.0.0=5000\r\n"
        "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*; q = 0.8\r\n"
        "Accept-Language: en-us,en;q=0.5\r\n"
        "Accept-Encoding: gzip,deflate\r\n"
        "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
        "Keep-Alive: 300\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    CHttpParser parser;
    size_t size = parser.Parser(str);
    if (parser.Errno() != 0) {
        printf("errno %d\n", parser.Errno());
        return -1;
    }
    if (size != 368) {
        printf("size error:%lld  %lld\n", size, str.size());
        return -2;
    }
    printf("method %d url %s\n", parser.Method(), (char*)parser.Url());
    str = "GET /favicon.ico HTTP/1.1\r\n"
        "Host: 0.0.0.0=5000\r\n"
        "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    size = parser.Parser(str);
    printf("errno %d size %lld\n", parser.Errno(), size);
    if (parser.Errno() != 0x7F) {
        return -3;
    }
    if (size != 0) {
        return -4;
    }
    UrlParser url1("https://www.baidu.com/s?ie=utf8&oe=utf8&wd=httplib&tn=98010089_dg&ch=3");

    int ret = url1.Parser();
    if (ret != 0) {
        printf("urlparser1 failed:%d\n", ret);
        return -5;
    }
    printf("ie = %s except:utf8\n", (char*)url1["ie"]);
    printf("oe = %s except:utf8\n", (char*)url1["oe"]);
    printf("wd = %s except:httplib\n", (char*)url1["wd"]);
    printf("tn = %s except:98010089_dg\n", (char*)url1["tn"]);
    printf("ch = %s except:3\n", (char*)url1["ch"]);
    UrlParser url2("http://127.0.0.1:19811/?time=144000&salt=9527&user=test&sign=1234567890abcdef");
    ret = url2.Parser();
    if (ret != 0) {
        printf("urlparser2 failed:%d\n", ret);
        return -6;
    }
    printf("time = %s except:144000\n", (char*)url2["time"]);
    printf("salt = %s except:9527\n", (char*)url2["salt"]);
    printf("user = %s except:test\n", (char*)url2["user"]);
    printf("sign = %s except:1234567890abcdef\n", (char*)url2["sign"]);
    printf("host:%s port:%d\n", (char*)url2.Host(), url2.Port());
    return 0;
}

#include "Sqlite3Client.h"
DECLARE_TABLE_CLASS(user_test, _sqlite3_table_)
DECLARE_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")
DECLARE_FIELD(TYPE_VARCHAR, user_phone, NOT_NULL | DEFAULT, "VARCHAR", "(12)", "18888888888", "")
DECLARE_FIELD(TYPE_TEXT, user_name, 0, "TEXT", "", "", "")
DECLARE_TABLE_CLASS_EDN()

int sql_test()
{
    user_test test, value;
    printf("create:%s\n", (char*)test.Create());
    printf("Delete:%s\n", (char*)test.Delete(test));
    value.Fields["user_qq"]->LoadFromStr("1817619619");
    value.Fields["user_qq"]->Condition = SQL_INSERT;
    printf("Insert:%s\n", (char*)test.Insert(value));
    value.Fields["user_qq"]->LoadFromStr("123456789");
    value.Fields["user_qq"]->Condition = SQL_MODIFY;
    printf("Modify:%s\n", (char*)test.Modify(value));
    printf("Query:%s\n", (char*)test.Query());
    printf("Drop:%s\n", (char*)test.Drop());
    getchar();
    int ret = 0;
    CDatabaseClient* pClient = new CSqlite3Client();
    KeyValue args;
    args["host"] = "test.db";
    ret = pClient->Connect(args);
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    ret = pClient->Exec(test.Create());
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    ret = pClient->Exec(test.Delete(value));
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    value.Fields["user_qq"]->LoadFromStr("1817619619");
    value.Fields["user_qq"]->Condition = SQL_INSERT;
    ret = pClient->Exec(test.Insert(value));
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    value.Fields["user_qq"]->LoadFromStr("123456789");
    value.Fields["user_qq"]->Condition = SQL_MODIFY;
    ret = pClient->Exec(test.Modify(value));
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    Result result;
    ret = pClient->Exec(test.Query(), result, test);
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    ret = pClient->Exec(test.Drop());
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    ret = pClient->Close();
    printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    //getchar();
    return 0;
}

int mysql_test()
{
    return 0;
}

int main()
{
    int ret = 0;
    //int ret = http_test();
    ret = sql_test();
    //ret = mysql_test();
    printf("main:ret = %d\n", ret);
    return ret;
}