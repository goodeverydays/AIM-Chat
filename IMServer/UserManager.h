#pragma once
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include "base/Logging.h"
#include "base/Singleton.h"
#include <set>
#include <list>
#include <mutex>

using namespace std;

class User
{
public:
	int32_t		 userid;//用户ID
	string		 username;//用户名
	string		 nickname;//昵称
	string		 password;//密码
	int32_t		 facetype;//默认头像
	string		 customface;//自定义头像
	string		 customfacefmt;//自定义头像格式
	int32_t		 gender;//性别
	int32_t		 birthday;//生日
	string		 signature;//个性签名
	string		 address;//地址
	string       phonenumber;//电话号码
	string		 mail;//邮箱
	int32_t		 ownerid;//用于标记群主信息的
	set<int32_t> friends;//好友列表，存储用户的好友ID，可以使用set容器来管理好友关系，方便进行添加、删除和查询操作
	User() : userid(-1), facetype(-1), gender(-1), 
		birthday(-1), ownerid(-1) {}
	~User() = default;
};

class UserManager final
{
public:
	UserManager() = default;
	~UserManager() = default;
	bool init();
	UserManager(const UserManager&) = delete;
	UserManager& operator=(const UserManager&) = delete;

	//添加用户
	bool AddUser(User& user);
	//从数据库加载用户信息
	bool LoadUserFromDB();
	bool LoadRelationshipFromDB(int32_t userid, set<int32_t>& friends);//从数据库加载用户关系信息，例如好友关系等
	bool GetUserInfoUsername(const string& name, User& user);
private:
	mutex m_mutex;//互斥锁对象，用于保护对用户信息的访问和修改，确保线程安全，避免数据竞争和不一致的问题
	list<User> m_cachedUsers;//缓存用户信息的列表，可以使用list容器来存储用户对象，方便进行遍历和管理
	int32_t m_baseUserID{ 0 };//用于生成新的用户ID的基数，可以根据需要进行自增或者其他操作来确保每个用户都有一个唯一的ID
	int32_t m_baseGroupID{ 0xFFFFFFF };//用于生成新的群ID的基数，可以根据需要进行自增或者其他操作来确保每个群都有一个唯一的ID
	
};