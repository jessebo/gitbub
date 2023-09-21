#include "RedisConnPool.h"
#include <assert.h>

using namespace std;

RedisConnPool::RedisConnPool(int nPoolSize, int heartCheckSecs)
{
	m_nPoolSize = nPoolSize;
	m_nHeartCheckSecs = heartCheckSecs;

	assert(nPoolSize > 0 && heartCheckSecs > 0);
}

RedisConnPool::~RedisConnPool()
{
	std::unordered_map<RedisClient*, time_t>::iterator iter = m_mapConnCheckLast.begin();
	for( ; iter != m_mapConnCheckLast.end(); iter++)
	{
		delete (iter->first);
	}

	m_mapConnCheckLast.clear();
}

int RedisConnPool::InitRedisConn(const std::string& strHost, int nPort, const std::string& strPass, int timeoutMillSecs)
{
	if(m_listConn.size() > 0)
	{
		std::cout<<"Error: RedisConnPool::InitRedisConn: RedisConnPool already init, can not init repeat!"<<std::endl;
		return -1;
	}
	if(strHost.empty() || nPort < 1024 || timeoutMillSecs < 1)
	{
		std::cout<<"Error: RedisConnPool::InitRedisConn: input param is invalid!"<<std::endl;
		return -2;
	}

	int nErrConn = 0;
	for(int Idx = 0; Idx < m_nPoolSize; Idx++)
	{
		RedisClient* pRedisCli = new RedisClient();
		if(NULL != pRedisCli)
		{
			nErrConn = pRedisCli->ConnectWithTimeout(strHost, nPort, timeoutMillSecs, strPass);
			if (nErrConn)
			{
				string  strErr = pRedisCli->GetErrorInfo();
				std::cout<<"Error: RedisConnPool::InitRedisConn: failed to connect redis, error : "<<strErr<<std::endl;

				std::list<RedisClient*>::iterator iter = m_listConn.begin();
				for(; m_listConn.end() != iter; ++iter)
				{
					delete (*iter);
				}
				m_listConn.clear();
				m_mapConnCheckLast.clear();

				delete pRedisCli;
				
				return -3;
			}

			m_listConn.push_back(pRedisCli);
			time_t curTm = time(NULL);
			m_mapConnCheckLast.insert(std::make_pair(pRedisCli, curTm));
		}
	}

	return 0;
}

RedisClient*  RedisConnPool::GetRedisConn()
{
	RedisClient* pRedisConn = NULL;
	{
		std::unique_lock<std::mutex> lck(m_mutex);

		int nFreeConnNum = 0;
		std::cout<<"redis connection size:"<<m_listConn.size()<<std::endl;
		while( (nFreeConnNum = m_listConn.size()) == 0)
		{
			m_condition.wait(lck);
		}

		pRedisConn = m_listConn.front();
		m_listConn.pop_front();		
	}
	if(NULL != pRedisConn)
	{
		time_t curTm = time(NULL);
		time_t lastCheckTm = curTm;

		std::unordered_map<RedisClient*, time_t>::iterator iterFind = m_mapConnCheckLast.find(pRedisConn);
		if(iterFind != m_mapConnCheckLast.end())
		{
			lastCheckTm = iterFind->second;

			if(curTm - lastCheckTm >= m_nHeartCheckSecs)  //. need check status
			{
				if( (false == pRedisConn->IsConnected()) || (false == pRedisConn->CheckStatus()))
				{
					int nErr = pRedisConn->ReConnect();
					if (0 != nErr)
					{
						std::cout<<"failed to reconnect redis server, error : "<<pRedisConn->GetErrorInfo()<<std::endl;
					}
				}
				iterFind->second = curTm;

				//std::cout<<"---RedisConnPool::GetRedisConn CheckStatus OK, RedisObj: "<<pRedisConn<<", thread id: "<<std::this_thread::get_id()<<std::endl;
			}
		}
	}

	return pRedisConn;
}

void  RedisConnPool::FreeRedisConn(RedisClient* pRedisConn)
{
	if(NULL == pRedisConn)
	{
		std::cout<<"Error: RedisConnPool::FreeRedisConn: free RedisClient object is NULL."<<std::endl;
		abort();
		return ;
	}

	std::unique_lock<std::mutex> lck(m_mutex);
	m_listConn.push_back(pRedisConn);
	m_condition.notify_one();
}

