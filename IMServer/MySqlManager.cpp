#include "MySqlManager.h"

MySqlManager::MySqlManager()
{
	//用户
	sTableInfo info;
	info.sName = "t_user";
	info.mapField["f_id"] = { "f_id", "bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID'","bigint(20)" };
	info.mapField["f_user_id"] = { "f_user_id", "bigint(20) NOT NULL COMMENT '用户ID'", "bigint(20)" };
	info.mapField["f_username"] = { "f_username", "varchar(64) NOT NULL COMMENT '用户名'", "varchar(64)" };
	info.mapField["f_nickname"] = { "f_nickname", "varchar(64) NOT NULL COMMENT '用户昵称'", "varchar(64)" };
	info.mapField["f_password"] = { "f_password", "varchar(64) NOT NULL COMMENT '用户密码'", "varchar(64)" };
	info.mapField["f_facetype"] = { "f_facetype", "int(10) DEFAULT 0 COMMENT '用户头像类型'", "int(10)" };
	info.mapField["f_customface"] = { "f_customface", "varchar(32) DEFAULT NULL COMMENT '自定义头像名'", "varchar(32)" };
	info.mapField["f_customfacefmt"] = { "f_customfacefmt", "varchar(6) DEFAULT NULL COMMENT '自定义头像格式'", "varchar(6)" };
	info.mapField["f_gender"] = { "f_gender", "int(2)  DEFAULT 0 COMMENT '性别'", "int(2)" };
	info.mapField["f_birthday"] = { "f_birthday", "bigint(20)  DEFAULT 19900101 COMMENT '生日'", "bigint(20)" };
	info.mapField["f_signature"] = { "f_signature", "varchar(256) DEFAULT NULL COMMENT '地址'", "varchar(256)" };
	info.mapField["f_address"] = { "f_address", "varchar(256) DEFAULT NULL COMMENT '地址'", "varchar(256)" };
	info.mapField["f_phonenumber"] = { "f_phonenumber", "varchar(64) DEFAULT NULL COMMENT '电话'", "varchar(64)" };
	info.mapField["f_mail"] = { "f_mail", "varchar(256) DEFAULT NULL COMMENT '邮箱'", "varchar(256)" };
	info.mapField["f_owner_id"] = { "f_owner_id", "bigint(20) DEFAULT 0 COMMENT '群账号群主userid'", "bigint(20)" };
	info.mapField["f_register_time"] = { "f_register_time", "datetime NOT NULL COMMENT '注册时间'", "datetime" };
	info.mapField["f_remark"] = { "f_remark", "varchar(64) NULL COMMENT '备注'", "varchar(64)" };
	info.mapField["f_update_time"] = { "f_update_time", "timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间'", "timestamp" };
	info.sKey = "PRIMARY KEY (f_user_id), INDEX f_user_id (f_user_id), KEY  f_id  ( f_id )";
	m_mapTable.insert(TablePair(info.sName, info));

	//聊天内容
	info.mapField.clear();
	info.sName = "t_chatmsg";
	info.mapField["f_id"] = { "f_id", "bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID'", "bigint(20)" };
	info.mapField["f_senderid"] = { "f_senderid", "bigint(20) NOT NULL COMMENT '发送者id'", "bigint(20)" };
	info.mapField["f_targetid"] = { "f_targetid", "bigint(20) NOT NULL COMMENT '接收者id'", "bigint(20)" };
	info.mapField["f_msgcontent"] = { "f_msgcontent", "BLOB NOT NULL COMMENT '聊天内容'", "BLOB" };
	info.mapField["f_create_time"] = { "f_create_time", "timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间'", "timestamp" };
	info.mapField["f_remark"] = { "f_remark", "varchar(64) NULL COMMENT '备注'", "varchar(64)" };
	info.sKey = "PRIMARY KEY (f_id), INDEX f_id (f_id)";
	m_mapTable.insert(TablePair(info.sName, info));

	//用户关系
	info.mapField.clear();
	info.sName = "t_user_relationship";
	info.m_strName = "t_user_relationship";
	info.mapField["f_id"] = { "f_id", "bigint(20) NOT NULL AUTO_INCREMENT COMMENT '自增ID'", "bigint(20)" };
	info.mapField["f_user_id1"] = { "f_user_id1", "bigint(20) NOT NULL COMMENT '用户ID'", "bigint(20)" };
	info.mapField["f_user_id2"] = { "f_user_id2", "bigint(20) NOT NULL COMMENT '用户ID'", "bigint(20)" };
	info.mapField["f_remark"] = { "f_remark", "varchar(64) NULL COMMENT '备注'", "varchar(64)" };
	info.mapField["f_create_time"] = { "f_create_time", "timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间'", "timestamp" };
	info.sKey = "PRIMARY KEY (f_id), INDEX f_id (f_id)";
	m_mapTable.insert(TablePair(info.sName, info));
}

MySqlManager::~MySqlManager()
{

}

bool MySqlManager::Init(
	const string& host,
	const string user,
	const string passwd,
	const string dbname,
	unsigned port
)
{
	m_mysql.reset(new MySQLTool());
	if (m_mysql->connect(host, user, passwd, db, port) == false)
	{
		cout << "connect mysql failed!\r\n";
		return false;
	}

	//TODO:数据库在不在， 不在应该创建
	//TODO：表在不在， 不在应该创建
	return true;
}

QueryResultPtr MySqlManager::Query(const string& sql)
{

}

bool MySqlManager::CheckDatabase()
{

}

bool MySqlManager::CheckTable()
{

}

bool MySqlManager::CreateDatabase()
{

}

bool MySqlManager::CreateTable()
{

}

bool MySqlManager::UpdateTable()
{

}
