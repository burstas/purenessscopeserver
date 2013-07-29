/*
 * ���Է���������Ϣ����
 * by w1w
 */
#include "PostServerCommand.h"
#include "ServerConfig.h"

CPostServerCommand::CPostServerCommand()
{
	m_pServerObject = NULL;
}

CPostServerCommand::~CPostServerCommand()
{
}

void CPostServerCommand::SetServerObject(CServerObject* pServerObject)
{
	m_pServerObject = pServerObject;
}

bool CPostServerCommand::RecvData(IClientParse* pClientParse)
{
	if(pClientParse == NULL)
	{
		OUR_DEBUG((LM_ERROR, "[CPostServerCommand::RecvData] pClientParse is NULL.\n"));
		return false;
	}
	if(pClientParse->GetPacketCommandID() == COMMAND_KEEP_ALIVE_RESPONSE)
	{
		//OUR_DEBUG((LM_ERROR, "[CPostServerCommand::RecvData] ������Ӧ��.\n"));
		return true;
	}
	if(pClientParse->GetPacketCommandID() == COMMAND_QUERY_APP_RESPONSE)
	{
		OUR_DEBUG((LM_ERROR, "[CPostServerCommand::RecvData] ����APP.\n"));
		IBuffPacket* pBodyPacket = m_pServerObject->GetPacketManager()->Create();
		if(NULL == pBodyPacket)
		{
			return false;
		}
		//�õ����ݰ��İ���
		_PacketInfo BodyPacket;
		BodyPacket.m_pData = pClientParse->GetMessageBody()->rd_ptr();
		BodyPacket.m_nDataLen = (int)pClientParse->GetMessageBody()->length();

		//������󶨸�pBodyPacket��
		pBodyPacket->WriteStream(BodyPacket.m_pData, BodyPacket.m_nDataLen);

		uint16     u2CommandID  = 0;
		uint16     ServerID  = 0;
		uint16 u2AppCount = 0;
		uint16 Appid = 0;
		uint8 isadd = 0;

		(*pBodyPacket) >> u2CommandID;   //�����������u2CommandID��ֵ
		(*pBodyPacket) >> ServerID;   
		(*pBodyPacket) >> u2AppCount; 
		icServerConfig::instance()->m_mapServerInfo[ServerID].m_mapAppInfo.clear();
		for (int i = 0; i<u2AppCount; ++i)
		{
			(*pBodyPacket) >> Appid;   
			(*pBodyPacket) >> isadd;
			_icAppInfo appinfo;
			appinfo.m_Appid = Appid;
			icServerConfig::instance()->m_mapServerInfo[ServerID].m_mapAppInfo[Appid]=appinfo;
		}
		if (u2CommandID == COMMAND_QUERY_APP_RESPONSE) icServerConfig::instance()->m_mapServerInfo[ServerID].m_NeedRefeshApp = false;

		//���л����������ˣ��黹������
		m_pServerObject->GetPacketManager()->Delete(pBodyPacket);
		return true;
	}
	if(pClientParse->GetPacketCommandID() == COMMAND_SYN_APP)
	{
		OUR_DEBUG((LM_ERROR, "[CPostServerCommand::RecvData] ͬ��APP.\n"));
		return true;
	}

	OUR_DEBUG((LM_INFO, "[CPostServerCommand::RecvData][δ����������]%d",pClientParse->GetPacketCommandID()));
	return true;
}

bool CPostServerCommand::ConnectError(int nError)
	{
		OUR_DEBUG((LM_ERROR, "[CPostServerCommand::ConnectError]���մ���(%d).\n", nError));
		return true;
	}