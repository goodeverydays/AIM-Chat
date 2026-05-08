#include "mysqltool.h"
#include <map>

class MySqlManager 
{
public:
	typedef struct fieldinfo{//
		fieldinfo(const string& name, string& tp, const string& desc)
		{
			sName = name;
			sType = tp;
			sDesc = desc;
		}
		string sName;
		string sType;
		string sDesc;
	}sFieldInfo;

	typedef struct {//表结构
		string sName;
		map<string, sFieldInfo>mapField;
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

	//TODO:业务实现的时候追加
private:
	bool CheckDatabase();
	bool CheckTable();
	bool CreateDatabase();
	bool CreateTable();
	bool UpdateTable();

private:
	map<string, sTableInfo> m_mapTable;//记录
	shared_ptr<MySQLTool> m_mysql;// 智能指针封装
};

typedef std::pair<string, sTableInfo> TablePair;