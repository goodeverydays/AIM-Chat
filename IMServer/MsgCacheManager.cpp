#include "MsgCacheManager.h"
#include "base/Logging.h"

MsgCacheManager::MsgCacheManager()
{

}

MsgCacheManager::~MsgCacheManager()
{

}

bool MsgCacheManager::AddNotifyMsgCache(int32_t userid, const std::string& cache)
{
	std::lock_guard<std::mutex> guard(m_mtNotifyMsgCache);
	NotifyMsgCache nc;
	nc.userid = userid;
	nc.notifymsg.append(cache.c_str(), cache.length());
	m_listNotifyMsgCache.push_back(nc);
	LOG_INFO << "append notify msg to cache, userid: " << userid << ", m_mapNotifyMsgCache.size() : " << m_listNotifyMsgCache.size() <<
		", cache length : " << cache.length();//记录日志，输出添加到通知消息缓存的信息，包括用户ID、当前通知消息缓存的大小以及添加的消息内容的长度


	//TODO: 存盘或写入数据库以防止程序奔溃丢失

	return true;
}

void MsgCacheManager::GetNotifyMsgCache(int32_t userid, std::list<NotifyMsgCache>& cached)
{

}

bool MsgCacheManager::AddChatMsgCache(int32_t userid, const std::string& cache)
{
	std::lock_guard<std::mutex> guard(m_mtChatMsgCache);
	ChatMsgCache c;
	c.userid = userid;
	c.chatmsg.append(cache.c_str(), cache.length());
	//队列的插入速度是非常快的
	m_listChatMsgCache.push_back(c);
	LOG_INFO << "append chat msg to cache, userid: " << userid << ", m_mapChatMsgCache.size() : " << m_listChatMsgCache.size() <<
		", cache length : " << cache.length();//记录日志，输出添加到通知消息缓存的信息，包括用户ID、当前通知消息缓存的大小以及添加的消息内容的长度
	return true;
}

void MsgCacheManager::GetChatMsgCache(int32_t userid, std::list<NotifyMsgCache>& cached)
{
	std::lock_guard<std::mutex> guard(m_mtChatMsgCache);
	for (auto iter = m_listChatMsgCache.begin(); iter != m_listChatMsgCache.end();)
	{
		if (iter->userid == userid)
		{
			cache.push_back(*iter);
			iter = m_listChatMsgCache.erase(iter);
		}
		else {
			iter++;
		}
	}

	LOG_INFO << "get chat msg  cache, no cache,  userid: " << userid << ", m_mapChatMsgCache.size() : " << m_listChatMsgCache.size() <<
		", cache size : " << cached.size();//记录日志，输出从聊天消息缓存中获取的信息，包括用户ID、当前聊天消息缓存的大小以及获取的消息内容的数量
}
