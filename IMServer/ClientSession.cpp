#include "ClientSession.h"

ClientSession::ClientSession(const TcpConnectionPtr& conn)
{
	m_sessionid = to_string(boost::uuids::random_generator()());//生成一个随机的UUID作为会话ID，确保每个会话都有一个唯一的标识符
	//这里就是两个括号，外面是to_string函数的括号，里面是boost::uuids::random_generator()()的括号，表示调用random_generator函数生成一个UUID对象，然后将其转换为字符串形式
	*const_cast<std::string*>(&conn->name()) = m_sessionid;//将连接对象的名称设置为会话ID，方便后续根据连接ID查找和管理连接
    //const_cast是C++中的一个类型转换操作符，用于去除对象的常量属性，使其可以被修改。在这里，conn->name()返回一个const std::string&类型的连接名称，
	// 而我们需要将其修改为会话ID，
	// 因此使用const_cast将其转换为std::string*类型的指针，然后通过解引用操作符*将其赋值为m_sessionid。这样做是为了在不修改TcpConnection类的定义的情况下，
	// 给每个连接分配一个唯一的会话ID，并且能够通过连接对象的名称来访问这个会话ID。
}

ClientSession::~ClientSession()
{

}