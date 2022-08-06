#pragma once

#include <string>


enum class DOMAIN_STATE: short
{
	NONE = 0,
	LOGIN = 1,
	ROOM = 2
};

class User
{
public:
	User() {}
	virtual ~User() {}

	void Init(const short index)
	{
		m_Index = index;
	}

	void Clear()
	{
		m_SessionIndex = 0;
		m_ID = "";
		m_IsConfirm = false;
		m_CurDomainState = DOMAIN_STATE::NONE;
		m_RoomIndex = -1;
	}

	void Set(const int sessionIndex, const char* pszID)
	{
		m_IsConfirm = true;
		m_CurDomainState = DOMAIN_STATE::LOGIN;

		m_SessionIndex = sessionIndex;
		m_ID = pszID;
	}

	short GetIndex() const
	{
		return m_Index;
	}

	short GetRoomIndex() const
	{
		return m_RoomIndex;
	}

	int GetSessionIndex() const
	{
		return m_SessionIndex;
	}

	const std::string& GetId() const
	{
		return m_ID;
	}

	bool IsConfirm() const
	{
		return m_IsConfirm;
	}

	void EnterRoom(const short roomIndex)
	{
		m_RoomIndex = roomIndex;
		m_CurDomainState = DOMAIN_STATE::ROOM;
	}

	void LeaveRoom()
	{
		m_RoomIndex = -1;
		m_CurDomainState = DOMAIN_STATE::LOGIN;
	}

	bool IsCurDomainInLogin() const
	{
		return m_CurDomainState == DOMAIN_STATE::LOGIN;
	}

	bool IsCurDomainInRoom() const
	{
		return m_CurDomainState == DOMAIN_STATE::ROOM;
	}

protected:
	short m_Index = -1;

	int m_SessionIndex = -1;

	std::string m_ID;

	bool m_IsConfirm = false;

	DOMAIN_STATE m_CurDomainState = DOMAIN_STATE::NONE;

	short m_RoomIndex = -1;
};
