#include "UserManager.h"
#include "MySqlManager.h"
#include "base/Singleton.h"
#include <sstream>

using namespace std;
using namespace muduo;

namespace {
    // SQL字符串转义：将单引号替换为两个单引号（SQL标准转义），防止SQL注入
    string EscapeSqlString(const string& str) {
        string result;
        result.reserve(str.size());
        for (char c : str) {
            if (c == '\'') {
                result += "''";
            } else {
                result += c;
            }
        }
        return result;
    }
}

bool UserManager::init()
{
	//加载用户信息（所有的用户）
	if(LoadUserFromDB() != true)
	{
		cout << "LoadUserFromDB failed!";
		return false;
	}
	//加载用户关系（好友关系）
	for (auto& iter : m_cachedUsers)
	{
		if (!LoadRelationshipFromDB(iter->userid, iter->friends))
		{
			cout << "load friend failed!\r\n";
		}
	}
	return true;
}

bool UserManager::AddUser(User& user)
//添加用户
{
	stringstream sql;
	m_baseUserID++;//自增用户ID基数，确保每个用户都有一个唯一的ID
	sql << "INSERT INTO t_user (f_user_id, f_username, f_nickname, f_password, f_register_time)" <<
		"VALUES (" << m_baseUserID << ", '" << EscapeSqlString(user.username) << "', '"
		<< EscapeSqlString(user.nickname) << "', '" << EscapeSqlString(user.password) << "', NOW())";
	//构造SQL插入语句，将用户信息插入到数据库中，假设t_user表已经存在，并且包含相应的字段
	bool result = Singleton<MySqlManager>::instance().Execute(sql.str());//调用MySqlManager的Execute方法执行SQL语句，如果执行失败，则输出错误信息并返回false
	if(result == false)
	{
		cout << "insert user failed!\r\n";
		return false;
	}
	user.userid = m_baseUserID;//将生成的用户ID赋值给用户对象的userid成员变量，确保用户对象具有正确的ID信息
	user.facetype = 0;//设置用户的默认头像类型，可以根据需要进行修改，例如根据用户的性别或者其他属性来设置不同的默认头像
	user.birthday = 20000101;//设置用户的默认生日，可以根据需要进行修改，例如设置为一个特定的日期或者根据用户的年龄来计算默认生日
	user.gender = 0;//设置用户的默认性别，可以根据需要进行修改，例如设置为一个特定的值来表示未知或者其他性别
	user.ownerid = 0;//设置用户的默认群主ID，可以根据需要进行修改，例如设置为一个特定的值来表示没有群主或者其他情况
	{
		lock_guard<mutex> guard(m_mutex);//使用lock_guard对象自动管理互斥锁的锁定和释放，确保在访问和修改用户信息时线程安全，避免数据竞争和不一致的问题
		m_cachedUsers.push_back(std::make_shared<User>(user));//将用户对象拷贝到堆上并添加到缓存列表中
	}
	return true;
}

bool UserManager::LoadUserFromDB()
//从数据库加载用户信息
{
	stringstream sql;
	sql << "SELECT f_user_id, f_username, f_nickname, f_password, f_facetype, f_customface,"
		<< " f_gender, f_birthday, f_signature, f_address, f_phonenumber, f_mail FROM t_user ORDER BY f_user_id DESC";
	//为什么用DESC：先注册的用户，id小， 后注册的用户id大，后注册的用户上线的概论大， 所以降序排列
	QueryResultPtr result = Singleton<MySqlManager>::instance().Query(sql.str());
	if (result == NULL)
	{
		return false;
	}
	while (result != NULL)
	{
		Field* pRow = result->Fetch();
		if (pRow == NULL) break;
		User u;
		u.userid = pRow[0].toInt32();
		u.username = pRow[1].GetString();
		u.nickname = pRow[2].GetString();
		u.password = pRow[3].GetString();
		u.facetype = pRow[4].toInt32();
		u.customface = pRow[5].GetString();
		u.gender = pRow[6].toInt32();
		u.birthday = pRow[7].toInt32();
		u.signature = pRow[8].GetString();
		u.address = pRow[9].GetString();
		u.phonenumber = pRow[10].GetString();
		u.mail = pRow[11].GetString();
		{
			lock_guard<mutex> guard(m_mutex);//使用lock_guard对象自动管理互斥锁的锁定和释放，确保在访问和修改用户信息时线程安全，避免数据竞争和不一致的问题
			m_cachedUsers.push_back(std::make_shared<User>(u));
			m_mapUsers[u.userid] = make_shared<User>(u);
		}
		if(u.userid < 0xFFFFFFF && u.userid > m_baseUserID)
		{
			m_baseUserID = u.userid;//更新基数，排除群ID，确保用户ID不落入群ID范围
		}
		if(u.userid >= 0xFFFFFFF && u.userid > m_baseGroupID)
		{
			m_baseGroupID = u.userid;//更新群ID基数，确保重启后群ID不冲突
		}
		if (result->NextRow() == false) break;//如果没有更多数据可供获取，跳出循环
	}
	result->EndQuery();//结束查询，释放资源
	return true;
}

