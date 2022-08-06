#pragma once

#include "PacketID.h"
#include "ErrorCode.h"

// CPP: push current alignment offset to internal stack 
// and set it to 1.
#pragma pack(push, 1)

struct PktHeader
{
	short toTalSize;
	short id;
	unsigned char reserve;
};

struct PktBase
{
	short errorCode = (short)ERROR_CODE::NONE;

	// ?: setter for public member
	void SetError(ERROR_CODE error)
	{
		errorCode = (short)error;
	}

	short GetError(void)
	{
		return errorCode;
	}
};

// Login request
const int MAX_USER_ID_SIZE = 16;
const int MAX_USER_PASSWORD_SIZE = 16;
struct PktLogInReq
{
	char szID[MAX_USER_ID_SIZE] = { 0, };
	char szPW[MAX_USER_PASSWORD_SIZE] = { 0, };
};

struct PktLogInRes : PktBase
{
};

struct PktRoomEnterReq
{
	// it uses -1 to search empty room
	int32_t roomNumber;
};

struct PktRoomEnterRes : PktBase
{
	int64_t userUniqueId;
};

// let users in a room know newly entered user.
struct PktRoomNewUserNtf
{
	int64_t userUniqueId;
	uint8_t idLen;
	char userID[MAX_USER_ID_SIZE] = { 0, };
};

// variable size
struct PktRoomUserListNtf
{
	uint8_t userCount;
	int8_t *userListData;
};

// request for room exit
struct PktRoomLeaveReq
{
};

struct PktRoomLeaveRes : PktBase
{
};

// let users in a lobby know about user exited from a room.
struct PktRoomLeaveUserNtf
{
	int64_t userUniqueId;
//	char userID[MAX_USER_ID_SIZE] = { 0, };
};

#define MSG_TYPE char

// room chat
const int MAX_ROOM_CHAT_MSG_SIZE = 256;
struct PktRoomChatReq
{
	uint16_t msgLen;
	MSG_TYPE msg[MAX_ROOM_CHAT_MSG_SIZE] = { 0, };
};

struct PktRoomChatRes : PktBase
{
};

struct PktRoomChatNtf
{
	int64_t userUniqueId;
	uint16_t msgLen;
	MSG_TYPE msg[MAX_ROOM_CHAT_MSG_SIZE] = { 0, };
};

// pop current alignment offset and take one from internal stack
#pragma pack(pop)
