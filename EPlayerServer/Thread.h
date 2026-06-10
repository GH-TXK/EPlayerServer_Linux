#pragma once
#include "Function.h"
#include <pthread.h>
#include <map>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <cstdio>

class CThread
{
public:
	CThread()
	{
		m_function = NULL;
		m_thread = 0;
		m_bpaused = false;
	}
	template<typename _FUNCTION_, typename... _ARGS_>
	CThread(_FUNCTION_ func, _ARGS_... args)
		:m_function(new CFunction<_FUNCTION_, _ARGS_...>(func, args...))
	{
		m_thread = 0;
		m_bpaused = false;
	}

	~CThread() {}
public:
	CThread(const CThread&) = delete;
	CThread operator=(const CThread&) = delete;
public:
	template<typename _FUNCTION_, typename... _ARGS_>
	int SetThreadFunc(_FUNCTION_ func, _ARGS_... args)
	{
		m_function = new CFunction<_FUNCTION_, _ARGS_...>(func, args...);
		if (m_function == NULL)return -1;
		return 0;
	}
	int Start()
	{
		pthread_attr_t attr;
		int ret = 0;
		ret = pthread_attr_init(&attr);
		if (ret != 0)return -1;
		ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		if (ret != 0)return -2;
		ret = pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
		if (ret != 0)return -3;
		ret = pthread_create(&m_thread, &attr, &CThread::ThreadEntry, this);
		if (ret != 0)return -4;
		m_mapThread[m_thread] = this;
		ret = pthread_attr_destroy(&attr);
		if (ret != 0)return -5;
		return 0;
	}
	int Pause() {
		if (m_thread != 0)return -1;
		if (m_bpaused)
		{
			m_bpaused = false;
			return 0;
		}
		m_bpaused = true;
		int ret = pthread_kill(m_thread, SIGUSR1);
		if (ret != 0) {
			m_bpaused = false;
			return -2;
		}
		return 0;
	}
	int Stop() {
		if (m_thread != 0)
		{
			pthread_t thread = m_thread;
			m_thread = 0;
			timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 100 * 1000000;//100ms
			int ret = pthread_timedjoin_np(thread, NULL, &ts);
			if (ret == ETIMEDOUT)
			{
				pthread_detach(thread);
				pthread_kill(thread, SIGUSR2);
			}
		}
		return 0;
	}
	bool isValid()const { return m_thread == 0; }
private:
	static void* ThreadEntry(void* arg)
	{
		CThread* thiz = (CThread*)arg;
		struct sigaction act = { 0 };
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_SIGINFO;
		act.sa_sigaction = &CThread::Sigaction;
		sigaction(SIGUSR1, &act, NULL);
		sigaction(SIGUSR2, &act, NULL);
		thiz->EnterThread();

		if (thiz->m_thread)thiz->m_thread = 0;
		pthread_t thread = pthread_self();//不是冗余， 有可能被stop函数把m_thread给清零了
		auto it = m_mapThread.find(thread);
		if (it != m_mapThread.end())
			m_mapThread[thread] = NULL;
		pthread_detach(thread);
		pthread_exit(NULL);
	}
	static void Sigaction(int signo, siginfo_t* info, void* context)
	{
		if (signo == SIGUSR1)
		{
			pthread_t thread = pthread_self();
			auto it = m_mapThread.find(thread);
			if (it != m_mapThread.end())
			{
				if (it->second) {
					while (it->second->m_bpaused) {
						if (it->second->m_thread == 0) {
							pthread_exit(NULL);
						}
					}
					usleep(1000);//1ms
				}
			}
		}
		else if (signo == SIGUSR2) {
			pthread_exit(NULL);
		}
	}

	void EnterThread() {
		if (m_function != NULL)
		{
			int ret = (*m_function)();
			if (ret != 0) {
				printf("%s(%d):[%s]ret = %d\n", __FILE__, __LINE__, __FUNCTION__);
			}
		}
	}
private:
	CFunctionBase* m_function;
	pthread_t m_thread;
	bool m_bpaused;//true表示暂停false表示运行中
	static std::map<pthread_t, CThread*> m_mapThread;
};