bool UserManager::LoadRelationshipFromDB(int32_t userid, set<int32_t>& friends)
//从数据库加载用户关系信息，例如好友关系等
{
	stringstream sql;
	sql << "SELECT f_user_id1, f_user_id2 FROM t_user_relationship WHERE f_user_id1 = " << userid << " OR f_user_id2 = " << userid << " ;";
	QueryResultPtr result = Singleton<MySqlManager>::instance().Query(sql.str());
	if (result == NULL)
	{
		return false;
	}
	while (result != NULL)
	{
		Field* pRow = result->Fetch();
		if (pRow == NULL) break;
		int friendid1 = pRow[0].toInt32();
		int friendid2 = pRow[1].toInt32();
		if (friendid1 == userid)//如果查询结果中的用户ID1与当前用户ID匹配，说明用户ID2是当前用户的好友，将其插入到好友列表中
		{
			friends.insert(friendid2);
		}
		else if (friendid2 == userid)
		{
			friends.insert(friendid1);
		}
		if (result->NextRow() == false) break;//如果没有更多数据可供获取，跳出循环
	}
	result->EndQuery();//结束查询，释放资源
	return true;
}

bool UserManager::GetUserInfoUsername(const string& name, UserPtr& user)
{
	lock_guard<mutex> guard(m_mutex);//使用lock_guard对象自动管理互斥锁的锁定和释放，确保在访问和修改用户信息时线程安全，避免数据竞争和不一致的问题
	for (const auto& iter : m_cachedUsers)
	{
		if (iter->username == name)//遍历缓存用户信息的列表，查找与给定用户名匹配的用户对象，如果找到，则将其赋值给参数user，并返回true
		{
			user = iter;
			return true;
		}
	}
	return false;//如果没有找到匹配的用户对象，则返回false
}

bool UserManager::GetFriendInfoByUserID(int32_t userid, list<UserPtr>& friends)
{
	lock_guard<mutex> guard(m_mutex);
	iterMapUser iter = m_mapUsers.find(userid);
	if (iter == m_mapUsers.end()) return false;
	//遍历该用户的好友ID集合，查找每个好友的详细信息并加入结果列表
	for (int32_t friendid : iter->second->friends)
	{
		iterMapUser itFriend = m_mapUsers.find(friendid);
		if (itFriend != m_mapUsers.end())
		{
			friends.push_back(itFriend->second);
		}
	}
	return true;
}

UserPtr UserManager::GetUserByID(int32_t userid)
{
	lock_guard<mutex> guard(m_mutex);
	iterMapUser iter = m_mapUsers.find(userid);//在用户ID到用户对象的映射中查找与给定用户ID匹配的用户对象，如果找到，则返回其对应的智能指针，否则返回空指针
	if (iter == m_mapUsers.end()) return UserPtr();
	return iter->second;
	return UserPtr();
}

bool UserManager::MakeFriendRelationship(int32_t smallid, int32_t greatid)
{
	if (smallid >= greatid) return false;//判断小ID是否大于等于大ID，如果是，则返回false，表示无法建立好友关系，因为小ID应该小于大ID
	stringstream sql;
	sql << "INSERT INTO t_user_relationship(f_user_id1, f_user_id2) VALUES(" <<
		smallid << ", " << greatid << ")";//构造SQL插入语句，将好友关系信息插入到数据库中，假设t_user_relationship表已经存在，并且包含相应的字段
	if (!Singleton<MySqlManager>::instance().Execute(sql.str()))
	{
		return false;
	}
	//修改缓存中的好友关系
	lock_guard<mutex> guard(m_mutex);
	//使用lock_guard对象自动管理互斥锁的锁定和释放，确保在访问和修改用户信息时线程安全，避免数据竞争和不一致的问题
	iterMapUser it = m_mapUsers.find(smallid);//在用户ID到用户对象的映射中查找与小ID匹配的用户对象，如果找到，则将大ID插入到其好友列表中
	if (it == m_mapUsers.end()) return false;
	it->second->friends.insert(greatid);
	it = m_mapUsers.find(greatid);//在用户ID到用户对象的映射
	if (it == m_mapUsers.end()) return false;
	it->second->friends.insert(smallid);
	return true;
}

bool UserManager::ReleaseFriendRelationship(int32_t smallid, int32_t greatid)
{
	if (smallid >= greatid) return false;
	stringstream sql;
	sql << "DELETE FROM t_user_relationship WHERE f_user_id1 = "
		<< smallid << " AND f_user_id2 = " << greatid;
	if (!Singleton<MySqlManager>::instance().Execute(sql.str()))
	{
		return false;
	}
	//更新缓存中双方的好友列表
	lock_guard<mutex> guard(m_mutex);
	iterMapUser itSmall = m_mapUsers.find(smallid);
	if (itSmall != m_mapUsers.end())
	{
		itSmall->second->friends.erase(greatid);
	}
	iterMapUser itGreat = m_mapUsers.find(greatid);
	if (itGreat != m_mapUsers.end())
	{
		itGreat->second->friends.erase(smallid);
	}
	return true;
}

