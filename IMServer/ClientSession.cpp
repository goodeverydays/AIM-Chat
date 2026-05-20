#include "ClientSession.h"
#include <sstream>
#include "BinaryReader.h"
#include <json/json.h>//JSON库的头文件，提供了处理JSON数据的功能，例如解析和生成JSON字符串等
#include "UserManager.h"


using namespace BR;

ClientSession::ClientSession(const TcpConnectionPtr& conn)
{
	m_seq = 0;//初始化会话的序号为0，可以根据需要进行自增或者其他操作来区分不同的会话
	stringstream ss;
	ss << (void*)conn.get();//将连接对象的地址转换为字符串形式，作为会话ID的一部分，确保每个会话都有一个唯一的标识符
	m_sessionid = ss.str();//
	//这里就是两个括号，外面是to_string函数的括号，里面是boost::uuids::random_generator()()的括号，表示调用random_generator函数生成一个UUID对象，然后将其转换为字符串形式
	TcpConnectionPtr* client = const_cast<TcpConnectionPtr*>(&conn);//将传入的连接对象转换为TcpConnectionPtr类型的引用，以便后续操作
	//*const_cast<std::string*>(&conn->name()) = m_sessionid;//将连接对象的名称设置为会话ID，方便后续根据连接ID查找和管理连接
    //const_cast是C++中的一个类型转换操作符，用于去除对象的常量属性，使其可以被修改。在这里，conn->name()返回一个const std::string&类型的连接名称，
	// 而我们需要将其修改为会话ID，
	// 因此使用const_cast将其转换为std::string*类型的指针，然后通过解引用操作符*将其赋值为m_sessionid。这样做是为了在不修改TcpConnection类的定义的情况下，
	// 给每个连接分配一个唯一的会话ID，并且能够通过连接对象的名称来访问这个会话ID。
	(*client)->setMessageCallback(std::bind(&ClientSession::OnRead, this, _1, _2, std::placeholders::_3));//设置连接对象的消息回调函数，当服务器接收到消息时会调用ClientSession类的OnRead成员函数进行处理
	
}

ClientSession::~ClientSession()
{

}

