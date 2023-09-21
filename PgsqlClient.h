#ifndef PGSQL_CLIENT_H
#define PGSQL_CLIENT_H

#include <string.h>
#include <vector>
#include <string>
#include <mutex>
#include <map>
#include <pqxx/pqxx>

class PgsqlClient
{
public:
	PgsqlClient();
	~PgsqlClient();
public:
	// 初始化psql的客户端，dbinfo为数据库连接信息
	int initPgsqlClint(const std::string& dbinfo);
	// 获取错误描述
	std::string getErrorInfo() { return errordesc_; }
	// 需要返回值的查询sql
	pqxx::result querySql(const std::string& strsql);
	// 需要返回值的查询sql,带错误信息
	int querySql(const std::string& strsq, pqxx::result &result_);
	// 不需要返回值的查询sql
	int execSql(const std::string& strsql);
	//执行多个sql
	int execSqlList(const std::vector<std::string>sql_vec);

	// 生成POLYGON的字符串
	std::string genPolygon(int srid, std::string input);

	pqxx::connection* getConnection();

private:
	pqxx::connection* pqconnection_;
	int					errorcode_;
	std::string			errordesc_;
};

#endif

