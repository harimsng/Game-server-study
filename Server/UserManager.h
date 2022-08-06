#pragma once

#include <unordered_map>
#include <deque>
#include <vector>
#include <string>

#include "ErrorCode.h"

class User;

class UserManager
{
public:
	UserManager();
	virtual ~UserManager();

	void Init(const int maxUserCount);

	// psz: pointer, null terminated(0) string
	ERROR_CODE AddUser(const int sessionIndex, const char* pszID);
	ERROR_CODE RemoveUser(const int sessionIndex);

	std::tuple<ERROR_CODE, User*> GetUser(const int sessionIndex);

	// private methods
private:
	User* AllocUserObjPoolIndex();
	void ReleaseUserObjPoolIndex(const int index);

	User* FindUser(const int sessionIndex);
	User* FindUser(const char* pszID);

	// private member
private:
	std::vector<User> m_UserObjPool;
	std::deque<int> m_UserObjPoolIndex;

	std::unordered_map<int, User*> m_UserSessionDic;
	// if key type is const char *, it extract hash value from address.
	std::unordered_map<std::string, User*> m_UserIDDic;
};