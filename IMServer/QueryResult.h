#include <unistd.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <errno.h>
#include <memory>
#include "base/Logging.h"

using namespace std;

class QueryResult
{
public:
	QueryResult(MYSQL_RES* result, uint32_t rowcount, uint32_t cloumncount);
	~QueryResult();
};

typedef shared_ptr<QueryResult> QueryResultPtr;//定义一个智能指针类型，方便管理QueryResult对象的生命周期