#pragma once

#include <vector>
#include <string>
#include <memory>

#include "User.h"


namespace NServerNetLib
{
	class ITcpNetwork;
	class ILog;
}


typedef NServerNetLib::ITcpNetwork	TcpNet;
typedef NServerNetLib::ILog			ILog;


class Game;

class Room
{
public:
	Room(const short index, const short maxUserCount);
	virtual ~Room();

	//void Init(const short index, const short maxUserCount);

	void SetNetwork(TcpNet* pNetwork, ILog* pLogger);

	void Clear();

	short GetIndex()
	{
		return m_Index;
	}

	bool IsUsed()
	{
		return m_IsUsed;
	}

	short GetMaxUserCount()
	{
		return m_MaxUserCount;
	}

	short GetUserCount()
	{
		return (short)m_UserList.size();
	}
	
	void AddNewUser(User* newUser);

	void RemoveUser(int64_t leaveUserUniqueId);

	// MSG_TYPE
	void NotifyChatMsg(User* sender, char *msgPtr, short msgLen);

private:
	ILog* m_pRefLogger;
	TcpNet* m_pRefNetwork;

	const short m_Index = -1;
	const short m_MaxUserCount;

	bool m_IsUsed = false;
	std::vector<User*> m_UserList;

	//Game *m_pGame = nullptr;
};
