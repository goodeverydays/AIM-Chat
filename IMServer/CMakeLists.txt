# CMakeList.txt: IMServer 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#

# 将源代码添加到此项目的可执行文件。

#if (CMAKE_VERSION VERSION_GREATER 3.12)
 # set_property(TARGET IMServer PROPERTY CXX_STANDARD 20)
#endif()
cmake_minimum_required (VERSION 3.16)

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

# --- Protobuf 依赖（IM协议编解码）---
find_package(Protobuf REQUIRED)

# 编译 .proto 文件生成 C++ 代码
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/im.proto)

# --- gRPC 依赖（Agent 通信）---
find_package(gRPC CONFIG QUIET)
if(gRPC_FOUND)
    message(STATUS "gRPC found, enabling Agent gRPC client")

    # 生成 agent.proto 的 gRPC C++ 代码
    get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
    protobuf_generate_cpp(AGENT_PROTO_SRCS AGENT_PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/agent.proto)
    protobuf_generate_grpc_cpp(AGENT_GRPC_SRCS AGENT_GRPC_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/agent.proto
        PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}")

    # avatar.proto
    protobuf_generate_cpp(AVATAR_PROTO_SRCS AVATAR_PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/avatar.proto)
    protobuf_generate_grpc_cpp(AVATAR_GRPC_SRCS AVATAR_GRPC_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/avatar.proto
        PLUGIN "protoc-gen-grpc=${GRPC_CPP_PLUGIN}")

    set(AGENT_SRCS
        AgentGrpcClient.cpp
        AvatarGrpcClient.cpp
        ${AGENT_PROTO_SRCS}
        ${AGENT_GRPC_SRCS}
        ${AVATAR_PROTO_SRCS}
        ${AVATAR_GRPC_SRCS}
    )
    add_definitions(-DHAVE_AGENT_GRPC)
else()
    message(WARNING "gRPC not found, Agent gRPC client disabled. "
                    "Install: apt install libgrpc++-dev protobuf-compiler-grpc")
    set(AGENT_SRCS "")
endif()

include_directories(. ./base ./net ./third_party/jsoncpp/include ${CMAKE_CURRENT_BINARY_DIR})

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
MsgCacheManager.cpp
IMCodec.cpp                # protobuf编解码适配层
net/protobuf/ProtobufCodecLite.cc
${PROTO_SRCS}              # protoc生成的 im.pb.cc
${AGENT_SRCS}              # Agent gRPC 客户端
)

set(file_srcs
    IMTestClient.cpp
)

# jsoncpp 第三方库
add_subdirectory(third_party/jsoncpp)

add_executable (imchatserver ${chat_srcs} ${muduo_srcs} ${mysql_srcs})
#只是确认库存在或者设置库目录，是没有用的，需要链接进来才能起效果

# 链接 gRPC 库
if(gRPC_FOUND)
    target_link_libraries(imchatserver mysqlclient jsoncpp ${Protobuf_LIBRARIES} z
        gRPC::grpc++ gRPC::grpc++_reflection)
else()
    target_link_libraries(imchatserver mysqlclient jsoncpp ${Protobuf_LIBRARIES} z)
endif()

add_executable (imfileserver ${file_srcs} ${muduo_srcs})
#只是确认库存在或者设置库目录，是没有用的，需要链接进来才能起效果
target_link_libraries(imfileserver)
# TODO: 如有需要，请添加测试并安装目标。
