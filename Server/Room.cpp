//#include <algorithm>
//#include <cstring>
#include <wchar.h>

#include "NetLib/ILog.h"
#include "NetLib/TcpNetwork.h"
#include "Packet.h"
#include "ErrorCode.h"
#include "User.h"
#include "Room.h"


typedef NServerNetLib::LOG_TYPE LOG_TYPE;


Room::Room(const short index, const short maxUserCount)
	:
	m_Index(index),
	m_MaxUserCount(maxUserCount)
{
}

Room::~Room()
{
}

/*
void Room::Init(const short index, const short maxUserCount)
{
	m_Index = index;
	m_MaxUserCount = maxUserCount;
}
*/

void Room::SetNetwork(TcpNet* pNetwork, ILog* pLogger)
{
	m_pRefNetwork = pNetwork;
	m_pRefLogger = pLogger;
}

void Room::Clear()
{
	m_IsUsed = false;
	m_UserList.clear();
}

void Room::AddNewUser(User *newUser)
{
	PktRoomNewUserNtf	resPkt;
	std::vector<int8_t>	userListData;
	size_t				writePos = 0;

	// notify new user to users in the room.
	resPkt.userUniqueId = newUser->GetIndex();
	resPkt.idLen = newUser->GetId().size();
	memcpy(resPkt.userID, newUser->GetId().data(),
		newUser->GetId().size());

	for (const auto& user : m_UserList)
	{
		m_pRefNetwork->SendData(user->GetSessionIndex(),
			(short)PACKET_ID::ROOM_ENTER_NEW_USER_NTF,
			sizeof(resPkt), (char *)&resPkt);
		m_pRefLogger->Write(LOG_TYPE::L_INFO,
			"%s | New user notify. sessionIndex(%d), m_ID(%s)",
			__FUNCTION__, user->GetSessionIndex(), user->GetId().data());
	}

	// notify users list in the room to new user.
	userListData.push_back(static_cast<int8_t>(GetUserCount()));
	writePos += 1;
	for (const auto& user: m_UserList)
	{
		// GetIndex() or GetSessionIndex()?
		uint64_t	tempUserUniqueId = user->GetIndex();
		const char* tempUserId = user->GetId().data();
		uint8_t		tempUserIdLen = user->GetId().size();

		/* reverse bytes
		tempUserUniqueId
			= ((tempUserUniqueId & 0xff00000000000000) >> 56)
			+ ((tempUserUniqueId & 0xff000000000000) >> 40)
			+ ((tempUserUniqueId & 0xff0000000000) >> 24)
			+ ((tempUserUniqueId & 0xff00000000) >> 8)
			+ ((tempUserUniqueId & 0xff000000) << 8)
			+ ((tempUserUniqueId & 0xff0000) << 24)
			+ ((tempUserUniqueId & 0xff00) << 40)
			+ ((tempUserUniqueId & 0xff) << 56);
		*/

		userListData.insert(userListData.begin() + writePos,
			(uint8_t *)&tempUserUniqueId, (uint8_t *)&tempUserUniqueId + 8);
		writePos += 8;
		userListData.push_back(tempUserIdLen);
		writePos += 1;
		userListData.insert(userListData.begin() + writePos,
			(uint8_t*)tempUserId, (uint8_t*)tempUserId + tempUserIdLen);
		writePos += tempUserIdLen;
	}

	// add newUser to m_UserList and set DOMAIN_STATE of the user.
	m_UserList.push_back(newUser);
	newUser->EnterRoom(GetIndex());
	m_pRefNetwork->SendData(newUser->GetSessionIndex(),
		(short)PACKET_ID::ROOM_ENTER_USER_LIST_NTF,
		(short)userListData.size(), (char*)userListData.data());
}

void Room::RemoveUser(int64_t leaveUserUniqueId)
{
	PktRoomLeaveUserNtf resPkt = {0, };
	short	curUserCount = this->GetUserCount();
	short	i;

	// find user and remove.
	// if there're many users, is using array OK?
	for (i = 0; i < curUserCount; ++i)
	{
		if (m_UserList[i]->GetIndex() == leaveUserUniqueId)
		{
			resPkt.userUniqueId = leaveUserUniqueId;
			m_UserList.erase(m_UserList.begin() + i);
			break;
		}
	}
	
	// notify leaved user to existing users.
	for (short i = 0; i < curUserCount - 1; ++i)
	{
		User* userPtr = m_UserList[i];

		m_pRefNetwork->SendData(userPtr->GetSessionIndex(),
			(short)PACKET_ID::ROOM_LEAVE_USER_NTF, sizeof(resPkt), (char*)&resPkt);
		m_pRefLogger->Write(LOG_TYPE::L_INFO,
			"%s | Notify leaved user. sessionIndex(%d), m_Id(%s)",
			__FUNCTION__, userPtr->GetSessionIndex(), userPtr->GetId().data());
	}
}

void Room::NotifyChatMsg(User* sender, MSG_TYPE *msgPtr, short msgLen)
{
	PktRoomChatNtf ntfPkt = {0, };

	ntfPkt.userUniqueId = sender->GetIndex();
	memcpy(ntfPkt.msg, msgPtr, msgLen);
	ntfPkt.msgLen = msgLen;
	msgPtr[msgLen] = 0;
	for (const User* user: m_UserList)
	{
		if (user == sender)
			continue;
		m_pRefLogger->Write(LOG_TYPE::L_INFO,
			"%s | Send messege. msg(%.64s)", __FUNCTION__, msgPtr);
		m_pRefNetwork->SendData(user->GetSessionIndex(), (short)PACKET_ID::ROOM_CHAT_NTF, 
			sizeof(ntfPkt), (char*)&ntfPkt);
	}
}
