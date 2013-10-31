#include "UserValidManager.h"

#include "ace/OS_main.h"
#include "ace/Reactor.h"
#include "ace/SOCK_Connector.h" 
#include "ace/SOCK_Acceptor.h" 
#include "ace/Auto_Ptr.h"
#include "ace/Singleton.h"
#include "ace/Thread.h"

#define SERVER_IP     "127.0.0.1"
#define SERVER_PORT   10005

typedef ACE_Singleton<CUserValidManager, ACE_Null_Mutex> App_UserValidManager;

class CClientService : public ACE_Event_Handler
{
public:
	ACE_SOCK_Stream &peer (void) { return this->m_sckClient; }

	int open (void)
	{
		//注册读就绪回调函数
		return this->reactor ()->register_handler(this, ACE_Event_Handler::READ_MASK);
	}

	virtual ACE_HANDLE get_handle (void) const 
	{ 
		return this->m_sckClient.get_handle(); 
	}

	virtual int handle_input (ACE_HANDLE fd )
	{
		if(fd == ACE_INVALID_HANDLE)
		{
			OUR_DEBUG((LM_ERROR, "[handle_input]fd is ACE_INVALID_HANDLE.\n"));
			return -1;
		}
		
		ACE_Time_Value nowait(0, MAX_QUEUE_TIMEOUT);

		//处理接收逻辑
		//接收字节，先接收4字节包长度，然后接收用户名字符串
		char szPacketSize[4] = {'\0'};
		int nDataLen = this->peer().recv(szPacketSize, 4, MSG_NOSIGNAL, &nowait);
		if(nDataLen != 4 && nDataLen <= 0)
		{
			return -1;
		}

		int nPacketSize = 0;
		ACE_OS::memcpy(&nPacketSize, szPacketSize, 4);

		char* pBuff = new char[nPacketSize];
		ACE_OS::memset(pBuff, 0, nPacketSize);

		//因为是内网程序，目前不考虑分包和粘包，这里在大流量下可以优化的，先以完成功能为主。
		//组包规则，2字节用户名长度+用户名+4字节ConnectID
		nDataLen = this->peer().recv(pBuff, nPacketSize, MSG_NOSIGNAL, &nowait);
		if(nDataLen != nPacketSize && nDataLen <= 0)
		{
			SAFE_DELETE_ARRAY(pBuff);
			return -1;
		}

		//解析数据
		int nRecvPos      = 0;
		int nUserNameSize = 0;
		int nUserPassSize = 0;
		int nConnectID    = 0;

		ACE_OS::memcpy((char* )&nUserNameSize, (char* )&pBuff[nRecvPos], 2);
		nRecvPos += 2;
		char* pUserName = new char[nUserNameSize + 1];
		ACE_OS::memset(pUserName, 0, nUserNameSize + 1);
		ACE_OS::memcpy((char* )pUserName, (char* )&pBuff[nRecvPos], nUserNameSize);
		nRecvPos += nUserNameSize;

		ACE_OS::memcpy((char* )&nUserPassSize, (char* )&pBuff[nRecvPos], 2);
		nRecvPos += 2;
		char* pUserPass = new char[nUserPassSize + 1];
		ACE_OS::memset(pUserPass, 0, nUserPassSize + 1);
		ACE_OS::memcpy((char* )pUserPass, (char* )&pBuff[nRecvPos], nUserPassSize);
		nRecvPos += nUserPassSize;

		ACE_OS::memcpy((char* )&nConnectID, (char* )&pBuff[nRecvPos], 4);
		nRecvPos += 4;

		int nSendSize = 4 + 2 + nUserNameSize + 2 + nUserPassSize + 1 + 4;
		char* pSend = new char[nSendSize];
		int nSendPos = 0;

		//处理接收数据
		bool blState = App_UserValidManager::instance()->Load_File(pUserName);
		if(blState == false)
		{
			//没有找到这个用户数据，组成返回包
			int nSendPacketSize = 2 + nUserNameSize + 2 + nUserPassSize + 1 + 4;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )&nSendPacketSize, 4);
			nSendPos += 4;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )&nUserNameSize, 2);
			nSendPos += 2;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )pUserName, nUserNameSize);
			nSendPos += nUserNameSize;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )&nUserPassSize, 2);
			nSendPos += 2;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )pUserPass, nUserPassSize);
			nSendPos += nUserPassSize;
			int nRet = 1;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )&nRet, 1);
			nSendPos += 1;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )&nConnectID, 4);
			nSendPos += 4;
		}
		else
		{
			//找到了这个用户数据，组成返回包
			int nSendPacketSize = 2 + nUserNameSize + 2 + nUserPassSize + 1 + 4;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )&nSendPacketSize, 4);
			nSendPos += 4;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )&nUserNameSize, 2);
			nSendPos += 2;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )pUserName, nUserNameSize);
			nSendPos += nUserNameSize;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )&nUserPassSize, 2);
			nSendPos += 2;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )pUserPass, nUserPassSize);
			nSendPos += nUserPassSize;
			int nRet = 0;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )&nRet, 1);
			nSendPos += 1;
			ACE_OS::memcpy((char* )&pSend[nSendPos], (char* )&nConnectID, 4);
			nSendPos += 4;
		}

		//发送返回数据
		this->peer().send(pSend, nSendSize, &nowait);

		SAFE_DELETE_ARRAY(pUserPass);
		SAFE_DELETE_ARRAY(pUserName);
		SAFE_DELETE_ARRAY(pSend);
		SAFE_DELETE_ARRAY(pBuff);
		return 0;
	}

	// 释放相应资源
	virtual int handle_close (ACE_HANDLE, ACE_Reactor_Mask mask)
	{
		if (mask == ACE_Event_Handler::WRITE_MASK)
		{
			return 0;
		}

		mask = ACE_Event_Handler::ALL_EVENTS_MASK |
			ACE_Event_Handler::DONT_CALL;
		this->reactor ()->remove_handler (this, mask);
		this->m_sckClient.close ();
		delete this;    //socket出错时，将自动删除该客户端，释放相应资源
		return 0;
	}

