#ifndef _REDIS_CONN_POOL_192038_H
#define _REDIS_CONN_POOL_192038_H

#include "RedisClient.h"
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


class RedisConnPool
{
public:
	RedisConnPool(int nPoolSize, int heartCheckSecs);
	~RedisConnPool();
private:
	RedisConnPool()
	{
	};

public:
	int InitRedisConn(const std::string& strHost, int nPort, const std::string& strPass, int timeoutMillSecs);

	RedisClient*  GetRedisConn();
	void  FreeRedisConn(RedisClient* pRedisConn);


protected:
	std::list<RedisClient*>	m_listConn;

	int 		m_nPoolSize;
	int 		m_nHeartCheckSecs;

	std::mutex 					m_mutex;
	std::condition_variable 	m_condition;

	std::unordered_map<RedisClient*, time_t>	m_mapConnCheckLast;
};



#endif
