#include "net/EventLoop.h"
#include "net/EventLoopThreadPool.h"
#include "net/EventLoopThread.h"
#include "net/TcpServer.h"
#include "base/Logging.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>


using namespace muduo;
using namespace muduo::net;
using namespace boost::uuids;

class ClientSession//表示一个客户端会话，管理与客户端连接相关的信息和操作
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
private:
	std::string m_sessionid;
};

typedef std::shared_ptr<ClientSession> ClientSessionPtr;//定义一个智能指针类型，方便管理ClientSession对象的生命周期