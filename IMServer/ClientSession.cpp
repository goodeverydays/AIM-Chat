#include "ClientSession.h"
#include <sstream>
#include "IMCodec.h"
#include "im.pb.h"                     // protoc生成的消息头文件
#include "UserManager.h"
// 注意: jsoncpp依赖已由protobuf替代，此文件不再直接使用jsoncpp

using namespace muduo::net;

ClientSession::ClientSession(const TcpConnectionPtr& conn)
    : m_seq(0)
    , m_target(0)
{
    std::stringstream ss;
    ss << (void*)conn.get();
    m_sessionid = ss.str();

    // 初始化protobuf编解码器，设置解码完成后的回调
    m_codec.reset(new IMCodec(
        std::bind(&ClientSession::OnMessageReceived, this, _1, _2, _3)));

    // 设置连接的数据到达回调，委托给IMCodec处理
    TcpConnectionPtr* client = const_cast<TcpConnectionPtr*>(&conn);
    (*client)->setMessageCallback(
        std::bind(&ClientSession::OnRead, this, _1, _2, std::placeholders::_3));

    m_conn = conn;
}

ClientSession::~ClientSession()
{
}

// 发送消息容器（供其他会话转发消息时调用）
void ClientSession::SendContainer(const im::MessageContainer& msg)
{
    if (m_codec && m_conn)
    {
        m_codec->send(m_conn, msg);
    }
}

