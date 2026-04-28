#include <iostream>
#include<unistd.h>
#include <stdio.h>
#include<sys/types.h>
#include <errno.h>

#include "base/Logging.h"
#include "net/TcpClient.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"


using namespace muduo;
using namespace muduo::net;

TcpClient* pclient = NULL;
EventLoop* ploop = NULL;
void OnClose(const TcpConnectionPtr& conn)
{
	std::cout << "onClose called " << std::endl;
	//ploop->quit();
	exit(0);
}

void onConnected(const TcpConnectionPtr& conn)
{
	conn->send("hello");
	conn->setCloseCallback(OnClose);
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buffer,
	Timestamp ts)
{
	std::cout << buffer->retrieveAllAsString() << std::endl;
	
	//conn->shutdown();
	//pclient->stop();
	//conn->getLoop()->queueInLoop(OnClose);
}

int main()
{
	EventLoop loop;
	ploop = &loop;
	InetAddress servAddr("127.0.0.1", 9527);
	TcpClient client(&loop, servAddr, "echo client");//创建TCP客户端对象，指定事件循环、服务器地址和客户端名称
	pclient = &client;
	client.setConnectionCallback(onConnected);
	client.setMessageCallback(onMessage);
	client.connect();
	loop.loop();
}