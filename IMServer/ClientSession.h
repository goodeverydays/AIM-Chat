#include "net/EventLoop.h"
#include "net/EventLoopThreadPool.h"
#include "net/EventLoopThread.h"
#include "net/TcpServer.h"
#include "base/Logging.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <iostream>
#include "BinaryReader.h"

using namespace muduo;
using namespace muduo::net;
using namespace boost::uuids;
using namespace std::placeholders;

enum {
	msg_type_unknown,
	//用户消息
	msg_type_heartbeart = 1000,
	msg_type_register,
	msg_type_login,
	msg_type_getofriendlist,
	msg_type_finduser,
	msg_type_operatefriend,
	msg_type_userstatuschange,
	msg_type_updateuserinfo,
	msg_type_modifypassword,
	msg_type_creategroup,
	msg_type_getgroupmembers,
	//聊天消息
	msg_type_chat = 1100,//单聊消息
	msg_type_multichat,//单发消息
};

class TcpSession
{
public:
	TcpSession() = default;
	~TcpSession() = default;
	void Send(const TcpConnectionPtr& conn, BR::BinaryWriter& writer)
	{
		string out = writer.toString();
		if (conn != NULL)
		{
			conn->send(out.c_str(), out.size());
		}
	}
};

class ClientSession : public TcpSession//表示一个客户端会话，管理与客户端连接相关的信息和操作
{
public:
	ClientSession(const TcpConnectionPtr& conn);
	//为了控制对象的生命周期，防止提前销毁，或者销毁之后，重复销毁，禁止拷贝构造和赋值操作
	ClientSession(const ClientSession&) = delete;
	ClientSession& operator=(const ClientSession&) = delete;
	~ClientSession();

	operator std::string()
	{
		return m_sessionid;
	}

	void OnRead(const muduo::net::TcpConnectionPtr& conn, Buffer* buf, Timestamp time);
	
	//业务函数
	bool Process(const TcpConnectionPtr& conn, string msgbuff);

	int32_t UserID()const { return (m_user != NULL) ? m_user->userid : -1; }
	void Send(BinarWriter& writer)
	{
		if (m_conn == NULL)
		{
			printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
			return;
		}
		TcpSession::Send(m_conn, writer);
	}

protected:
	void OnHeartbeatResponse(const TcpConnectionPtr& conn, const string& data);//处理心跳响应消息的函数，接收客户端发送的心跳消息，并进行相应的处理，例如更新会话状态或者记录心跳时间等
	void OnRegisterResponse(const TcpConnectionPtr& conn, const string& data);//处理注册响应消息的函数，接收客户端发送的注册请求消息，并进行相应的处理，例如验证注册信息、创建用户账户等
	void OnLoginResponse(const TcpConnectionPtr& conn, const string& data);//处理登录响应消息的函数，接收客户端发送的登录请求消息，并进行相应的处理，例如验证登录信息、创建会话等
	void OnGetFriendListResponse(const TcpConnectionPtr& conn, const string& data);//处理获取好友列表响应消息的函数，接收客户端发送的获取好友列表请求消息，并进行相应的处理，例如查询好友列表信息、返回好友列表等
	void OnFindUserResponse(const TcpConnectionPtr& conn, const string& data);//处理查找用户响应消息的函数，接收客户端发送的查找用户请求消息，并进行相应的处理，例如查询用户信息、返回用户信息等
	void OnOperateFriendResponse(const TcpConnectionPtr& conn, const string& data);//处理操作好友响应消息的函数，接收客户端发送的操作好友请求消息，并进行相应的处理，例如添加好友、删除好友等
	
	void OnUpdateUserInfoResponse(const TcpConnectionPtr& conn, const string& data);//处理更新用户信息响应消息的函数，接收客户端发送的更新用户信息请求消息，并进行相应的处理，例如更新用户信息、返回更新结果等
	void OnModifyPasswordResponse(const TcpConnectionPtr& conn, const string& data);//处理修改密码响应消息的函数，接收客户端发送的修改密码请求消息，并进行相应的处理，例如验证旧密码、更新新密码等
	void OnCreateGroupResponse(const TcpConnectionPtr& conn, const string& data);//处理创建群组响应消息的函数，接收客户端发送的创建群组请求消息，并进行相应的处理，例如创建群组、返回创建结果等
	void OnGetGroupMembersResponse(const TcpConnectionPtr& conn, const string& data);//处理获取群成员响应消息的函数，接收客户端发送的获取群成员请求消息，并进行相应的处理，例如查询群成员信息、返回群成员列表等
	void OnChatResponse(const TcpConnectionPtr& conn, const string& data);//处理聊天响应消息的函数，接收客户端发送的聊天请求消息，并进行相应的处理，例如转发聊天消息、保存聊天记录等
	void OnMultiChatResponse(const TcpConnectionPtr& conn, const string& data);//处理单发响应消息的函数，接收客户端发送的单发请求消息，并进行相应的处理，例如转发单发消息、保存聊天记录等

	void DeleteFriend(const TcpConnectionPtr& conn, const string& data);//删除好友
	void OnAddGroupResponse(const TcpConnectionPtr& conn, const string& data);//添加群成员
	void SendUserStatusChangeMsg(const TcpConnectionPtr& conn, const string& data);//发送用户状态变化消息，通知好友用户的状态发生了变化，例如上线、离线等
private:
	std::string m_sessionid;
	int m_seq;//会话的序号
	UserPtr m_user;//会话关联的用户信息，可以根据需要定义一个用户类来存储用户相关的信息，例如用户名、昵称、密码等
	int32_t m_target;//会话消息的时候使用
	string m_targets;//群聊消息的时候使用
	TcpConnectionPtr m_conn;
};

typedef std::shared_ptr<ClientSession> ClientSessionPtr;//定义一个智能指针类型，方便管理ClientSession对象的生命周期