bool UserManager::UpdateUserInfo(int32_t userid, const User& newuserinfo)
{
	stringstream sql;
	sql << "UPDATE t_user SET "
		<< "f_nickname = '" << EscapeSqlString(newuserinfo.nickname) << "', "
		<< "f_facetype = " << newuserinfo.facetype << ", "
		<< "f_customface = '" << EscapeSqlString(newuserinfo.customface) << "', "
		<< "f_gender = " << newuserinfo.gender << ", "
		<< "f_birthday = " << newuserinfo.birthday << ", "
		<< "f_signature = '" << EscapeSqlString(newuserinfo.signature) << "', "
		<< "f_address = '" << EscapeSqlString(newuserinfo.address) << "', "
		<< "f_phonenumber = '" << EscapeSqlString(newuserinfo.phonenumber) << "', "
		<< "f_mail = '" << EscapeSqlString(newuserinfo.mail) << "' "
		<< "WHERE f_user_id = " << userid;
	if (!Singleton<MySqlManager>::instance().Execute(sql.str()))
	{
		return false;
	}
	//更新缓存中的用户信息
	lock_guard<mutex> guard(m_mutex);
	iterMapUser iter = m_mapUsers.find(userid);
	if (iter != m_mapUsers.end())
	{
		iter->second->nickname = newuserinfo.nickname;
		iter->second->facetype = newuserinfo.facetype;
		iter->second->customface = newuserinfo.customface;
		iter->second->gender = newuserinfo.gender;
		iter->second->birthday = newuserinfo.birthday;
		iter->second->signature = newuserinfo.signature;
		iter->second->address = newuserinfo.address;
		iter->second->phonenumber = newuserinfo.phonenumber;
		iter->second->mail = newuserinfo.mail;
	}
	return true;
}

bool UserManager::ModifyUserPassword(int32_t userid, const string& newpassword)
{
	stringstream sql;
	sql << "UPDATE t_user SET f_password = '"
		<< EscapeSqlString(newpassword) << "' WHERE f_user_id = " << userid;
	if (!Singleton<MySqlManager>::instance().Execute(sql.str()))
	{
		return false;
	}
	//更新缓存中的密码
	lock_guard<mutex> guard(m_mutex);
	iterMapUser iter = m_mapUsers.find(userid);
	if (iter != m_mapUsers.end())
	{
		iter->second->password = newpassword;
	}
	return true;
}

bool UserManager::AddGroup(const char* groupname, int32_t ownerid, int32_t& groupid)
{
	m_baseGroupID++;
	groupid = m_baseGroupID;
	stringstream sql;
	sql << "INSERT INTO t_user (f_user_id, f_username, f_nickname, f_password, f_owner_id, f_register_time) VALUES ("
		<< groupid << ", '" << EscapeSqlString(groupname) << "', '"
		<< EscapeSqlString(groupname) << "', '', " << ownerid << ", NOW())";
	if (!Singleton<MySqlManager>::instance().Execute(sql.str()))
	{
		return false;
	}
	//将群组作为特殊用户加入缓存
	User group;
	group.userid = groupid;
	group.username = groupname;
	group.nickname = groupname;
	group.ownerid = ownerid;
	group.facetype = 0;
	group.gender = 0;
	group.birthday = 20000101;
	{
		lock_guard<mutex> guard(m_mutex);
		m_cachedUsers.push_back(std::make_shared<User>(group));
		m_mapUsers[groupid] = make_shared<User>(group);
	}
	return true;
}

bool UserManager::SaveChatMsgToDb(int32_t senderid, int32_t targetid, const string& chatmsg)
{
	stringstream sql;
	sql << "INSERT INTO t_chatmsg (f_senderid, f_targetid, f_msgcontent) VALUES ("
		<< senderid << ", " << targetid << ", '"
		<< EscapeSqlString(chatmsg) << "')";
	return Singleton<MySqlManager>::instance().Execute(sql.str());
}

bool UserManager::DeleteFriendToUser(int32_t userid, int32_t friendid)
{
	int32_t smallid = (userid < friendid) ? userid : friendid;
	int32_t greatid = (userid < friendid) ? friendid : userid;
	stringstream sql;
	sql << "DELETE FROM t_user_relationship WHERE f_user_id1 = "
		<< smallid << " AND f_user_id2 = " << greatid;
	if (!Singleton<MySqlManager>::instance().Execute(sql.str()))
	{
		return false;
	}
	//更新缓存中双方的好友列表
	lock_guard<mutex> guard(m_mutex);
	iterMapUser iterUser = m_mapUsers.find(userid);
	if (iterUser != m_mapUsers.end())
	{
		iterUser->second->friends.erase(friendid);
	}
	iterMapUser iterFriend = m_mapUsers.find(friendid);
	if (iterFriend != m_mapUsers.end())
	{
		iterFriend->second->friends.erase(userid);
	}
	return true;
}
