#include "IMSer.h"
#include "ClientSession.h"
#include <sstream>

using namespace std::placeholders;

bool IMSer::init(const std::string& ip, short port, EventLoop* loop)
{
	InetAddress addr(ip, port);
	m_server.reset(new TcpServer(loop, addr, "chatserver", TcpServer::kReusePort));//创建TCP服务器对象，绑定事件循环和监听地址
	//kReusePort选项允许多个服务器实例绑定到同一端口，提高负载均衡和性能,哪个端口链接的没了，哪个端口就会继续接受连接
	m_server->setConnectionCallback(std::bind(&IMSer::OnConnection, this, std::placeholders::_1));
	//std::placeholders::_1是一个占位符，表示在回调函数被调用时会传入一个参数，这个参数将被绑定到OnConnection函数的第一个参数位置
	m_server->start();//启动服务器，开始监听和接受客户端连接
	return true;
}

void IMSer::OnConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		ClientSessionPtr client(new ClientSession(conn));//创建一个新的ClientSession对象，管理这个连接的生命周期
		{
			std::lock_guard<std::mutex> guard(m_sessionlock);//使用互斥锁保护对m_mapclient的访问，确保线程安全
			//guard对象在构造时会自动锁定互斥锁，在析构时会自动释放互斥锁，确保在函数执行期间对m_mapclient的访问是安全的，避免竞争条件和数据不一致的问题
			m_mapclient.insert(ConnPair((std::string)*client, client));//将连接的名称和连接对象添加到映射中，方便后续根据连接ID查找和管理连接
			//m_lstConn.push_back(client);//将新的连接添加到连接列表中，方便后续管理和广播消息
		}
	}
	else {
		OnClose(conn);
	}

}

void IMSer::OnClose(const TcpConnectionPtr& conn)
{
	//TODO: 处理这个链接，找到这个链接在列表中的位置，并删除它
	stringstream ss;
	ss << (void*)conn.get();//将连接对象的地址转换为字符串形式，作为连接ID的一部分，确保每个连接都有一个唯一的标识符
	ConnIter iter = m_mapclient.find(ss.str());
	if(iter != m_mapclient.end())
	{
		/*m_mapclient.erase(iter);*/
		//TODO: 连接关闭，删除连接对象，释放资源
		m_mapclient.erase(iter);
	}
	else
	{
		//TODO:有问题的连接
		std::cout << conn->name() << std::endl;
	}
}

std::shared_ptr<ClientSession> IMSer::GetSessionByID(int32_t userid)
{
	std::lock_guard<std::mutex> guard(m_sessionlock);
	for (const auto& pair : m_mapclient)
	{
		if (pair.second->UserID() == userid)
			return pair.second;
	}
	return ClientSessionPtr();
}
