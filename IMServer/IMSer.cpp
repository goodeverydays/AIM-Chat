#include "IMSer.h"
#include "ClientSession.h"
#ifdef HAVE_AGENT_GRPC
#include "AgentGrpcClient.h"
#endif
#include <sstream>

using namespace std::placeholders;

bool IMSer::init(const std::string& ip, short port, EventLoop* loop)
{
	InetAddress addr(ip, port);
	m_server.reset(new TcpServer(loop, addr, "chatserver", TcpServer::kReusePort));
	m_server->setConnectionCallback(std::bind(&IMSer::OnConnection, this, std::placeholders::_1));
	m_server->start();

#ifdef HAVE_AGENT_GRPC
	// 初始化 Agent gRPC 客户端（连接 goagent 微服务）
	m_agentClient.reset(new AgentGrpcClient("127.0.0.1:19527"));
#endif

	return true;
}

void IMSer::OnConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		ClientSessionPtr client(new ClientSession(conn));
		{
			std::lock_guard<std::mutex> guard(m_sessionlock);
			m_mapclient.insert(ConnPair((std::string)*client, client));
		}
	}
	else {
		OnClose(conn);
	}

}

void IMSer::OnClose(const TcpConnectionPtr& conn)
{
	stringstream ss;
	ss << (void*)conn.get();
	ConnIter iter = m_mapclient.find(ss.str());
	if(iter != m_mapclient.end())
	{
		m_mapclient.erase(iter);
	}
	else
	{
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

#ifdef HAVE_AGENT_GRPC
AgentGrpcClient* IMSer::GetAgentClient()
{
	return m_agentClient.get();
}
#endif
