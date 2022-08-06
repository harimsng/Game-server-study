#pragma once

#include "Packet.h"
#include "ErrorCode.h"
#include "NetLib/Define.h"


namespace NServerNetLib
{
	class ITcpNetwork;
	class ILog;
}


class UserManager;
class RoomManager;


class PacketProcess
{
	typedef NServerNetLib::RecvPacketInfo	PacketInfo;
	typedef NServerNetLib::ITcpNetwork		TcpNet;
	typedef NServerNetLib::ILog				ILog;

// member functions
public:
	PacketProcess();
	~PacketProcess();

	void Init(TcpNet* pNetwork, UserManager* pUserMgr, RoomManager* pRoomMgr,
		NServerNetLib::ServerConfig* pConfig, ILog* pLogger);
	void Process(PacketInfo *packetInfo);

private:
	ERROR_CODE NtfSysConnectSession(const PacketInfo *packetInfo);
	ERROR_CODE NtfSysCloseSession(PacketInfo *packetInfo);

	ERROR_CODE Login(PacketInfo *packetInfo);
	ERROR_CODE EnterRoom(PacketInfo* packetInfo);
	ERROR_CODE LeaveRoom(PacketInfo* packetInfo);
	ERROR_CODE RoomChat(PacketInfo* packetInfo);

// member variables
private:
	ILog *m_pRefLogger;
	TcpNet *m_pRefNetwork;

	UserManager *m_pRefUserMgr;
	RoomManager* m_pRefRoomMgr;
};
