#include "pgsql_connpool.h"
#include <assert.h>

using namespace std;
using namespace pqxx;

PgsqlConnPool::PgsqlConnPool(int pool_size) {
	pool_size_ = pool_size;
	assert(pool_size > 0);
}

PgsqlConnPool::~PgsqlConnPool() {
	std::list<PgsqlClient*>::iterator iter = m_listConn.begin();
	for(iter = m_listConn.begin(); m_listConn.end() != iter; ++iter)
	{
		delete (*iter);
	}
	m_listConn.clear();
}

int PgsqlConnPool::initPgsqlConn(const std::string& dbinfo) {
	if(m_listConn.size() > 0) {
		std::cout<<"Error: PgsqlConnPool::initPgsqlConn: PgsqlConnPool already init, can not init repeat!"<<std::endl;
		return -1;
	}

	int nErrConn = 0;
	for(int Idx = 0; Idx < pool_size_; Idx++) {
		PgsqlClient* pq_client = new PgsqlClient();
		if(NULL != pq_client) {
			nErrConn = pq_client->initPgsqlClint(dbinfo);
			if (nErrConn) {
				string strErr = pq_client->getErrorInfo();
				std::cout<<"Error: PgsqlConnPool::initPgsqlConn: failed to connect rds, error : "<< strErr <<std::endl;

				std::list<PgsqlClient*>::iterator iter = m_listConn.begin();
				for(; m_listConn.end() != iter; ++iter)
				{
					delete (*iter);
				}
				m_listConn.clear();
				delete pq_client;
				return -3;
			}

			m_listConn.push_back(pq_client);
		}
	}

	return 0;
}

PgsqlClient* PgsqlConnPool::getPgsqlConn() {
	PgsqlClient* pq_client = nullptr;
	std::unique_lock<std::mutex> lck(m_mutex);
	std::cout<<"pgsql size:"<< m_listConn.size()<<std::endl;
	
	int nFreeConnNum = 0;
	while( (nFreeConnNum = m_listConn.size()) == 0)
	{
		m_condition.wait(lck);
	}

	pq_client = m_listConn.front();
	//std::cout<<"get one client,pgsql size:"<< m_listConn.size()<<std::endl;
	m_listConn.pop_front();		

	return pq_client;
}

void  PgsqlConnPool::freePgsqlConn(PgsqlClient* pq_client) {
	if(nullptr == pq_client) {
		std::cout<<"Error: PgsqlConnPool::freePgsqlConn: free PgsqlConn object is NULL."<<std::endl;
		abort();
		return ;
	}

	std::unique_lock<std::mutex> lck(m_mutex);

	m_listConn.push_back(pq_client);

	//std::cout<<"free one client, pgsql size:"<< m_listConn.size()<<std::endl;
	m_condition.notify_one();
}

