#include <cstring>

#include "NetLib/ILog.h"
#include "NetLib/TcpNetwork.h"
#include "User.h"
#include "UserManager.h"
#include "Room.h"
#include "RoomManager.h"
#include "PacketProcess.h"

typedef NServerNetLib::LOG_TYPE		LOG_TYPE;
typedef NServerNetLib::ServerConfig	ServerConfig;

PacketProcess::PacketProcess()
{
}

PacketProcess::~PacketProcess()
{
}

void PacketProcess::Init(TcpNet* pNetwork,
	UserManager* pUserMgr,
	RoomManager* pRoomMgr,
	NServerNetLib::ServerConfig* pConfig,
	ILog* pLogger)
{
	m_pRefLogger = pLogger;
	m_pRefNetwork = pNetwork;
	m_pRefUserMgr = pUserMgr;
	m_pRefRoomMgr = pRoomMgr;
	m_pRefLogger = pLogger;
}

void PacketProcess::Process(PacketInfo *packetInfo)
{
	typedef NServerNetLib::PACKET_ID	netLibPacketId;
	typedef PACKET_ID					commonPacketId;

	auto packetID = packetInfo->PacketId;

	switch (packetID)
	{
	case (int)netLibPacketId::NTF_SYS_CONNECT_SESSION:
		NtfSysConnectSession(packetInfo);
		break;
	case (int)netLibPacketId::NTF_SYS_CLOSE_SESSION:
		NtfSysCloseSession(packetInfo);
		break;
	case (int)commonPacketId::LOGIN_IN_REQ:
		Login(packetInfo);
		break;
	case (int)commonPacketId::ROOM_ENTER_REQ:
		EnterRoom(packetInfo);
		break;
	case (int)commonPacketId::ROOM_LEAVE_REQ:
		LeaveRoom(packetInfo);
		break;
	case (int)commonPacketId::ROOM_CHAT_REQ:
		RoomChat(packetInfo);
		break;
		// TODO: fill the rest of packet type
	}
}

ERROR_CODE PacketProcess::NtfSysConnectSession(const PacketInfo *packetInfo)
{
	m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | sessionIndex(%d)",
		__FUNCTION__, packetInfo->SessionIndex);
	return ERROR_CODE::NONE;
}

ERROR_CODE PacketProcess::NtfSysCloseSession(PacketInfo *packetInfo)
{
	auto pUser = std::get<1>(m_pRefUserMgr->GetUser(packetInfo->SessionIndex));

	if (pUser)
		m_pRefUserMgr->RemoveUser(packetInfo->SessionIndex);

	m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | sessionIndex(%d)",
		__FUNCTION__, packetInfo->SessionIndex);
	return ERROR_CODE::NONE;
}

// TODO: password
// duplicate ID is error
ERROR_CODE PacketProcess::Login(PacketInfo *packetInfo)
{
	PktLogInRes resPkt;
	PktLogInReq *reqPkt = (PktLogInReq *)packetInfo->pRefData;

	auto addRet = m_pRefUserMgr->AddUser(packetInfo->SessionIndex, reqPkt->szID);

	m_pRefLogger->Write(LOG_TYPE::L_INFO,
		"%s | Request. sessionIndex(%d)",
		__FUNCTION__, packetInfo->SessionIndex);

	// duplicate ID
	if (addRet != ERROR_CODE::NONE)
	{
		resPkt.SetError(addRet);
		m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::LOGIN_IN_RES,
			sizeof(PktLogInRes), (char*)&resPkt);
		m_pRefLogger->Write(LOG_TYPE::L_ERROR,
			"%s | Fail. sessionIndex(%d), szID(%s), errorCode(%hd)",
			__FUNCTION__, packetInfo->SessionIndex, reqPkt->szID, addRet);
		return addRet;
	}

	// login success
	resPkt.SetError(addRet);
	m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::LOGIN_IN_RES,
		sizeof(PktLogInRes), (char*)&resPkt);
	m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Success. sessionIndex(%d), szID(%s)",
		__FUNCTION__, packetInfo->SessionIndex, reqPkt->szID);
	return ERROR_CODE::NONE;
}

