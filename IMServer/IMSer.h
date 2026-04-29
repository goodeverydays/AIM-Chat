#include "net/EventLoop.h"
#include "net/EventLoopThreadPool.h"
#include "net/EventLoopThread.h"
#include "net/TcpServer.h"
#include "base/Logging.h"
#include "ClientSession.h"
#include <list>
#include <mutex>


using namespace muduo;
using namespace muduo::net;

class IMSer final//final关键字表示这个类不能被继承，确保IMSer类的设计和实现不会被修改或扩展，保持其稳定性和安全性
{
public:
	IMSer() = default;
	IMSer(const IMSer&) = delete;
	IMSer& operator=(const IMSer&) = delete;
	~IMSer() = default;
	bool init(const std::string& ip, short port, EventLoop* loop);

protected:
	void OnConnection(const TcpConnectionPtr& conn);
	void OnClose(const TcpConnectionPtr& conn);
private:
	std::shared_ptr<TcpServer> m_server;//TCP服务器对象，负责监听和接受客户端连接
	std::map<std::string, ClientSessionPtr> m_mapclient;//连接ID和连接对象的映射，方便根据连接ID查找和管理连接
	std::mutex m_sessionlock;//保护m_mapclient的操作
};
typedef std::pair<std::string, ClientSessionPtr> ConnPair;//定义一个连接对，包含连接ID和连接对象
typedef std::map<std::string, ClientSessionPtr>::iterator ConnIter;//定义一个连接迭代器，方便遍历连接映射