protected:
	ACE_SOCK_Stream m_sckClient;
};

class CServerAcceptor : public ACE_Event_Handler
{
public:
	virtual ~CServerAcceptor (){this->handle_close (ACE_INVALID_HANDLE, 0);}

	int open (const ACE_INET_Addr &listen_addr)
	{
		if (this->m_objAcceptor.open (listen_addr, 1) == -1)
		{
			OUR_DEBUG((LM_INFO, "open port fail.\n"));
			return -1;
		}

		OUR_DEBUG((LM_INFO, "Begin Listen....\n"));
		//注册接受连接回调事件
		return this->reactor ()->register_handler(this, ACE_Event_Handler::ACCEPT_MASK);
	}

	virtual ACE_HANDLE get_handle (void) const
	{ 
		return this->m_objAcceptor.get_handle (); 
	}

	virtual int handle_input (ACE_HANDLE fd )
	{
		if(fd == ACE_INVALID_HANDLE)
		{
			return -1;
		}		
		
		CClientService *client = new CClientService();
		auto_ptr<CClientService> p(client);

		if (this->m_objAcceptor.accept (client->peer ()) == -1)
		{
			OUR_DEBUG((LM_INFO, "accept client fail.\n"));
			return -1;
		}
		p.release ();
		client->reactor (this->reactor ());
		if (client->open () == -1)
		{
			client->handle_close (ACE_INVALID_HANDLE, 0);
		}
		return 0;
	}

	virtual int handle_close (ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
	{
		OUR_DEBUG((LM_ERROR, "[handle_close]close_mask=%d.\n", (int)close_mask));
		if (handle != ACE_INVALID_HANDLE)
		{
			ACE_Reactor_Mask m = ACE_Event_Handler::ACCEPT_MASK |
				ACE_Event_Handler::DONT_CALL;
			this->reactor ()->remove_handler (this, m);
			this->m_objAcceptor.close ();
		}
		return 0;
	}

protected:
	ACE_SOCK_Acceptor m_objAcceptor;
};

void* worker(void *arg) 
{
	if(NULL != arg)
	{
		OUR_DEBUG((LM_INFO, "[worker]have param.\n"));
	}
	
	while(true)
	{
		OUR_DEBUG((LM_INFO, "[Watch]Valid Begin.\n"));
		App_UserValidManager::instance()->Check_File2Memory();
		OUR_DEBUG((LM_INFO, "[Watch]Valid End.\n"));
		ACE_Time_Value tvSleep(60, 0);
		ACE_OS::sleep(tvSleep);
	}

	return NULL; 
} 


int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	if(argc > 0)
	{
		OUR_DEBUG((LM_INFO, "[main]argc = %d.\n", argc));
		for(int i = 0; i < argc; i++)
		{
			OUR_DEBUG((LM_INFO, "[main]argc(%d) = %s.\n", argc, argv[i]));
		}
	}	
	
	ACE_thread_t  threadId;
	ACE_hthread_t threadHandle;

	//初始化共享内存
	App_UserValidManager::instance()->Init();

	//首先创建工作线程
	ACE_Thread::spawn(
		(ACE_THR_FUNC)worker,        //线程执行函数
		NULL,                        //执行函数参数
		THR_JOINABLE | THR_NEW_LWP,
		&threadId,
		&threadHandle
		);

	//然后打开监听
	ACE_INET_Addr addr(SERVER_PORT, SERVER_IP);
	CServerAcceptor server;
	server.reactor(ACE_Reactor::instance());
	server.open(addr);

	while(true)
	{
		ACE_Reactor::instance()->handle_events(); 
	}

	return 0;
}
