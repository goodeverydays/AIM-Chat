#include "ClientSession.h"
#include <sstream>
#include "BinaryReader.h"

using namespace BR;

ClientSession::ClientSession(const TcpConnectionPtr& conn)
{
	m_seq = 0;//初始化会话的序号为0，可以根据需要进行自增或者其他操作来区分不同的会话
	m_sessionid = to_string(boost::uuids::random_generator()());//生成一个随机的UUID作为会话ID，确保每个会话都有一个唯一的标识符
	//这里就是两个括号，外面是to_string函数的括号，里面是boost::uuids::random_generator()()的括号，表示调用random_generator函数生成一个UUID对象，然后将其转换为字符串形式
	TcpConnectionPtr* client = const_cast<TcpConnectionPtr*>(&conn);//将传入的连接对象转换为TcpConnectionPtr类型的引用，以便后续操作
	*const_cast<std::string*>(&conn->name()) = m_sessionid;//将连接对象的名称设置为会话ID，方便后续根据连接ID查找和管理连接
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
		packagesize = *(int32_t*)buf->peek();//从缓冲区中读取消息长度，假设消息长度是一个32位整数，存储在消息的开头
		if (buf->readableBytes() < sizeof(int32_t) + packagesize)//判断缓冲区中是否有足够的数据来读取完整的消息，如果没有，则返回，等待下一次读取
		{
			return;
		}
		buf->retrieve(sizeof(int32_t));//从缓冲区中移除已经读取的消息长度
		string msgbuff;
		msgbuff.assign(buf->peek(), packagesize);//从缓冲区中读取消息内容，假设消息内容紧跟在消息长度之后，长度为packagesize
		buf->retrieve(packagesize);//从缓冲区中移除已经读取的消息内容
		//TODO:处理消息内容，例如解析消息格式，执行相应的操作
		if(Process(conn, msgbuff) != true)//调用Process函数处理消息内容，如果处理成功，则继续读取下一条消息，否则返回，等待下一次读取
		{
			cout << "process error, close connection!\r\n";
			conn->forceClose();//如果处理失败，则强制关闭连接，断开与客户端的通信
		}
	}

}

void ClientSession::Send(const std::string& buf)
{
	//TODO: 发送消息给客户端，可以通过连接对象的send方法将消息发送出去，例如conn->send(buf);
}

bool ClientSession::Process(const TcpConnectionPtr& conn, string msgbuff)
{
	BinaryReader reader(msgbuff);
	int cmd = -1;
	if (reader.ReadData<decltype(cmd)>(cmd) == false)return false;
	if (reader.ReadData<int>(m_seq) == false) return false;
	size_t dataLength = 0;
	if (reader.ReadData<size_t>(dataLength) == false) return false;
	switch (cmd)
	{
	case msg_type_heartbeart://心跳包
		break;
	case msg_type_register://注册消息
		break;
	case msg_type_login://登录消息
		break;
	case msg_type_getofriendlist://获取好友列表
		break;
	case msg_type_finduser://查找用户
		break;
	case msg_type_operatefriend://操作好友
		break;
	case msg_type_updateuserinfo://更新用户信息
		break;
	case msg_type_modifypassword://修改密码
		break;
	case msg_type_creategroup://创建群聊
		break;
	case msg_type_getgroupmembers://获取群成员
		break;
	case msg_type_chat://单聊信息
		break;
	case msg_type_multichat://群聊信息
		break;
	default:
		break;
	}
	return true;
}