// TCP数据到达回调 —— 委托给IMCodec处理帧解析和protobuf反序列化
void ClientSession::OnRead(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
{
    // IMCodec内部完成：
    // 1. 从Buffer中读取完整帧（按total_len分帧）
    // 2. 验证Adler32校验和
    // 3. 反序列化protobuf MessageContainer
    // 4. 触发OnMessageReceived回调
    m_codec->onMessage(conn, buf, time);
}

// IMCodec解码完成回调 —— 根据cmd分派到各业务处理函数
void ClientSession::OnMessageReceived(const TcpConnectionPtr& conn,
                                      const im::MessageContainer& msg,
                                      Timestamp time)
{
    m_seq = msg.seq();  // 同步当前序号

    switch (msg.cmd())
    {
    case msg_type_heartbeart:
        OnHeartbeatResponse(conn, msg);
        break;
    case msg_type_register:
        OnRegisterResponse(conn, msg);
        break;
    case msg_type_login:
        OnLoginResponse(conn, msg);
        break;
    case msg_type_getofriendlist:
        OnGetFriendListResponse(conn, msg);
        break;
    case msg_type_finduser:
        OnFindUserResponse(conn, msg);
        break;
    case msg_type_operatefriend:
        OnOperateFriendResponse(conn, msg);
        break;
    case msg_type_updateuserinfo:
        OnUpdateUserInfoResponse(conn, msg);
        break;
    case msg_type_modifypassword:
        OnModifyPasswordResponse(conn, msg);
        break;
    case msg_type_creategroup:
        OnCreateGroupResponse(conn, msg);
        break;
    case msg_type_getgroupmembers:
        OnGetGroupMembersResponse(conn, msg);
        break;
    case msg_type_getchathistory:
        OnGetChatHistoryResponse(conn, msg);
        break;
    case msg_type_chat:
        OnChatResponse(conn, msg);
        break;
    case msg_type_multichat:
        OnMultiChatResponse(conn, msg);
        break;
    default:
        LOG_WARN << "Unknown message cmd=" << msg.cmd()
                 << " from " << conn->peerAddress().toIpPort();
        break;
    }
}

// ============================================================
// 心跳响应 (cmd=1000)
// ============================================================
void ClientSession::OnHeartbeatResponse(const TcpConnectionPtr& conn,
                                        const im::MessageContainer& msg)
{
    im::MessageContainer response;
    response.set_cmd(msg_type_heartbeart);
    response.set_seq(m_seq);
    // 心跳无需payload

    m_codec->send(conn, response);
    printf("%s(%d): %s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

// ============================================================
// 注册响应 (cmd=1001)
// 请求: RegisterReq { username, nickname, password }
// 响应: CommonRsp { code, msg }
// ============================================================
void ClientSession::OnRegisterResponse(const TcpConnectionPtr& conn,
                                       const im::MessageContainer& msg)
{
    im::RegisterReq req;
    im::CommonRsp rsp;

    // 解析注册请求
    if (!req.ParseFromString(msg.payload()))
    {
        LOG_ERROR << "Failed to parse RegisterReq";
        rsp.set_code(101);
        rsp.set_msg("protobuf parse failed!");
    }
    else if (req.username().empty() || req.nickname().empty() || req.password().empty())
    {
        rsp.set_code(102);
        rsp.set_msg("data field missing!");
    }
    else
    {
        // 构建User对象并尝试注册
        User user;
        user.username = req.username();
        user.nickname = req.nickname();
        user.password = req.password();

        if (!Singleton<UserManager>::instance().AddUser(user))
        {
            rsp.set_code(100);
            rsp.set_msg("register failed!");
            printf("%s(%d): %s - add user failed\r\n", __FILE__, __LINE__, __FUNCTION__);
        }
        else
        {
            rsp.set_code(0);
            rsp.set_msg("ok");
            printf("%s(%d): %s - register success\r\n", __FILE__, __LINE__, __FUNCTION__);
        }
    }

    // 构建并发送响应
    im::MessageContainer response;
    response.set_cmd(msg_type_register);
    response.set_seq(m_seq);
    response.set_payload(rsp.SerializeAsString());

    m_codec->send(conn, response);
}

// ============================================================
// 登录响应 (cmd=1002)
// 请求: LoginReq { username, password, clienttype, status }
// 响应: LoginRsp { code, msg, UserInfo }
// ============================================================
void ClientSession::OnLoginResponse(const TcpConnectionPtr& conn,
                                    const im::MessageContainer& msg)
{
    im::LoginReq req;
    im::LoginRsp rsp;

    if (!req.ParseFromString(msg.payload()))
    {
        rsp.set_code(101);
        rsp.set_msg("protobuf parse failed!");
    }
    else if (req.username().empty() || req.password().empty())
    {
        rsp.set_code(102);
        rsp.set_msg("data field missing!");
    }
    else
    {
        std::string username = req.username();
        std::string password = req.password();

        // 查找用户
        if (!Singleton<UserManager>::instance().GetUserInfoUsername(username, m_user))
        {
            rsp.set_code(103);
            rsp.set_msg("user is not exist or password is incorrect!");
        }
        else if (password != m_user->password)
        {
            rsp.set_code(104);
            rsp.set_msg("user is not exist or password is incorrect!");
        }
        else
        {
            // 登录成功，填充用户信息
            rsp.set_code(0);
            rsp.set_msg("ok");

            im::UserInfo* userInfo = rsp.mutable_user();
            userInfo->set_userid(m_user->userid);
            userInfo->set_username(m_user->username);
            userInfo->set_nickname(m_user->nickname);
            userInfo->set_facetype(m_user->facetype);
            userInfo->set_customface(m_user->customface);
            userInfo->set_gender(m_user->gender);
            userInfo->set_birthday(m_user->birthday);
            userInfo->set_signature(m_user->signature);
            userInfo->set_address(m_user->address);
            userInfo->set_phonenumber(m_user->phonenumber);
            userInfo->set_mail(m_user->mail);

            m_user->status = 1;  // 设置为在线
        }
    }

    // 发送登录响应
    im::MessageContainer response;
    response.set_cmd(msg_type_login);
    response.set_seq(m_seq);
    response.set_payload(rsp.SerializeAsString());
    m_codec->send(conn, response);

    // 登录成功后推送离线消息
    if (rsp.code() == 0)
    {
        // 推送通知消息（缓存的序列化数据通过sendRaw发送）
        std::list<NotifyMsgCache> listNotifyCache;
        Singleton<MsgCacheManager>::instance().GetNotifyMsgCache(m_user->userid, listNotifyCache);
        for (const auto& iter : listNotifyCache)
        {
            // 缓存的notifymsg是MessageContainer序列化后的字节串
            m_codec->sendRaw(conn, iter.notifymsg);
        }

        // 推送聊天消息
        std::list<ChatMsgCache> listChatCache;
        Singleton<MsgCacheManager>::instance().GetChatMsgCache(m_user->userid, listChatCache);
        for (const auto& iter : listChatCache)
        {
            m_codec->sendRaw(conn, iter.chatmsg);
        }

        // 推送用户上线状态给所有好友
        std::list<UserPtr> friends;
        Singleton<UserManager>::instance().GetFriendInfoByUserID(m_user->userid, friends);
        IMSer& imserver = Singleton<IMSer>::instance();
        for (const auto& iter : friends)
        {
            ClientSessionPtr targetSession = imserver.GetSessionByID(iter->userid);
            if (targetSession)
            {
                printf("%s(%d): %s userid %d target %d\r\n",
                    __FILE__, __LINE__, __FUNCTION__, m_user->userid, targetSession->UserID());
                targetSession->SendUserStatusChangeMsg(m_user->userid, 1);
            }
        }
    }

    printf("%s(%d) : %s userid %d\r\n", __FILE__, __LINE__, __FUNCTION__, m_user->userid);
}

// ============================================================
// 获取好友列表 (cmd=1003)
// 响应: FriendListRsp { code, msg, friends[] }
// ============================================================
void ClientSession::OnGetFriendListResponse(const TcpConnectionPtr& conn,
                                            const im::MessageContainer& msg)
{
    im::FriendListRsp rsp;
    rsp.set_code(0);
    rsp.set_msg("ok");

    std::list<UserPtr> lstFriend;
    Singleton<UserManager>::instance().GetFriendInfoByUserID(m_user->userid, lstFriend);

    for (const auto& iter : lstFriend)
    {
        im::UserInfo* f = rsp.add_friends();
        f->set_userid(iter->userid);
        f->set_username(iter->username);
        f->set_nickname(iter->nickname);
        f->set_facetype(iter->facetype);
        f->set_customface(iter->customface);
        f->set_gender(iter->gender);
        f->set_birthday(iter->birthday);
        f->set_signature(iter->signature);
        f->set_address(iter->address);
        f->set_phonenumber(iter->phonenumber);
        f->set_mail(iter->mail);
        f->set_clienttype(1);
        f->set_status(iter->status ? 1 : 0);
    }

    im::MessageContainer response;
    response.set_cmd(msg_type_getofriendlist);
    response.set_seq(m_seq);
    response.set_payload(rsp.SerializeAsString());

    m_codec->send(conn, response);
    printf("%s(%d) : %s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

// ============================================================
// 查找用户 (cmd=1004)
// 请求: FindUserReq { type, keyword }
// 响应: FindUserRsp { code, msg, UserInfo }
// ============================================================
void ClientSession::OnFindUserResponse(const TcpConnectionPtr& conn,
                                       const im::MessageContainer& msg)
{
    im::FindUserReq req;
    im::FindUserRsp rsp;

    if (!req.ParseFromString(msg.payload()))
    {
        rsp.set_code(101);
        rsp.set_msg("protobuf parse failed!");
    }
    else
    {
        UserPtr user;

        if (req.type() == "username")
        {
            Singleton<UserManager>::instance().GetUserInfoUsername(req.keyword(), user);
        }
        else if (req.type() == "userid")
        {
            int32_t userid = std::stoi(req.keyword());
            user = Singleton<UserManager>::instance().GetUserByID(userid);
        }

        if (!user)
        {
            rsp.set_code(103);
            rsp.set_msg("user not found!");
        }
        else if (user->userid >= GROUPID_BOUNDARY)
        {
            rsp.set_code(105);
            rsp.set_msg("cannot search for group!");
        }
        else
        {
            rsp.set_code(0);
            rsp.set_msg("ok");

            im::UserInfo* userInfo = rsp.mutable_user();
            userInfo->set_userid(user->userid);
            userInfo->set_username(user->username);
            userInfo->set_nickname(user->nickname);
            userInfo->set_facetype(user->facetype);
            userInfo->set_customface(user->customface);
            userInfo->set_gender(user->gender);
            userInfo->set_birthday(user->birthday);
            userInfo->set_signature(user->signature);
            userInfo->set_address(user->address);
            userInfo->set_phonenumber(user->phonenumber);
            userInfo->set_mail(user->mail);
            userInfo->set_status(user->status);
        }
    }

    im::MessageContainer response;
    response.set_cmd(msg_type_finduser);
    response.set_seq(m_seq);
    response.set_payload(rsp.SerializeAsString());

    m_codec->send(conn, response);
    printf("%s(%d): %s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

// ============================================================
// 操作好友 (cmd=1005)
// 请求: OperateFriendReq { type(1=add,2=delete), friendid }
// 响应: CommonRsp { code, msg }
// ============================================================
void ClientSession::OnOperateFriendResponse(const TcpConnectionPtr& conn,
                                            const im::MessageContainer& msg)
{
    im::OperateFriendReq req;
    im::CommonRsp rsp;

    if (!req.ParseFromString(msg.payload()))
    {
        rsp.set_code(101);
        rsp.set_msg("protobuf parse failed!");
    }
    else
    {
        int32_t friendid = req.friendid();
        UserManager& userMgr = Singleton<UserManager>::instance();

        if (req.type() == 1)  // 添加好友
        {
            // 不允许将群组添加为好友
            if (friendid >= GROUPID_BOUNDARY)
            {
                rsp.set_code(106);
                rsp.set_msg("cannot add group as friend!");
            }
            else if (userMgr.MakeFriendRelationship(
                (m_user->userid < friendid) ? m_user->userid : friendid,
                (m_user->userid < friendid) ? friendid : m_user->userid))
            {
                rsp.set_code(0);
                rsp.set_msg("ok");
                // 通知被添加方刷新好友列表
                IMSer& imserver = Singleton<IMSer>::instance();
                ClientSessionPtr targetSession = imserver.GetSessionByID(friendid);
                if (targetSession)
                {
                    targetSession->SendUserStatusChangeMsg(m_user->userid, 4);  // type=4: 被添加好友
                }
            }
            else
            {
                rsp.set_code(100);
                rsp.set_msg("add friend failed!");
            }
        }
        else if (req.type() == 2)  // 删除好友
        {
            if (userMgr.DeleteFriendToUser(m_user->userid, friendid))
            {
                rsp.set_code(0);
                rsp.set_msg("ok");
                // 通知被删除方
                DeleteFriend(conn, friendid);
            }
            else
            {
                rsp.set_code(100);
                rsp.set_msg("delete friend failed!");
            }
        }
        else if (req.type() == 3)  // 邀请入群
        {
            int32_t groupid = msg.target_id();
            if (groupid >= GROUPID_BOUNDARY)
            {
                int32_t smallid = (friendid < groupid) ? friendid : groupid;
                int32_t greatid = (friendid < groupid) ? groupid : friendid;
                if (userMgr.MakeFriendRelationship(smallid, greatid))
                {
                    rsp.set_code(0);
                    rsp.set_msg("ok");
                }
                else
                {
                    rsp.set_code(100);
                    rsp.set_msg("invite to group failed!");
                }
            }
            else
            {
                rsp.set_code(104);
                rsp.set_msg("invalid group id!");
            }
        }
        else
        {
            rsp.set_code(103);
            rsp.set_msg("unknown operation type!");
        }
    }

    im::MessageContainer response;
    response.set_cmd(msg_type_operatefriend);
    response.set_seq(m_seq);
    response.set_payload(rsp.SerializeAsString());

    m_codec->send(conn, response);
    printf("%s(%d): %s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

// ============================================================
// 更新用户信息 (cmd=1007)
// 请求: UpdateUserInfoReq { nickname, facetype, customface, gender, birthday, ... }
// 响应: CommonRsp { code, msg }
// ============================================================
void ClientSession::OnUpdateUserInfoResponse(const TcpConnectionPtr& conn,
                                             const im::MessageContainer& msg)
{
    im::UpdateUserInfoReq req;
    im::CommonRsp rsp;

    if (!req.ParseFromString(msg.payload()))
    {
        rsp.set_code(101);
        rsp.set_msg("protobuf parse failed!");
    }
    else
    {
        // 构造新用户信息，protobuf默认值表示未传（保留原值）
        // 注意：protobuf3中基础类型默认值为0/空字符串，需区分"未设置"和"设置为空"
        User newinfo;
        newinfo.nickname    = req.nickname().empty()    ? m_user->nickname    : req.nickname();
        newinfo.facetype    = req.facetype()            ? req.facetype()     : m_user->facetype;
        newinfo.customface  = req.customface().empty()  ? m_user->customface  : req.customface();
        newinfo.gender      = req.gender()              ? req.gender()       : m_user->gender;
        newinfo.birthday    = req.birthday()            ? req.birthday()     : m_user->birthday;
        newinfo.signature   = req.signature().empty()   ? m_user->signature   : req.signature();
        newinfo.address     = req.address().empty()     ? m_user->address     : req.address();
        newinfo.phonenumber = req.phonenumber().empty() ? m_user->phonenumber : req.phonenumber();
        newinfo.mail        = req.mail().empty()        ? m_user->mail        : req.mail();

        if (Singleton<UserManager>::instance().UpdateUserInfo(m_user->userid, newinfo))
        {
            // 同步更新会话缓存
            m_user->nickname    = newinfo.nickname;
            m_user->facetype    = newinfo.facetype;
            m_user->customface  = newinfo.customface;
            m_user->gender      = newinfo.gender;
            m_user->birthday    = newinfo.birthday;
            m_user->signature   = newinfo.signature;
            m_user->address     = newinfo.address;
            m_user->phonenumber = newinfo.phonenumber;
            m_user->mail        = newinfo.mail;

            rsp.set_code(0);
            rsp.set_msg("ok");
        }
        else
        {
            rsp.set_code(100);
            rsp.set_msg("update user info failed!");
        }
    }

    im::MessageContainer response;
    response.set_cmd(msg_type_updateuserinfo);
    response.set_seq(m_seq);
    response.set_payload(rsp.SerializeAsString());

    m_codec->send(conn, response);
    printf("%s(%d): %s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

// ============================================================
// 修改密码 (cmd=1008)
// 请求: ModifyPasswordReq { oldpassword, newpassword }
// 响应: CommonRsp { code, msg }
// ============================================================
void ClientSession::OnModifyPasswordResponse(const TcpConnectionPtr& conn,
                                             const im::MessageContainer& msg)
{
    im::ModifyPasswordReq req;
    im::CommonRsp rsp;

    if (!req.ParseFromString(msg.payload()))
    {
        rsp.set_code(101);
        rsp.set_msg("protobuf parse failed!");
    }
    else if (req.oldpassword().empty() || req.newpassword().empty())
    {
        rsp.set_code(102);
        rsp.set_msg("invalid parameter!");
    }
    else if (req.oldpassword() != m_user->password)
    {
        rsp.set_code(103);
        rsp.set_msg("old password is incorrect!");
    }
    else if (Singleton<UserManager>::instance().ModifyUserPassword(m_user->userid, req.newpassword()))
    {
        m_user->password = req.newpassword();
        rsp.set_code(0);
        rsp.set_msg("ok");
    }
    else
    {
        rsp.set_code(100);
        rsp.set_msg("modify password failed!");
    }

    im::MessageContainer response;
    response.set_cmd(msg_type_modifypassword);
    response.set_seq(m_seq);
    response.set_payload(rsp.SerializeAsString());

    m_codec->send(conn, response);
    printf("%s(%d): %s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

// ============================================================
// 创建群组 (cmd=1009)
// 请求: CreateGroupReq { groupname }
// 响应: CreateGroupRsp { code, msg, groupid, groupname }
// ============================================================
void ClientSession::OnCreateGroupResponse(const TcpConnectionPtr& conn,
                                          const im::MessageContainer& msg)
{
    im::CreateGroupReq req;
    im::CreateGroupRsp rsp;

    if (!req.ParseFromString(msg.payload()))
    {
        rsp.set_code(101);
        rsp.set_msg("protobuf parse failed!");
    }
    else if (req.groupname().empty())
    {
        rsp.set_code(102);
        rsp.set_msg("invalid parameter!");
    }
    else
    {
        int32_t groupid = 0;
        UserManager& userMgr = Singleton<UserManager>::instance();

        if (userMgr.AddGroup(req.groupname().c_str(), m_user->userid, groupid))
        {
            // 群主自动加入群组
            OnAddGroupResponse(conn, groupid);

            rsp.set_code(0);
            rsp.set_msg("ok");
            rsp.set_groupid(groupid);
            rsp.set_groupname(req.groupname());
        }
        else
        {
            rsp.set_code(100);
            rsp.set_msg("create group failed!");
        }
    }

    im::MessageContainer response;
    response.set_cmd(msg_type_creategroup);
    response.set_seq(m_seq);
    response.set_payload(rsp.SerializeAsString());

    m_codec->send(conn, response);
    printf("%s(%d): %s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

// ============================================================
// 获取群成员 (cmd=1010)
// 请求: GetGroupMembersReq { groupid }
// 响应: GetGroupMembersRsp { code, msg, members[] }
// ============================================================
void ClientSession::OnGetGroupMembersResponse(const TcpConnectionPtr& conn,
                                              const im::MessageContainer& msg)
{
    im::GetGroupMembersReq req;
    im::GetGroupMembersRsp rsp;

    if (!req.ParseFromString(msg.payload()))
    {
        rsp.set_code(101);
        rsp.set_msg("protobuf parse failed!");
    }
    else
    {
        int32_t groupid = req.groupid();

        std::list<UserPtr> members;
        if (Singleton<UserManager>::instance().GetFriendInfoByUserID(groupid, members))
        {
            rsp.set_code(0);
            rsp.set_msg("ok");

            for (const auto& member : members)
            {
                im::UserInfo* m = rsp.add_members();
                m->set_userid(member->userid);
                m->set_username(member->username);
                m->set_nickname(member->nickname);
                m->set_facetype(member->facetype);
                m->set_customface(member->customface);
                m->set_gender(member->gender);
                m->set_birthday(member->birthday);
                m->set_signature(member->signature);
                m->set_address(member->address);
                m->set_phonenumber(member->phonenumber);
                m->set_mail(member->mail);
                m->set_status(member->status);
            }
        }
        else
        {
            rsp.set_code(100);
            rsp.set_msg("get group members failed!");
        }
    }

    im::MessageContainer response;
    response.set_cmd(msg_type_getgroupmembers);
    response.set_seq(m_seq);
    response.set_payload(rsp.SerializeAsString());

    m_codec->send(conn, response);
    printf("%s(%d): %s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

// ============================================================
// 获取聊天历史 (cmd=1011)
// 请求: ChatHistoryReq { targetid }
// 响应: ChatHistoryRsp { code, msg, targetid, messages[] }
// ============================================================
void ClientSession::OnGetChatHistoryResponse(const TcpConnectionPtr& conn,
                                              const im::MessageContainer& msg)
{
    im::ChatHistoryReq req;
    im::ChatHistoryRsp rsp;

    if (!req.ParseFromString(msg.payload()))
    {
        rsp.set_code(101);
        rsp.set_msg("protobuf parse failed!");
    }
    else
    {
        int32_t targetid = req.targetid();
        std::list<im::ChatMsg> messages;
        UserManager& userMgr = Singleton<UserManager>::instance();

        if (userMgr.GetChatHistory(m_user->userid, targetid, messages, 50))
        {
            rsp.set_code(0);
            rsp.set_msg("ok");
            rsp.set_targetid(targetid);
            for (const auto& m : messages)
            {
                *rsp.add_messages() = m;
            }
        }
        else
        {
            rsp.set_code(100);
            rsp.set_msg("get chat history failed!");
        }
    }

    im::MessageContainer response;
    response.set_cmd(msg_type_getchathistory);
    response.set_seq(m_seq);
    response.set_payload(rsp.SerializeAsString());

    m_codec->send(conn, response);
    printf("%s(%d): %s, userid=%d
", __FILE__, __LINE__, __FUNCTION__, m_user->userid);
}


// ============================================================
// 单聊消息处理 (cmd=1100)
// MessageContainer中包含 target_id 和 ChatMsg载荷
// ChatMsg { senderid, targetid, content, timestamp }
// ============================================================
void ClientSession::OnChatResponse(const TcpConnectionPtr& conn,
                                   const im::MessageContainer& msg)
{
    m_target = msg.target_id();

    // 构建要转发/缓存的完整MessageContainer
    im::MessageContainer forwardMsg;
    forwardMsg.set_cmd(msg_type_chat);
    forwardMsg.set_seq(m_seq);
    forwardMsg.set_target_id(m_target);
    forwardMsg.set_payload(msg.payload());  // 转发原始ChatMsg载荷

    // 解析ChatMsg用于记录
    im::ChatMsg chatMsg;
    chatMsg.ParseFromString(msg.payload());

    printf("%s(%d): %s target:%d cur:%d\r\n",
        __FILE__, __LINE__, __FUNCTION__, m_target, m_user->userid);
    std::cout << chatMsg.content() << std::endl;

    UserManager& userMgr = Singleton<UserManager>::instance();

    // 写入消息记录到数据库
    if (!userMgr.SaveChatMsgToDb(m_user->userid, m_target, chatMsg.content()))
    {
        LOG_ERROR << "Write chat msg to db error, senderid = " << m_user->userid
                  << ", targetid = " << m_target << ", chatmsg:" << chatMsg.content();
    }

    IMSer& imserver = Singleton<IMSer>::instance();
    MsgCacheManager& msgCacheMgr = Singleton<MsgCacheManager>::instance();

    // 单聊消息
    if (m_target < GROUPID_BOUNDARY)
    {
        ClientSessionPtr targetSession = imserver.GetSessionByID(m_target);
        if (!targetSession)
        {
            // 目标用户不在线，缓存消息
            msgCacheMgr.AddChatMsgCache(m_target, forwardMsg.SerializeAsString());
            return;
        }

        targetSession->SendContainer(forwardMsg);
        return;
    }

    // 群聊消息：遍历群成员转发（跳过发送者本人）
    std::list<UserPtr> friends;
    userMgr.GetFriendInfoByUserID(m_target, friends);
    for (const auto& iter : friends)
    {
        if (iter->userid == m_user->userid)
            continue;  // 不转发给发送者，避免客户端重复显示
        ClientSessionPtr targetSession = imserver.GetSessionByID(iter->userid);
        if (!targetSession)
        {
            msgCacheMgr.AddChatMsgCache(iter->userid, forwardMsg.SerializeAsString());
            continue;
        }
        targetSession->SendContainer(forwardMsg);
    }

    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
}

// ============================================================
// 群发消息处理 (cmd=1101)
// MessageContainer中 targets 字段包含群发目标列表(JSON数组)
// payload为ChatMsg
// ============================================================
void ClientSession::OnMultiChatResponse(const TcpConnectionPtr& conn,
                                        const im::MessageContainer& msg)
{
    // targets字段在MessageContainer中，这里使用JSON数组解析（保持与旧协议兼容）
    m_targets = msg.targets();

    // 解析MultiChatTargets获取目标列表
    im::MultiChatTargets multiTargets;
    if (!multiTargets.ParseFromString(msg.payload()))
    {
        LOG_ERROR << "invalid protobuf: MultiChatTargets parse failed, userid="
                  << m_user->userid << ", client: " << conn->peerAddress().toIpPort();
        return;
    }

    for (int i = 0; i < multiTargets.targets_size(); ++i)
    {
        m_target = multiTargets.targets(i);

        // 构建转发消息
        im::MessageContainer forwardMsg;
        forwardMsg.set_cmd(msg_type_chat);
        forwardMsg.set_seq(m_seq);
        forwardMsg.set_target_id(m_target);
        forwardMsg.set_payload(multiTargets.content());

        OnChatResponse(conn, forwardMsg);
    }

    printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
    LOG_INFO << "Send to client: cmd=msg_type_multichat, targets count: "
             << multiTargets.targets_size() << ", userid: "
             << m_user->userid << ", client: " << conn->peerAddress().toIpPort();
}

// ============================================================
// 删除好友通知
// ============================================================
void ClientSession::DeleteFriend(const TcpConnectionPtr& conn, int32_t friendid)
{
    IMSer& imserver = Singleton<IMSer>::instance();
    ClientSessionPtr targetSession = imserver.GetSessionByID(friendid);
    if (targetSession)
    {
        targetSession->SendUserStatusChangeMsg(m_user->userid, 3);  // 3=被删除好友
    }
    printf("%s(%d): %s, userid=%d, friendid=%d\r\n",
        __FILE__, __LINE__, __FUNCTION__, m_user->userid, friendid);
}

// ============================================================
// 加入群组
// ============================================================
void ClientSession::OnAddGroupResponse(const TcpConnectionPtr& conn, int32_t groupid)
{
    int32_t smallid = (m_user->userid < groupid) ? m_user->userid : groupid;
    int32_t greatid = (m_user->userid < groupid) ? groupid : m_user->userid;
    Singleton<UserManager>::instance().MakeFriendRelationship(smallid, greatid);
    printf("%s(%d): %s, userid=%d, groupid=%d\r\n",
        __FILE__, __LINE__, __FUNCTION__, m_user->userid, groupid);
}

// ============================================================
// 发送用户状态变更通知
// 通知: UserStatusChangeNotify { type, userid }
// ============================================================
void ClientSession::SendUserStatusChangeMsg(int32_t userid, int type)
{
    im::UserStatusChangeNotify notify;
    notify.set_type(type);
    notify.set_userid(userid);

    im::MessageContainer container;
    container.set_cmd(msg_type_userstatuschange);
    container.set_seq(0);
    container.set_payload(notify.SerializeAsString());

    m_codec->send(m_conn, container);
    printf("%s(%d): %s, userid=%d, type=%d\r\n",
        __FILE__, __LINE__, __FUNCTION__, userid, type);
}
