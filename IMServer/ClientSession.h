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
		writer.Clear();
		int	cmd = (int)out.size();//获取包的长度
		writer.WriteData<int>(cmd);
		out = writer.toString() + out;
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

	void Send(const std::string& buf);
	
	//业务函数
	bool Process(const TcpConnectionPtr& conn, string msgbuff);

protected:
	void OnHeartbeatResponse(const TcpConnectionPtr& conn, const string& data);//处理心跳响应消息的函数，接收客户端发送的心跳消息，并进行相应的处理，例如更新会话状态或者记录心跳时间等
	void OnRegisterResponse(const TcpConnectionPtr& conn, const string& data);//处理注册响应消息的函数，接收客户端发送的注册请求消息，并进行相应的处理，例如验证注册信息、创建用户账户等
private:
	std::string m_sessionid;
	int m_seq;//会话的序号
};

typedef std::shared_ptr<ClientSession> ClientSessionPtr;//定义一个智能指针类型，方便管理ClientSession对象的生命周期