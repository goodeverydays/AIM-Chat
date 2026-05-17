#include "mysqltool.h"
#include <map>

class MySqlManager 
{
public:
	typedef struct fieldinfo{//字段信息
		fieldinfo() = default;
		fieldinfo(const string& name, const string& tp, const string& desc)
		{
			sName = name;
			sType = tp;
			sDesc = desc;
		}
		string sName;
		string sType;
		string sDesc;
	}sFieldInfo;

	typedef struct {//表结构信息
		string sName;
		map<string, sFieldInfo>mapField;//表结构信息中的字段信息映射表，键为字段名称，值为字段信息，方便在程序中管理和访问字段信息
		string sKey;
	}sTableInfo;
public:
	MySqlManager();
	~MySqlManager();
	bool Init(
		const string& host, 
		const string user, 
		const string passwd, 
		const string dbname, 
		unsigned port = 3306
	);

	QueryResultPtr Query(const string& sql);
	//执行SQL查询语句，返回查询结果的智能指针，方便管理查询结果的生命周期，确保在程序结束时自动释放资源，避免内存泄漏
	bool Execute(const string& sql);
	//执行SQL语句，返回执行结果的布尔值，表示执行是否成功，可以根据需要进行错误处理和日志记录等操作
	
private:
	bool CheckDatabase();
	bool CheckTable(const sTableInfo& info);
	bool CreateDatabase();
	bool CreateTable(const sTableInfo& info);
	bool UpdateTable(const sTableInfo& info);

private:
	map<string, sTableInfo> m_mapTable;//表结构信息的映射表，键为表名，值为表结构信息，方便在程序中管理和访问表结构信息
	shared_ptr<MySQLTool> m_mysql;//MySQL连接工具的智能指针，方便管理MySQL连接的生命周期，确保在程序结束时自动释放资源，避免内存泄漏
};

typedef std::pair<string, MySqlManager::sTableInfo> TablePair;//定义一个键值对类型，方便插入表结构信息到映射表中，键为表名，
typedef std::map<string, MySqlManager::sTableInfo>::iterator TableIter;//定义一个迭代器类型，方便遍历表结构信息的映射表
typedef std::map<string, MySqlManager::sFieldInfo>::iterator FieldIter;//定义一个迭代器类型，方便遍历字段信息的映射表
typedef std::map<string, MySqlManager::sFieldInfo>::const_iterator FieldConstIter;
typedef pair<string, MySqlManager::sFieldInfo> FieldPair;