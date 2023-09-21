#ifndef _PGSQL_CONN_POOL_H
#define _PGSQL_CONN_POOL_H

#include "pgsql_client.h"
#include <string>
#include <vector>
#include <list>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <time.h>
#include <map>
#include <unordered_map>
#include <iostream>
#include <pqxx/pqxx>


class PgsqlConnPool
{
public:
	PgsqlConnPool(int pool_size);
	~PgsqlConnPool();

public:
	int initPgsqlConn(const std::string& dbinfo);
	PgsqlClient*  getPgsqlConn();
	void  freePgsqlConn(PgsqlClient* pq_client);

protected:
	std::list<PgsqlClient*>	m_listConn;

	int 		pool_size_;

	std::mutex 					m_mutex;
	std::condition_variable 	m_condition;
};



#endif