/*
if sessionIndex is invalid, send fail response.
if existing user attempts to enter, send fail response.
if room index is -1 set roomNumber to MaxRoomCount().
if roomNumber is out of range, send fail response.
if the room doesn't have empty slot, send fail response.
else send success response.
*/
ERROR_CODE PacketProcess::EnterRoom(PacketInfo* packetInfo)
{
	PktRoomEnterRes resPkt;
	PktRoomEnterReq reqPkt;
	Room* targetRoom;

	memcpy(&reqPkt, packetInfo->pRefData, packetInfo->PacketBodySize);

	m_pRefLogger->Write(LOG_TYPE::L_INFO,
		"%s | Request. sessionIndex(%d)", __FUNCTION__, packetInfo->SessionIndex);

	// check sessionIndex
	auto newUserInfo = m_pRefUserMgr->GetUser(packetInfo->SessionIndex);
	if (std::get<0>(newUserInfo) != ERROR_CODE::NONE)
	{
		resPkt.SetError(std::get<0>(newUserInfo));
		resPkt.userUniqueId = -1;
		m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES,
			sizeof(PktRoomEnterRes), (char*)&resPkt);
		m_pRefLogger->Write(LOG_TYPE::L_ERROR,
			"%s | Invalid sessionIndex. sessionIndex(%d), errorCode(%hd)",
			__FUNCTION__, packetInfo->SessionIndex, resPkt.GetError());
		return std::get<0>(newUserInfo);
	}
	resPkt.userUniqueId = std::get<1>(newUserInfo)->GetIndex();

	// check whether valid user
	if (std::get<1>(newUserInfo)->IsCurDomainInRoom())
	{
		resPkt.SetError(ERROR_CODE::ROOM_MGR_INVALID_NEW_USER);
		m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES,
			sizeof(PktRoomEnterRes), (char*)&resPkt);
		m_pRefLogger->Write(LOG_TYPE::L_ERROR,
			"%s | Invaild user. sessionIndex(%d), roomNumber(%d), errorCode(%hd)",
			__FUNCTION__, packetInfo->SessionIndex, reqPkt.roomNumber, resPkt.GetError());
		return ERROR_CODE::ROOM_MGR_INVALID_NEW_USER;
	}

	// get roomNumber
	if (reqPkt.roomNumber == -1)
	{
		Room* roomPtr;

		for (int i = 0; ((roomPtr = m_pRefRoomMgr->GetRoom(i)) != nullptr); ++i)
		{
			if (roomPtr->GetUserCount() == 0)
			{
				reqPkt.roomNumber = i;
				break;
			}
		}
		if (roomPtr == nullptr)
		{
			resPkt.SetError(ERROR_CODE::ROOM_MGR_NO_EMPTY_ROOM);
			m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES,
				sizeof(PktRoomEnterRes), (char*)&resPkt);
			m_pRefLogger->Write(LOG_TYPE::L_ERROR,
				"%s | Invaild user. sessionIndex(%d), errorCode(%hd)",
				__FUNCTION__, packetInfo->SessionIndex, resPkt.GetError());
			return ERROR_CODE::ROOM_MGR_NO_EMPTY_ROOM;
		}
	}

	targetRoom = m_pRefRoomMgr->GetRoom(reqPkt.roomNumber);

	// roomNumber is out of range
	if (targetRoom == nullptr)
	{
		resPkt.SetError(ERROR_CODE::ROOM_MGR_INVALID_ROOM_NUMBER);
		m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES,
			sizeof(PktRoomEnterRes), (char*)&resPkt);
		m_pRefLogger->Write(LOG_TYPE::L_ERROR,
			"%s | Invalid roomNumber. sessionIndex(%d), roomNumber(%d), errorCode(%hd)",
			__FUNCTION__, packetInfo->SessionIndex, reqPkt.roomNumber, resPkt.GetError());
		return ERROR_CODE::ROOM_MGR_INVALID_ROOM_NUMBER;
	}

	// room is full
	if (targetRoom->GetUserCount() == targetRoom->GetMaxUserCount())
	{
		resPkt.SetError(ERROR_CODE::ROOM_MGR_ROOM_IS_FULL);
		m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES,
			sizeof(PktRoomEnterRes), (char*)&resPkt);
		m_pRefLogger->Write(LOG_TYPE::L_ERROR,
			"%s | Room is full. sessionIndex(%d), roomNumber(%d), errorCode(%hd)",
			__FUNCTION__, packetInfo->SessionIndex, reqPkt.roomNumber, resPkt.GetError());
		return ERROR_CODE::ROOM_MGR_ROOM_IS_FULL;
	}

	// room enter success
	resPkt.SetError(ERROR_CODE::NONE);
	targetRoom->AddNewUser(std::get<1>(newUserInfo));
	m_pRefNetwork->SendData(packetInfo->SessionIndex,
		(short)PACKET_ID::ROOM_ENTER_RES,
		sizeof(PktRoomEnterRes), (char*)&resPkt);
	m_pRefLogger->Write(LOG_TYPE::L_INFO,
		"%s | Success. sessionIndex(%d), roomNumber(%d)",
		__FUNCTION__, packetInfo->SessionIndex, reqPkt.roomNumber);
	return ERROR_CODE::NONE;
}

