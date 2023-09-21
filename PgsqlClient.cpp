#include <thread>					// std::thread
#include <iostream>
#include <unistd.h>
#include "pgsql_client.h"

using namespace std;
using namespace pqxx;

PgsqlClient::PgsqlClient() {
	pqconnection_ = nullptr;
	errorcode_ = 0;
	errordesc_.clear();
}

PgsqlClient::~PgsqlClient() {
	if ( pqconnection_!=nullptr &&  pqconnection_->is_open()) {
		pqconnection_->disconnect();
 	}
}

int PgsqlClient::initPgsqlClint(const std::string& dbinfo) {
	int error_code = 1;
	try {
		pqconnection_ = new connection(dbinfo);
		usleep(20 * 1000);
		if (pqconnection_!=nullptr && pqconnection_->is_open())
			return 0;
		else {
			errordesc_ = "open error";
			return 1;
		}
	} catch (const std::exception &e) {
		errordesc_ = e.what();
		error_code = 1;
	}
	return error_code;
}

pqxx::result PgsqlClient::querySql(const std::string& strsql) {
	try {
		/* Create a transactional object. */
		work W(*pqconnection_);
		/* Execute SQL query */
		return W.exec( strsql );
	} catch (const std::exception &e) {
		errordesc_ = e.what();
	}
	return pqxx::result();
}

int PgsqlClient::querySql(const std::string& strsql, pqxx::result &result_)
{
	try {
		/* Create a transactional object. */
		work W(*pqconnection_);
		/* Execute SQL query */
		result_= W.exec( strsql );
		errorcode_ = 0;
	} catch (const std::exception &e) {
		errordesc_ = e.what();
		errorcode_ = 1;
	}
	return errorcode_;
}

int PgsqlClient::execSql(const std::string& strsql) {
	try {
		/* Create a transactional object. */
		work W(*pqconnection_);
		/* Execute SQL query */
		W.exec( strsql );
		W.commit();
		errorcode_ = 0;
	} catch (const std::exception &e) {
		errordesc_ = e.what();
		errorcode_ = 1;
	}
	return errorcode_;
}

int PgsqlClient::execSqlList(const std::vector<std::string>sql_vec)
{
	try {
		/* Create a transactional object. */
		work W(*pqconnection_);
		/* Execute SQL */
		for (auto &strsql:sql_vec)
		{
			W.exec( strsql );
		}

		W.commit();
		errorcode_ = 0;
	} catch (const std::exception &e) {
		errordesc_ = e.what();
		errorcode_ = 1;
	}
	return errorcode_;
}

string PgsqlClient::genPolygon(int srid, std::string input) {
	return "\'SRID=" + to_string(srid) + ";POLYGON((" + input + "))\'::geometry";
}


pqxx::connection* PgsqlClient::getConnection()
{
	return pqconnection_;
}
