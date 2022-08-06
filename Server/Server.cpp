#include <thread>
//#include <chrono>

#include "NetLib/ServerNetErrorCode.h"
#include "NetLib/Define.h"
#include "NetLib/TcpNetwork.h"
#include "ConsoleLogger.h"
#include "PacketProcess.h"
#include "UserManager.h"
#include "RoomManager.h"

#include "Server.h"

// CPP: typedef and using is same except the order and equal sign.
typedef NServerNetLib::NET_ERROR_CODE	NET_ERROR_CODE;
typedef NServerNetLib::LOG_TYPE			LOG_TYPE;

Server::Server()
{
}

Server::~Server()
{
	Release();
}

ERROR_CODE Server::Init()
{
	m_pLogger = std::make_unique<ConsoleLog>();

	// sets m_pServerConfig
	LoadConfig();

	// CPP: assign address of new TcpNetwork object to pointer to ITcpNetwork
	m_pNetwork = std::make_unique<NServerNetLib::TcpNetwork>();
	auto result = m_pNetwork->Init(m_pServerConfig.get(), m_pLogger.get());
	
	if (result != NET_ERROR_CODE::NONE)
	{
		m_pLogger->Write(LOG_TYPE::L_ERROR, "%s | Init Fail. NetErrorCode(%s)",
			__FUNCTION__, (short)result);
		return ERROR_CODE::MAIN_INIT_NETWORK_INIT_FAIL;
	}

	m_pUserMgr = std::make_unique<UserManager>();
	m_pUserMgr->Init(m_pServerConfig->MaxClientCount);

	// CPP: get() of unique pointer returns the pointer it owns.
	m_pRoomMgr = std::make_unique<RoomManager>();
	m_pRoomMgr->Init(m_pServerConfig->MaxRoomCount, m_pServerConfig->MaxRoomUserCount);
	m_pRoomMgr->SetNetwork(m_pNetwork.get(), m_pLogger.get());

	m_pPacketProc = std::make_unique<PacketProcess>();
	m_pPacketProc->Init(m_pNetwork.get(), m_pUserMgr.get(), m_pRoomMgr.get(), m_pServerConfig.get(), m_pLogger.get());

	m_IsRun = true;
	m_pLogger->Write(LOG_TYPE::L_INFO, "%s | Init Success. Server Run", __FUNCTION__);
	return ERROR_CODE::NONE;
}

void Server::Release()
{
	if (m_pNetwork)
		m_pNetwork->Release();
}

void Server::Stop()
{
	m_IsRun = false;
}

void Server::Run()
{
	while (m_IsRun)
	{
		// ?: what it does?
		m_pNetwork->Run();
		
		while (true)
		{
			NServerNetLib::RecvPacketInfo	packetInfo = m_pNetwork->GetPacketInfo();

			if (packetInfo.PacketId == 0)
				break;
			else
				m_pPacketProc->Process(&packetInfo);
		}
	}
}

ERROR_CODE Server::LoadConfig()
{
	m_pServerConfig = std::make_unique<NServerNetLib::ServerConfig>();

	// todo: parse json config
	m_pServerConfig->Port = 11021;
	m_pServerConfig->BackLogCount = 128;
	m_pServerConfig->MaxClientCount = 1000;

	// ?: SockOpt = Socket Option?
	m_pServerConfig->MaxClientSockOptRecvBufferSize = 10240; // 10k
	m_pServerConfig->MaxClientSockOptSendBufferSize = 10240;
	m_pServerConfig->MaxClientRecvBufferSize = 8192;
	m_pServerConfig->MaxClientSendBufferSize = 8192;

	m_pServerConfig->IsLoginCheck = false;

	m_pServerConfig->ExtraClientCount = 64;
	m_pServerConfig->MaxRoomCount = 20;
	m_pServerConfig->MaxRoomUserCount = 4;

	m_pLogger->Write(LOG_TYPE::L_INFO, "%s | Port(%d), Backlog(%d)",\
		__FUNCTION__, m_pServerConfig->Port, m_pServerConfig->BackLogCount);
	m_pLogger->Write(LOG_TYPE::L_INFO, "%s | IsLoginCheck(%d)",\
		__FUNCTION__, m_pServerConfig->IsLoginCheck);
		return ERROR_CODE::NONE;
}