ERROR_CODE PacketProcess::LeaveRoom(PacketInfo* packetInfo)
{
	PktRoomLeaveReq	*reqPkt = (PktRoomLeaveReq *)packetInfo->pRefData;
	PktRoomLeaveRes resPkt; 
	auto			leaveUserInfo = m_pRefUserMgr->GetUser(packetInfo->SessionIndex);
	User*			leaveUser = std::get<1>(leaveUserInfo);

	m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Request. sessionIndex(%d)",
		__FUNCTION__, packetInfo->SessionIndex);

	// check sessionIndex
	if (std::get<0>(leaveUserInfo) != ERROR_CODE::NONE)
	{
		resPkt.SetError(std::get<0>(leaveUserInfo));
		m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES,
			sizeof(PktRoomLeaveRes), (char*)&resPkt);
		m_pRefLogger->Write(LOG_TYPE::L_ERROR,
			"%s | Invalid sessionIndex. sessionIndex(%d), errorCode(%hd)",
			__FUNCTION__, packetInfo->SessionIndex, resPkt.GetError());
		return std::get<0>(leaveUserInfo);
	}

	// check whether valid user
	if (std::get<1>(leaveUserInfo)->IsCurDomainInRoom() == false)
	{
		resPkt.SetError(ERROR_CODE::ROOM_MGR_INVALID_LEAVE_USER);
		m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES,
			sizeof(PktRoomLeaveRes), (char*)&resPkt);
		m_pRefLogger->Write(LOG_TYPE::L_ERROR,
			"%s | Invalid user. sessionIndex(%d), errorCode(%hd)",
			__FUNCTION__, packetInfo->SessionIndex, resPkt.GetError());
		return ERROR_CODE::ROOM_MGR_INVALID_LEAVE_USER;
	}

	Room* roomPtr = m_pRefRoomMgr->GetRoom(leaveUser->GetRoomIndex());

	// TODO: null check
	// remove user from the room and notify that.
	roomPtr->RemoveUser(leaveUser->GetIndex());
	leaveUser->LeaveRoom();

	resPkt.SetError(ERROR_CODE::NONE);
	m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Success. sessionIndex(%d)",
		__FUNCTION__, packetInfo->SessionIndex);
	m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES,
		sizeof(resPkt), (char *)&resPkt);
	return ERROR_CODE::NONE;
}

ERROR_CODE PacketProcess::RoomChat(PacketInfo* packetInfo)
{
	PktRoomChatReq	*reqPkt = (PktRoomChatReq *)packetInfo->pRefData;
	PktRoomChatRes	resPkt; 
	auto			userInfo = m_pRefUserMgr->GetUser(packetInfo->SessionIndex);
	User*			user = std::get<1>(userInfo);

	m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Request. sessionIndex(%d)",
		__FUNCTION__, packetInfo->SessionIndex);

	// check sessionIndex
	if (std::get<0>(userInfo) != ERROR_CODE::NONE)
	{
		resPkt.SetError(std::get<0>(userInfo));
		m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES,
			sizeof(PktRoomChatRes), (char*)&resPkt);
		m_pRefLogger->Write(LOG_TYPE::L_ERROR,
			"%s | Invalid sessionIndex. sessionIndex(%d), errorCode(%hd)",
			__FUNCTION__, packetInfo->SessionIndex, resPkt.GetError());
		return std::get<0>(userInfo);
	}

	Room*		roomPtr = m_pRefRoomMgr->GetRoom(user->GetRoomIndex());
	MSG_TYPE*	msgPtr = reqPkt->msg;
	short		msgLen = reqPkt->msgLen;

	// check whether valid user and chat message length
	if (std::get<1>(userInfo)->IsCurDomainInRoom() == false
		|| msgLen >= MAX_ROOM_CHAT_MSG_SIZE)
	{
		resPkt.SetError(ERROR_CODE::ROOM_MGR_INVALID_CHAT_REQ);
		m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES,
			sizeof(PktRoomChatRes), (char*)&resPkt);
		m_pRefLogger->Write(LOG_TYPE::L_ERROR,
			"%s | Invalid user. sessionIndex(%d), errorCode(%hd)",
			__FUNCTION__, packetInfo->SessionIndex, resPkt.GetError());
		return ERROR_CODE::ROOM_MGR_INVALID_CHAT_REQ;
	}

	roomPtr->NotifyChatMsg(user, msgPtr, msgLen);
	resPkt.SetError(ERROR_CODE::NONE);
	m_pRefNetwork->SendData(packetInfo->SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES,
		sizeof(PktRoomChatRes), (char*)&resPkt);
	m_pRefLogger->Write(LOG_TYPE::L_INFO,
		"%s | Success. sessionIndex(%d), userID(%s)",
		__FUNCTION__, packetInfo->SessionIndex, user->GetId().data());
	return ERROR_CODE::NONE;
}
