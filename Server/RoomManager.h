#pragma once

#include <vector>
#include <unordered_map>


namespace NServerNetLib
{
	class ITcpNetwork;
	class ILog;
}


typedef NServerNetLib::ITcpNetwork	TcpNet;
typedef NServerNetLib::ILog			ILog;

class User;
class Room;


class RoomManager
{
public:
	RoomManager() = default;
	virtual ~RoomManager() = default;

	void Init(const int maxRoomCountByLobby, const int maxRoomUserCount);

	void Release();

	void SetNetwork(TcpNet* pNetwork, ILog* pLogger);

	Room* GetRoom(const short roomIndex);

	auto MaxRoomCount()
	{
		return (short)m_RoomList.size();
	}

protected:
	ILog* m_pRefLogger;
	TcpNet* m_pRefNetwork;

	std::vector<Room*> m_RoomList;
};