void ClientSession::OnRead(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
{
	while (buf->readableBytes() >= sizeof(int32_t))//循环读取缓冲区中的数据，直到没有可读的数据为止
	{
		int32_t packagesize = 0;
		BinaryReader::dump(buf->peek(), buf->readableBytes());
		packagesize = *(int32_t*)buf->peek();//从缓冲区中读取消息长度，假设消息长度是一个32位整数，存储在消息的开头
		if (buf->readableBytes() < sizeof(int32_t) + packagesize)//判断缓冲区中是否有足够的数据来读取完整的消息，如果没有，则返回，等待下一次读取
		{
			return;
		}

		buf->retrieve(sizeof(int32_t));//从缓冲区中移除已经读取的消息长度
		string msgbuff;
		msgbuff.assign(buf->peek()+6, packagesize-6);//从缓冲区中读取消息内容，假设消息内容紧跟在消息长度之后，长度为packagesize
		BinaryReader::dump(msgbuff);//调用BinaryReader类的dump函数将消息内容以十六进制格式输出到控制台，假设消息内容存储在缓冲区中，长度为packagesize

		buf->retrieve(packagesize);//从缓冲区中移除已经读取的消息内容
		//TODO:处理消息内容，例如解析消息格式，执行相应的操作
		if(Process(conn, msgbuff) != true)//调用Process函数处理消息内容，如果处理成功，则继续读取下一条消息，否则返回，等待下一次读取
		{
			cout << "process error, close connection!\r\n";
			conn->forceClose();//如果处理失败，则强制关闭连接，断开与客户端的通信
		}
	}

}



bool ClientSession::Process(const TcpConnectionPtr& conn, string msgbuff)
{
	BinaryReader reader(msgbuff);
	int cmd = -1;
	if (reader.ReadData<decltype(cmd)>(cmd) == false)return false;
	if (reader.ReadData<int>(m_seq) == false) return false;
	
	string data;
	
	if (reader.ReadData<string>(data) == false) return false;

	switch (cmd)
	{
	case msg_type_heartbeart://心跳包
		OnHeartbeatResponse(conn, data);//调用OnHeartbeatResponse函数处理心跳响应消息
		break;
	case msg_type_register://注册消息
		OnRegisterResponse(conn, data);//调用OnRegisterResponse函数处理注册响应消息
		break;
	case msg_type_login://登录消息
		OnLoginResponse(conn, data);//调用OnLoginResponse函数处理登录响应消息
		break;
	case msg_type_getofriendlist://获取好友列表
		OnGetFriendListResponse(conn, data);
		break;
	case msg_type_finduser://查找用户
		OnFindUserResponse(conn, data);
		break;
	case msg_type_operatefriend://操作好友
		OnOperateFriendResponse(conn, data);
		break;
	case msg_type_updateuserinfo://更新用户信息
		OnUpdateUserInfoResponse(conn, data);
		break;
	case msg_type_modifypassword://修改密码
		OnModifyPasswordResponse(conn, data);
		break;
	case msg_type_creategroup://创建群聊
		OnCreateGroupResponse(conn, data);
		break;
	case msg_type_getgroupmembers://获取群成员
		OnGetGroupMembersResponse(conn, data);
		break;
	case msg_type_chat://单聊信息
		reader.ReadData(m_target);
		OnChatResponse(conn, data);
		break;
	case msg_type_multichat://群聊信息
		reader.ReadData(m_targets);
		OnMultiChatResponse(conn, data);
		break;
	default:
		break;
	}
	return true;
}

void ClientSession::OnHeartbeatResponse(const TcpConnectionPtr& conn, const string& data)
{
	//包的长度 4字节， 包的长度不能压缩，固定格式
	// 包的类型 4字节 这个也不能压缩，固定格式
	// 包的序号 4字节 这个也不能压缩， 固定格式
	// 包的数据长度 4字节，包的数据内容 可变长度，这两个可以压缩
	BinaryWriter writer;
	int cmd = msg_type_heartbeart;//心跳响应消息的类型
	writer.WriteData(htonl(cmd));
	writer.WriteData(htonl(m_seq));//心跳响应消息的序号，可以根据需要进行自增或者其他操作来区分不同的响应
	string empty;
	writer.WriteData(empty);
	TcpSession::Send(conn,writer);//调用Send函数将构建好的心跳响应消息发送给客户端，假设Send函数已经实现，并且能够正确发送消息
	printf("%s(%d): %s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

void ClientSession::OnRegisterResponse(const TcpConnectionPtr& conn, const string& data)
{
	//{"username" : "手机号"， "nickname" : "昵称", "password" : "密码"}
	//解析注册请求消息的内容，假设消息内容是一个JSON字符串，包含
	string result;
	BinaryWriter writer;//创建一个二进制写入器对象，用于构建注册响应消息的内容
	Json::Reader reader;//创建一个JSON解析器对象，用于将JSON字符串解析成JSON值对象
	Json::Value root, response;//创建一个JSON值对象，用于存储解析后的JSON数据
	int cmd = msg_type_register;//注册响应消息的类型，可以根据需要定义不同的类型来表示不同的响应，例如注册成功、注册失败等
	writer.WriteData(htonl(cmd));//将注册响应消息的类型写入消息内容
	writer.WriteData(htonl(m_seq));//将注册响应消息的序号写入消息内容，假设m_seq是一个整数变量，表示当前会话的序号，可以根据需要进行自增或者其他操作来区分不同的响应

	if (reader.parse(data, root) == false)//调用JSON解析器的parse方法将消息内容解析成JSON值对象，如果解析失败，则返回
	{
		cout << "error json: " << data << "\r\n";
		response["code"] = 101;
		response["message"] = "json parse failed!";
		result = response.toStyledString();
		writer.WriteData(result);
		TcpSession::Send(conn, writer);
		return;
	}
	if(!root["username"].isString() || !root["nickname"].isString() || !root["password"].isString())//判断解析后的JSON数据是否包含必要的字段，例如用户名、昵称和密码，如果缺少任何一个字段，则返回
	{
		cout << "error type:" << data << "\r\n";
		response["code"] = 102;
		response["message"] = "json data type failed!";
		result = response.toStyledString();
		writer.WriteData(result);
		TcpSession::Send(conn, writer);
		return;
	}
	User user;//创建一个用户对象，用于存储注册请求中的用户信息
	user.username = root["username"].asString();//从解析后的JSON数据中提取用户名，并将其赋值给用户对象的username成员变量
	user.nickname = root["nickname"].asString();//从解析后的JSON数据中提取昵称，并将其赋值给用户对象的nickname成员变量
	user.password = root["password"].asString();//从解析后的JSON数据中提取密码，并将其赋值给用户对象的password成员变量
	
	if (!Singleton<UserManager>::instance().AddUser(user))
	{
		cout << "add user failed!\r\n";
		response["code"] = 100;
		response["message"] = "register failed!";
		result = response.toStyledString();
		writer.WriteData(result);
		TcpSession::Send(conn, writer);
		printf("%s(%d): %s\r\n", __FILE__, __LINE__, __FUNCTION__);
	}
	else {
		response["code"] = 0;//将注册响应消息的状态码设置为0，表示注册成功，可以根据需要定义不同的状态码来表示不同的注册结果
		response["message"] = "ok";//将注册响应消息的提示信息设置为"注册成功"，可以根据需要定义不同的提示信息来描述注册结果
		result = response.toStyledString();//将构建好的JSON值对象转换为一个格式化的JSON字符串，作为注册响应消息的内容
		writer.WriteData<string>(result);//将注册响应消息的内容写入消息内容，假设result是一个字符串变量，包含了注册响应的结果信息，例如状态码和提示信息等
		TcpSession::Send(conn, writer);//调用Send函数将构建好的注册响应消息发送给客户端，假设Send函数已经实现，并且能够正确发送消息
		printf("%s(%d): %s\r\n", __FILE__, __LINE__, __FUNCTION__);
	}
}

void ClientSession::OnLoginResponse(const TcpConnectionPtr& conn, const string& data)
{
	//
	BinaryWriter writer;
	string result;
	Json::Reader reader;
	Json::Value root, response;
	int cmd = msg_type_login;
	writer.WriteData(htonl(cmd));
	writer.WriteData(htonl(m_seq));

	if (reader.parse(data, root) == false)
	{
		cout << __FILE__ << "(" << __LINE__ << ")\r\n";
		response["code"] = 101;
		response["message"] = "json parse failed!";
		result = root.toStyledString();
		writer.WriteData(result);
		TcpSession::Send(conn, writer);
		return;
	}

	if(!root["username"].isString() || !root["password"].isString() || !root["clienttype"].isInt() ||
		!root["status"].isInt())
	{
		cout << __FILE__ << "(" << __LINE__ << ")\r\n";
		response["code"] = 102;
		response["message"] = "json data type failed!";
		result = response.toStyledString();
		writer.WriteData(result);
		TcpSession::Send(conn, writer);
		return;
	}
	string username = root["username"].asString();
	string password = root["password"].asString();
	User user;
	if (Singleton<UserManager>::instance().GetUserInfoUsername(username, user) == false)
	{
		cout << __FILE__ << "(" << __LINE__ << ")\r\n";
		response["code"] = 103;
		response["message"] = "user is not exist or password is incorrect!";
		result = response.toStyledString();
		writer.WriteData(result);
		TcpSession::Send(conn, writer);
		return;
	}
	
	if (password != user.password)
	{
		cout << __FILE__ << "(" << __LINE__ << ")\r\n";
		response["code"] = 104;
		response["msg"] = "user is not exist or password is incorrect!";
		result = response.toStyledString();
		writer.WriteData(result);
		TcpSession::Send(conn, writer);
		return;
	}


	//如果成功返回应答
	response["code"] = 0;
	response["msg"] = "ok";
	response["userid"] = m_user->userid;
	response["username"] = m_user->username;
	response["nickname"] = m_user->nickname;
	response["facetype"] = m_user->facetype;
	response["customface"] = m_user->customface;
	response["gender"] = m_user->gender;
	response["birthday"] = m_user->birthday;
	response["signature"] = m_user->signature;
	response["address"] = m_user->address;
	response["phonenumber"] = m_user->phonenumber;
	response["mail"] = m_user->mail;
	m_user->status = 1;//将用户的状态设置为在线，可以根据需要定义不同的状态值来表示用户的在线状态，例如0表示离线，1表示在线等
	result = response.toStyledString();
	writer.WriteData(result);
	TcpSession::Send(conn, writer);
	//推送通知信息
	list<NotifyMsgCache> listNotifyCache;
	Singleton<MsgCacheManager>::instance().GetNotifyMsgCache(m_user->userid, listNotifyCache);
	for (const auto& iter : listNotifyCache)
	{
		writer = iter.notifymsg;
		TcpSession::Send(conn, writer);
	}
	//推送聊天消息
	list<ChatMsgCache>listChatCache;
	Singleton<MsgCacheManager>::instance().GetChatMsgCache(m_user->userid, listChatCache);
	for (const auto& iter : listChatCache)
	{
		writer = iter.chatmsg;
		TcpSession::Send(conn, writer);
	}

	//给其他用户推送上线信息
	list<UserPtr> friends;
	Singleton<UserManager>::instance().GetFriendInfoByUserId(m_user->userid, friends);
	IMServer& imserver = Singleton<IMSer>::instance();
	for (const auto& iter : friends)
	{
		ClientSessionPtr targetSession = imserver.GetSessionByID(iter->userid);
		if (targetSession)
		{
			printf("%s(%d): %s userid %d target %d\r\n", __FILE__, __LINE__, __FUNCTION__, m_user->userid, targetSession->UserID());
			targetSession->SendUserStatusChangeMsg(m_user->userid, 1);//调用SendUserStatusChangeMsg函数向好友发送用户状态变化消息，通知好友用户的状态发生了变化，例如上线、离线等，假设SendUserStatusChangeMsg函数已经实现，并且能够正确发送消息
		}
	}
	printf("%s(%d) : %s userid %d\r\n", __FILE__, __LINE__, __FUNCTION__, m_user->userid);
}

void ClientSession::OnGetFriendListResponse(const TcpConnectionPtr& conn, const string& data)
{
	std::list<UserPtr> lstFriend;
	Singleton<UserManager>::instance().GetFriendInfoByUserID(m_user->userid, lstFriends);
	Json::Value root;
	root["code"] = 0;
	root["msg"] = "ok";
	root["userinfo"] = Json::Value(Json::arrayValue);
	for (const auto& iter : lstFriends)
	{
		Json::Value f;
		f["userid"] = iter->userid;
		f["username"] = iter->username;
		f["nickname"] = iter->nickname;
		f["facetype"] = iter->facetype;
		f["customface"] = iter->customface;
		f["gender"] = iter->gender;
		f["birthday"] = iter->birthday;
		f["signature"] = iter->signature;
		f["address"] = iter->address;
		f["phonenumber"] = iter->phonenumber;
		f["mail"] = iter->mail;
		f["clienttype"] = 1; 
		f["status"] = iter->status ? 1 : 0;
		root["userinfo"].append(f);
	}
	BinaryWriter writer;
	writer.WriteData(htonl(msg_type_getofriendlist));
	writer.WriteData(htonl(m_seq));
	writer.WriteData(root.toStyledString());
	TcpSession::Send(conn, writer);
	printf("%s(%d) : %s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

void ClientSession::OnFindUserResponse(const TcpConnectionPtr& conn, const string& data)
{
	Json::Value root, result;
	Json::Reader reader;
}

void ClientSession::OnOperateFriendResponse(const TcpConnectionPtr& conn, const string& data)
{
}

void ClientSession::OnUpdateUserInfoResponse(const TcpConnectionPtr& conn, const string& data)
{
}

void ClientSession::OnModifyPasswordResponse(const TcpConnectionPtr& conn, const string& data)
{
}

void ClientSession::OnCreateGroupResponse(const TcpConnectionPtr& conn, const string& data)
{
}

void ClientSession::OnGetGroupMembersResponse(const TcpConnectionPtr& conn, const string& data)
{
}

void ClientSession::OnChatResponse(const TcpConnectionPtr& conn, const string& data)
{
	BinaryWriter writer;
	writer.WriteData(htonl(msg_type_chat));
	writer.WriteData(htonl(m_seq));
	writer.WriteData(data);
	//消息发送者
	writer.WriteData(m_user->userid);
	//消息接收者
	writer.WriteData(m_target);
	printf("%s(%d): %s target:%d cur:%dr\r\n",
		__FILE__, __LINE__, __FUNCTION__, m_target, m_user->userid);
	cout << data << endl;
	UserManager& userMgr = Singleton<UserManager>::instance();
	//写入消息记录
	if (!userMgr.SaveChatMsgToDb(m_user->userid, m_target, data))
	{
		LOG_ERROR << "Write chat msg to db error, senderid = " << m_user->userid << ", targetid = " << m_target << ", chatmsg:" << data;;
	}

	IMServer& imserver = Singleton<IMSer>::instance();
	MsgCacheManager& msgCacheMgr = Singleton<MsgCacheManager>:::instance();
	//单聊消息
	if (m_target < GROUPID_BOUBDARY)
	{
		//先看目标用户是否在线
		ClientSessionPtr targetSession = imserver.GetSessionByID(m_target);
		//目标用户不在线， 缓存这个消息
		if (!targetSession)
		{
			msgCacheMgr.AddChatMsgCache(m_target, writer.toString());
			return;
		}

		targetSession->Send(writer);
		return;
	}

	//群聊消息
	std::list<UserPtr> friends;//群成员
	userMgr.GetFriendInfoByUserId(m_target, friends);
	std::string strUserInfo;
	bool userOnline = false;
	for (const auto& iter : friends)
	{
		//先看目标用户是否在线
		ClienSessionPtr targetSession = imserver.GetSessionByID(iter->userid);
		//目标用户不在线， 缓存这个消息
		if (!targetSession)
		{
			msgCacheMgr.AddChatMsgCache(iter->userid, writer.toString());
			continue;
		}
		targetSession->Send(writer);
	}
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

//群发业务处理
void ClientSession::OnMultiChatResponse(const TcpConnectionPtr& conn, const string& data)
{
	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(m_targets, root))
	{
		LOG_ERROR << "invalid json: targets: " << m_targets << "data: " << data << ", userid" << m_user->userid << ", client: " <<
			conn.peerAddress().toIpPort();
		return;
	}
	if (!root["targets"].isArray())
	{
		LOG_ERROR << "invalid json: targets: " << m_targets << "data: " << data << ", userid" << m_user->userid << ", client: " <<
			conn.peerAddress().toIpPort();
		return;
	}
	for (uint32_t i = 0; i < root["targets"].size(); ++i)
	{
		m_target = root["targets"][i].asInt();
		OnChatResponse(conn, data);
	}
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	LOG_INFO << "Send to client: cmd=msg_type_multichat, targets: " << m_targets << "data: " << data << ", userid : " <<
		m_user->userid << ", client : " << conn->peerAddress().tolpPort();
}

void ClientSession::DeleteFriend(const TcpConnectionPtr& conn, const string& data)
{
}

void ClientSession::OnAddGroupResponse(const TcpConnectionPtr& conn, const string& data)
{
}

void ClientSession::SendUserStatusChangeMsg(const TcpConnectionPtr& conn, const string& data)
{
}
