#ifndef _CLIENTCONNECTMANAGER_H
#define _CLIENTCONNECTMANAGER_H

#include "ace/Connector.h"
#include "ace/SOCK_Connector.h"

#include "TimerManager.h"
#include "IClientManager.h"
#include "ConnectClient.h"
#include "ReactorUDPClient.h"

#include <map>

using namespace std;

typedef ACE_Connector<CConnectClient, ACE_SOCK_CONNECTOR> ReactorConnect;

class CReactorClientInfo
{
public:
	CReactorClientInfo();
	~CReactorClientInfo();

	bool Init(int nServerID, const char* pIP, int nPort, ReactorConnect* pReactorConnect, IClientMessage* pClientMessage, ACE_Reactor* pReactor);  //初始化链接地址和端口
	bool Run();                                //开始链接
	bool SendData(ACE_Message_Block* pmblk);                        //发送数据
	bool ConnectError(int nError);                                  //链接错误，报错
	int  GetServerID();                                             //得到服务器ID
	bool Close();                                                   //关闭服务器链接
	void SetConnectClient(CConnectClient* pConnectClient);          //设置链接状态
	CConnectClient* GetConnectClient();                             //得到ProConnectClient指针
	IClientMessage* GetClientMessage();                             //获得当前的消息处理指针

private:
	ACE_INET_Addr      m_AddrServer;             //远程服务器的地址
	CConnectClient*    m_pConnectClient;         //当前链接对象
	ReactorConnect*    m_pReactorConnect;        //Connector链接对象
	IClientMessage*    m_pClientMessage;         //回调函数类，回调返回错误和返回数据方法
	int                m_nServerID;              //远程服务器的ID
	ACE_Reactor*       m_pReactor;               //记录使用的反应器
};

class CClientReConnectManager : public ACE_Event_Handler, public IClientManager
{
public:
	CClientReConnectManager(void);
	~CClientReConnectManager(void);

public:
	bool Init(ACE_Reactor* pReactor);
	bool Connect(int nServerID, const char* pIP, int nPort, IClientMessage* pClientMessage);           //链接服务器(TCP)
	bool ConnectUDP(int nServerID, const char* pIP, int nPort, IClientUDPMessage* pClientUDPMessage);  //建立一个指向UDP的链接（UDP）
	bool Close(int nServerID);                                                                         //关闭连接
	bool CloseUDP(int nServerID);                                                                      //关闭链接（UDP）
	bool ConnectErrorClose(int nServerID);                                                             //由内部错误引起的失败，由ProConnectClient调用
	bool SendData(int nServerID, const char* pData, int nSize);                                        //发送数据
	bool SendDataUDP(int nServerID,const char* pIP, int nPort, const char* pMessage, uint32 u4Len);    //发送数据（UDP）
	bool SetHandler(int nServerID, CConnectClient* pConnectClient);                                    //将指定的CProConnectClient*绑定给nServerID
	IClientMessage* GetClientMessage(int nServerID);                                                   //获得ClientMessage对象
	bool StartConnectTask(int nIntervalTime = CONNECT_LIMIT_RETRY);                                    //设置自动重连的定时器
	void CancelConnectTask();                                                                          //关闭重连定时器
	void Close();

	void GetConnectInfo(vecClientConnectInfo& VecClientConnectInfo);      //返回当前存活链接的信息（TCP）
	void GetUDPConnectInfo(vecClientConnectInfo& VecClientConnectInfo);   //返回当前存活链接的信息（UDP）

	virtual int handle_timeout (const ACE_Time_Value &current_time, const void *act = 0);              //定时器执行

private:
	typedef map<int, CReactorClientInfo*> mapReactorConnectInfo;
	typedef map<int, CReactorUDPClient*>  mapReactorUDPConnectInfo;

public:
	mapReactorConnectInfo       m_mapConnectInfo;
	mapReactorUDPConnectInfo    m_mapReactorUDPConnectInfo;
	ReactorConnect              m_ReactorConnect;
	ACE_Recursive_Thread_Mutex  m_ThreadWritrLock;             //线程锁
	ActiveTimer                 m_ActiveTimer;                 //时间管理器
	int                         m_nTaskID;                     //定时检测工具
	ACE_Reactor*                m_pReactor;                    //当前的反应器
};

typedef ACE_Singleton<CClientReConnectManager, ACE_Recursive_Thread_Mutex> App_ClientReConnectManager;
#endif
