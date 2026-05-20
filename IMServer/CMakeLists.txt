# CMakeList.txt: IMServer 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

# 将源代码添加到此项目的可执行文件。

#if (CMAKE_VERSION VERSION_GREATER 3.12)
 # set_property(TARGET IMServer PROPERTY CXX_STANDARD 20)
#endif()
cmake_minimum_required (VERSION 3.12)

project(IMServer)

#这个宏会影响编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -g -Wall -O0 -Wno-unused-variable -pthread")

Link_directories(#这个是针对64位的
	${PROJECT_SOURCE_DIR}/lib
	/usr/lib64/mysql/
)

find_library(MYSQL_LIB libmysqlclient.so /usr/lib64/mysql/)
if(NOT MYSQL_LIB)
	message(FATAL_ERROR "not found mysqlclient.so at /usr/lib64/mysql/")
endif(NOT MYSQL_LIB)#必须进行匹配

include_directories(. ./base ./net ./third_party/jsoncpp/include)

set(muduo_srcs
base/Timestamp.cc
base/TimeZone.cc
base/Date.cc
base/CountDownLatch.cc
base/FileUtil.cc
base/Logging.cc
base/LogFile.cc
base/LogStream.cc
base/AsyncLogging.cc
base/Thread.cc
base/CurrentThread.cc
base/ProcessInfo.cc
"base/Condition.cc"


net/Acceptor.cc
net/Buffer.cc
net/Channel.cc
net/Connector.cc
net/Poller.cc
net/poller/DefaultPoller.cc
net/poller/EPollPoller.cc
net/poller/PollPoller.cc
net/InetAddress.cc
net/Socket.cc
net/SocketsOps.cc
net/TcpClient.cc
net/TcpConnection.cc
net/TcpServer.cc
net/EventLoop.cc
net/EventLoopThread.cc
net/EventLoopThreadPool.cc
net/Timer.cc
net/TimerQueue.cc
)

set(mysql_srcs
mysqltool.cpp
Field.cpp
QueryResult.cpp
MySqlManager.cpp

)

set(chat_srcs  
IMServer.cpp
IMSer.cpp
ClientSession.cpp
UserManager.cpp
BinaryReader.cpp
MsgCacheManager.cpp
)

set(file_srcs 
    IMTestClient.cpp
)

# jsoncpp 第三方库
add_subdirectory(third_party/jsoncpp)

add_executable (imchatserver ${chat_srcs} ${muduo_srcs} ${mysql_srcs})
#只是确认库存在或者设置库目录，是没有用的，需要链接进来才能起效果
target_link_libraries(imchatserver mysqlclient jsoncpp)

add_executable (imfileserver ${file_srcs} ${muduo_srcs})
#只是确认库存在或者设置库目录，是没有用的，需要链接进来才能起效果
target_link_libraries(imfileserver)
# TODO: 如有需要，请添加测试并安装目标。