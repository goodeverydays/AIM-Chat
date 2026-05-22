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
		", cache length : " << cache.length();//��¼��־��������ӵ�֪ͨ��Ϣ�������Ϣ�������û�ID����ǰ֪ͨ��Ϣ����Ĵ�С�Լ����ӵ���Ϣ���ݵĳ���


	//TODO: ���̻�д�����ݿ��Է�ֹ��������ʧ

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
	//���еĲ����ٶ��Ƿǳ����
	m_listChatMsgCache.push_back(c);
	LOG_INFO << "append chat msg to cache, userid: " << userid << ", m_mapChatMsgCache.size() : " << m_listChatMsgCache.size() <<
		", cache length : " << cache.length();//��¼��־��������ӵ�֪ͨ��Ϣ�������Ϣ�������û�ID����ǰ֪ͨ��Ϣ����Ĵ�С�Լ����ӵ���Ϣ���ݵĳ���
	return true;
}

void MsgCacheManager::GetChatMsgCache(int32_t userid, std::list<ChatMsgCache>& cached)
{
	std::lock_guard<std::mutex> guard(m_mtChatMsgCache);
	for (auto iter = m_listChatMsgCache.begin(); iter != m_listChatMsgCache.end();)
	{
		if (iter->userid == userid)
		{
			cached.push_back(*iter);
			iter = m_listChatMsgCache.erase(iter);
		}
		else {
			iter++;
		}
	}

	LOG_INFO << "get chat msg  cache, no cache,  userid: " << userid << ", m_mapChatMsgCache.size() : " << m_listChatMsgCache.size() <<
		", cache size : " << cached.size();//��¼��־�������������Ϣ�����л�ȡ����Ϣ�������û�ID����ǰ������Ϣ����Ĵ�С�Լ���ȡ����Ϣ���ݵ�����
}
