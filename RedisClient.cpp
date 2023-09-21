#include "RedisClient.h"
#include <hiredis/hiredis.h>
#include <thread>					// std::thread

#include <iostream>

using namespace std;

RedisClient::RedisClient()
{
	m_pRedisContext = NULL;
	m_bIsConnected = false;//是否连接

	m_strIP = "127.0.0.1";
	m_nPort = 6379;
	m_nTimeout = 5000;
	m_strPasswd.clear();

	m_nHeartInterSecs = 30;
	m_bHeartStarted = false;

	m_nErrCode = 0;
	m_strErrDesc.clear();
}

RedisClient::~RedisClient()
{
	m_bIsConnected = false;
}

//连接redis服务器，成功返回0，1连接失败，2密码错误, 超时时间为毫秒
int RedisClient::ConnectWithTimeout(const string& strIP, int nPort, int nTimeout, const string& strPasswd)
{
	REDIS_SAFE_ACCESS;

	m_strIP = strIP;
	m_nPort = nPort;
	m_nTimeout = nTimeout;
	m_strPasswd = strPasswd;

	redisReply *pReply=NULL;

	int nSeconds = nTimeout / 1000;
	int nMicroseconds = (nTimeout % 1000) * 1000;
	struct timeval timeout = { nSeconds, nMicroseconds };
	// std::cout<<"host IP:"<<strIP<<", Port:"<<nPort<<", Pass:"<<strPasswd<<std::endl;
	std::cout<<"host IP:"<<strIP<<", Port:"<<nPort<<std::endl;

	m_pRedisContext = redisConnectWithTimeout(strIP.c_str(), nPort, timeout);
	if ( (NULL == m_pRedisContext)|| m_pRedisContext->err) {
		if (m_pRedisContext) 
		{
			m_nErrCode = m_pRedisContext->err;
			m_strErrDesc = m_pRedisContext->errstr;
		}
		else 
		{
			m_nErrCode = 1;
			m_strErrDesc = "Can't allocate redis context";
		}
		m_bIsConnected = false;
		printf("Redis connection error: %s\n", m_strErrDesc.c_str());
		return m_nErrCode;
	}
	if (strPasswd.empty())
	{
		m_bIsConnected = true;
		m_strErrDesc = "Redis connect success.";
		m_nErrCode = 0;
		return m_nErrCode;
	}

	pReply = (redisReply *)redisCommand(m_pRedisContext, "AUTH %s", strPasswd.c_str());
	if( (NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command AUTH";
		}
		m_bIsConnected = false;
		m_nErrCode = 2; 
		printf("Redis Auth error: %s\n", m_strErrDesc.c_str());
		return m_nErrCode;
	}

	m_strErrDesc = pReply->str;
	m_nErrCode = 0;

	freeReplyObject(pReply);
	m_bIsConnected = true;

	return m_nErrCode;
}

int  RedisClient::StartHeartBeatThread(int nHeartSeconds)
{
	char szErrDesc[1024];
	if (m_bHeartStarted)
	{
		m_nErrCode = -1;
		snprintf(szErrDesc, sizeof(szErrDesc), "error : Heartbeat thread already started, interval : %d seconds.", m_nHeartInterSecs);
		m_strErrDesc = szErrDesc;

		return m_nErrCode;
	}
	if (nHeartSeconds <= 0)
	{
		m_nErrCode = -1;
		m_strErrDesc = "error : heart interval must above 0 second.";
		return m_nErrCode;
	}

	m_nHeartInterSecs = nHeartSeconds;

	thread t(std::bind(&RedisClient::RunHeartBeat, this));
	t.detach();

	return 0;
}

void RedisClient::RunHeartBeat()
{
	m_bHeartStarted = true;
	while (1)
	{
		CheckStatus();

		if (false == IsConnected())
		{
			int nErr = ReConnect();
			if (0 != nErr)
			{
				printf("failed to reconnect redis server, error : %s.\n", m_strErrDesc.c_str());
				abort();
			}
		}
		printf("redis activity.\n");

		std::chrono::milliseconds dura(m_nHeartInterSecs * 1000);
		std::this_thread::sleep_for(dura);
	}
}


	//判断是不是存在key
int RedisClient::ExistKey(const std::string &strKey)
{
	int ret = 0;
	redisReply *pReply=NULL;

	pReply = (redisReply *)redisCommand(m_pRedisContext,"EXISTS %s",strKey.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command EXISTS";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return 0;
	}

	ret = pReply->integer;
	freeReplyObject(pReply);

	return ret;
}

bool RedisClient::SetAKey( string strKey,string strData )
{
	REDIS_SAFE_ACCESS;

	redisReply *pReply=NULL;

	pReply = (redisReply *)redisCommand(m_pRedisContext,"SET %s %b",strKey.c_str(), strData.c_str(), strData.length());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command SET";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}

	freeReplyObject(pReply);

	return true;
}

bool RedisClient::SetAKeyUsingBinarySafeAPI(unsigned char* pkey,int iKeyLen,unsigned char* pData,int iDataLen)
{
	/* Set a key using binary safe API */
	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext,"SET %b %b", pkey, iKeyLen, pData, iDataLen);
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command SET BinarySafeAPI";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}

	freeReplyObject(pReply);

	return true;
}

bool RedisClient::GetData( string strKey ,string &strGetData)
{
	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	bool bSuccess = false;

	pReply = (redisReply *)redisCommand(m_pRedisContext,"GET %s",strKey.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command GET";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}
	if ( (REDIS_REPLY_STRING == pReply->type) && (pReply->str != NULL) )
	{
		strGetData = string(pReply->str, pReply->len);
		bSuccess = true;
	}
	freeReplyObject(pReply);

	return bSuccess;
}

int  RedisClient::MGetData(const std::vector<std::string>& arrayKeys, std::map<std::string, std::string>& mapValues)
{
	std::string strCommand ="MGET ";
	
	mapValues.clear();

	if(arrayKeys.size() == 0)
		return 0;

	for(int Idx = 0; Idx < arrayKeys.size(); Idx++)
	{
		strCommand = strCommand + " " + arrayKeys[Idx];
	}

	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext, strCommand.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command GET";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return -1;
	}

	if (pReply->type == REDIS_REPLY_ARRAY) {
		for (int j = 0; j < pReply->elements; j++) {
			if( (REDIS_REPLY_STRING == pReply->element[j]->type) && (pReply->element[j]->str != NULL) )
			{
				mapValues.insert(std::make_pair(arrayKeys[j], pReply->element[j]->str));
			}
		}
	}

	freeReplyObject(pReply);
	return mapValues.size();	
}

bool RedisClient::Incr_IntData( string strKey )
{
	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext,"INCR %s",strKey.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command INCR";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}
	freeReplyObject(pReply);

	return true;
}

bool RedisClient::DelData( string strKey )
{
	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext,"DEL %s",strKey.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command DEL";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}
	freeReplyObject(pReply);

	return true;
}

bool RedisClient::PushData( string strListKey,string strData )
{
	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext,"LPUSH %s %s",strListKey.c_str() ,strData.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command LPUSH";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}
	freeReplyObject(pReply);
	return true;
}

bool RedisClient::PopData(std::string strListKey, std::string& strData)
{
	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext,"RPOP %s",strListKey.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command LPUSH";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}
	if(pReply->type == REDIS_REPLY_NIL)
	{
		m_strErrDesc = "listkey is empty.";
		m_nErrCode = -1;
		freeReplyObject(pReply);
		return false;
	}

	strData = pReply->str;
	freeReplyObject(pReply);
	return true;	
}

bool RedisClient::GetRangeData( string strListKey, int iBeginIndex, int iEndIndex, vector<string>& vecGetData )
{
	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext,"LRANGE %s %d %d",strListKey.c_str(), iBeginIndex, iEndIndex);
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command LRANGE";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}
	if(pReply->type == REDIS_REPLY_NIL)
	{
		m_strErrDesc = "listkey is empty.";
		m_nErrCode = -1;
		freeReplyObject(pReply);
		return false;
	}

	if (pReply->type == REDIS_REPLY_ARRAY) {
		for (int j = 0; j < pReply->elements; j++) {
			if(pReply->element[j]->str != NULL)
			{
				vecGetData.push_back(pReply->element[j]->str);
			}
		}
	}

	freeReplyObject(pReply);
	return true;
}

bool RedisClient::PushDataToSet(string strKey,string strData)
{
	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext,"SADD %s %s",strKey.c_str() ,strData.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command SADD";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}
	freeReplyObject(pReply);
	return true;
}

bool RedisClient::PopDataFromSet(std::string strKey, std::string &strData)
{
	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext,"SPOP %s",strKey.c_str());
	
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command SPOP";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}
	if (pReply->type == REDIS_REPLY_STRING) {
		strData = pReply->str;	
	}

	freeReplyObject(pReply);

	return true;
}

bool RedisClient::GetAllDataFromSet(string strKey, vector<string>& vecGetData)
{
	vecGetData.clear();

	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext,"SMEMBERS %s",strKey.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command SMEMBERS";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}

	if (pReply->type == REDIS_REPLY_ARRAY) {
		for (int j = 0; j < pReply->elements; j++) {
			if(pReply->element[j]->str != NULL)
			{
				vecGetData.push_back(pReply->element[j]->str);
			}
		}
	}

	freeReplyObject(pReply);

	return true;
}

bool RedisClient::DelDataFromSet(string strKey,string strData)
{
	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext,"SREM %s %s",strKey.c_str() ,strData.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command SREM";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}
	freeReplyObject(pReply);
	return true;
}

bool   RedisClient::PushDataToStruct(std::string strKey, std::map<std::string, std::string>& mapStruct)
{
	std::string strCommand;
	char szFields[1024];

	memset(szFields, 0, sizeof(szFields));

	int FieldNum = mapStruct.size();
	if(FieldNum == 0)
	{
		m_strErrDesc = "field name size is empty.";
		m_nErrCode = -1;
		return false;
	}

	snprintf(szFields, sizeof(szFields) - 1, "HMSET %s ", strKey.c_str());
	strCommand = szFields;

	int nLens = 0;
	std::map<std::string, std::string>::iterator iterPos = mapStruct.begin();
	while(iterPos != mapStruct.end())
	{
		snprintf(szFields, sizeof(szFields), " %s %s", iterPos->first.c_str(), iterPos->second.c_str());

		strCommand = strCommand + szFields;
		iterPos++;
	}

	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext, strCommand.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command HMSET";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}

	freeReplyObject(pReply);

	return true;
}

bool   RedisClient::GetDataFromStruct(std::string strKey, std::map<std::string, std::string>& mapStruct)
{
	mapStruct.clear();

	std::string strCommand;

	char szFields[1024];
	memset(szFields, 0, sizeof(szFields));

	snprintf(szFields, sizeof(szFields), "HGETALL %s ", strKey.c_str());
	strCommand = szFields;

	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext, strCommand.c_str());
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command HGETALL";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}

	if(pReply->type == REDIS_REPLY_NIL)
	{
		m_strErrDesc = "hashkey is empty.";
		m_nErrCode = -1;
		freeReplyObject(pReply);
		return false;
	}

	if(pReply->type != REDIS_REPLY_ARRAY)
	{
		m_strErrDesc = "error return value by command HGETALL.";
		m_nErrCode = -1;
		freeReplyObject(pReply);
		return false;
	}

	for (int j = 0; j + 1 < pReply->elements; j = j + 2)
	{
		mapStruct[pReply->element[j]->str] = pReply->element[j+1]->str;
	}

	freeReplyObject(pReply);

	if(mapStruct.size() == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool RedisClient::PushMemberDataToGeo(const std::string& strKey, const std::string& strMember, double pos_lon, double pos_lat)
{
	char szFields[512];
	memset(szFields, 0, sizeof(szFields));

	if(strKey.empty() || strMember.empty())
	{
		m_strErrDesc = "input params has empty value.";
		m_nErrCode = -1;
		return false;
	}
	snprintf(szFields, sizeof(szFields) - 1, "geoadd %s  %.7f  %.7f  %s", strKey.c_str(), pos_lon, pos_lat, strMember.c_str());

	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext, szFields);
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command geoadd";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}

	freeReplyObject(pReply);
	return true;	
}

bool RedisClient::GetMemberDataFromGeo(const std::string& strKey, const std::string& strMember, double& pos_lon, double& pos_lat)
{
	char szFields[512];
	memset(szFields, 0, sizeof(szFields));

	if(strKey.empty() || strMember.empty())
	{
		m_strErrDesc = "input params has empty value.";
		m_nErrCode = -1;
		return false;
	}

	snprintf(szFields, sizeof(szFields) - 1, "geopos %s %s", strKey.c_str(), strMember.c_str());

	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext, szFields);
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command geopos";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}

	if((pReply->type != REDIS_REPLY_ARRAY) || (pReply->elements != 1))
	{
		m_strErrDesc = "error return value by command geopos.";
		m_nErrCode = -1;
		freeReplyObject(pReply);
		return false;
	}
	redisReply *pReplyElem = (redisReply *)pReply->element[0];
	if((pReplyElem->type != REDIS_REPLY_ARRAY) || (pReplyElem->elements != 2))
	{
		m_strErrDesc = "error return value by command geopos.";
		m_nErrCode = -1;
		freeReplyObject(pReply);
		return false;		
	}
	pos_lon = atof(pReplyElem->element[0]->str);
	pos_lat = atof(pReplyElem->element[1]->str);

	freeReplyObject(pReply);

	return true;	
}

int  RedisClient::GetMembersCloseGeo(const std::string& strKey, double pos_lon, double pos_lat, double distance_m, std::vector<T_Geo_Search_Result>& members)
{
	char szFields[512];
	memset(szFields, 0, sizeof(szFields));

	members.clear();

	if( strKey.empty() )
	{
		m_strErrDesc = "input params has empty value.";
		m_nErrCode = -1;
		return -1;
	}
	snprintf(szFields, sizeof(szFields) - 1, "georadius %s %.7f  %.7f %.1f m withcoord withdist asc", strKey.c_str(), pos_lon, pos_lat, distance_m);

	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext, szFields);
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command georadius";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return -1;
	}
	if(REDIS_REPLY_NIL == pReply->type)
	{
		freeReplyObject(pReply);
		return 0;
	}
	else if( pReply->type != REDIS_REPLY_ARRAY )
	{
		m_strErrDesc = "error return value by command georadius.";
		m_nErrCode = -1;
		freeReplyObject(pReply);
		return -1;
	}
	else
	{
		for(int Idx = 0; Idx < pReply->elements; Idx++)
		{
			redisReply *pReplyElem = (redisReply *)pReply->element[Idx];
			if(NULL != pReplyElem)
			{
				T_Geo_Search_Result geo_result;

				geo_result.member_name = pReplyElem->element[0]->str;
				geo_result.dist = atof(pReplyElem->element[1]->str);

				redisReply *pReplyPos = (redisReply *)pReplyElem->element[2];
				geo_result.pos_lon = atof(pReplyPos->element[0]->str);
				geo_result.pos_lat = atof(pReplyPos->element[1]->str);

				members.push_back(geo_result);
			}
		}		
	}

	freeReplyObject(pReply);

	return members.size();	
}

double RedisClient::GetDistanceByTwoMembers(const std::string& strKey, const std::string& strMember1, const std::string& strMember2)
{
	char szFields[512];
	memset(szFields, 0, sizeof(szFields));

	if(strKey.empty() || strMember1.empty() || strMember2.empty())
	{
		m_strErrDesc = "input params has empty value.";
		m_nErrCode = -1;
		return -1;
	}
	snprintf(szFields, sizeof(szFields) - 1, "geodist %s %s %s m", strKey.c_str(), strMember1.c_str(), strMember2.c_str());

	REDIS_SAFE_ACCESS;

	redisReply *pReply = NULL;
	pReply = (redisReply *)redisCommand(m_pRedisContext, szFields);
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command geodist";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return -1;
	}
	if(pReply->type == REDIS_REPLY_NIL)
	{
		m_strErrDesc = "member param not exist.";
		m_nErrCode = -1;
		freeReplyObject(pReply);
		return -1;
	}

	double fDistance = atof(pReply->str);
	freeReplyObject(pReply);

	return fDistance;		
}

void RedisClient::Test(string strNum)
{
	//Set Test
	string setName = "SetDemo";
	bool bRet = PushDataToSet(setName, "A");
	bRet = PushDataToSet(setName, "B");
	bRet = PushDataToSet(setName, "C");
	bRet = PushDataToSet(setName, "A");

	vector<string>  retSet;
	bRet = GetAllDataFromSet(setName, retSet);

	bRet = DelDataFromSet(setName,"D");
	bRet = DelDataFromSet(setName,"A");

	bRet = GetAllDataFromSet(setName, retSet);

	bRet = DelData(setName);

	// struct Test
	string strKeyName = "StructDemo";
	std::map<std::string, std::string> mapStruct;

	mapStruct["abc"] = "111";
	mapStruct["cde"] = "38943";
	mapStruct["adkjsfkj"] = "9384";

	bRet = PushDataToStruct(strKeyName, mapStruct);

	mapStruct.clear();
	bRet = GetDataFromStruct(strKeyName, mapStruct);

	bRet = DelData(strKeyName);

	bRet = DelData(strKeyName);

	string strArrayName = "ArrayDemo";

	bRet = PushData(strArrayName, "dkljd");
	bRet = PushData(strArrayName, "dskd");
	bRet = PushData(strArrayName, "dfdjfj");
	bRet = PushData(strArrayName, "fcvclkjdfl");

	vector<string>  arrayValues;
	bRet = GetRangeData(strArrayName, 0, 2, arrayValues);

	bRet = DelData(strArrayName);

	string stringName = "StringDemo";
	bRet = SetAKey(stringName, "dklsfjd");

	string stringValue;
	bRet = GetData(stringName, stringValue);

	bRet = DelData(stringName);

	return ;

	/*string strKey = "aaaaa_"+strNum;
	SetAKey(strKey,"bbbbbbbbbbbbbbbb");
	string strTemp;
	bool bRedisRes = GetData(strKey,strTemp);
	printf("setData:%s=>%s\n",strKey.c_str(),strTemp.c_str());

	strKey = "safe_save_"+strNum;
	char strData[128] = "111111";
	SetAKeyUsingBinarySafeAPI((unsigned char*)strKey.c_str(),5,(unsigned char*)strData,6);
	bRedisRes = GetData(strKey,strTemp);
	printf("setData:%s=>%s\n",strKey.c_str(),strTemp.c_str());*/

}

bool RedisClient::CheckStatus()
{
	REDIS_SAFE_ACCESS;

	redisReply *pReply = (redisReply*)redisCommand(m_pRedisContext,"ping");
	if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
	{
		if (pReply)
		{
			m_strErrDesc = pReply->str;
			freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command PING";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}

	if(pReply->type!=REDIS_REPLY_STATUS)
	{
		m_bIsConnected = false;
		freeReplyObject(pReply);

		m_strErrDesc = "return error value by command PING";
		m_nErrCode = -1;
		return false;
	}
	if(strcasecmp(pReply->str,"PONG") != 0)
	{
		m_bIsConnected = false;
		freeReplyObject(pReply);
		m_strErrDesc = "return error value by command PING";
		m_nErrCode = -1;
		return false;
	}

	freeReplyObject(pReply);
	return true;
}

bool RedisClient::IsConnected()
{
	REDIS_SAFE_ACCESS;

	return m_bIsConnected;
}

int RedisClient::ReConnect()
{
	REDIS_SAFE_ACCESS;

	if(true == m_bIsConnected)
		return 0;

	if(m_pRedisContext)
	{
		redisFree(m_pRedisContext);
		m_pRedisContext = NULL;
	}

	int nErr = ConnectWithTimeout(m_strIP, m_nPort, m_nTimeout, m_strPasswd);

	return nErr;
}



bool RedisClient::SetExpire(const std::string &strKey, int seconds )
{
    REDIS_SAFE_ACCESS;

    redisReply *pReply = NULL;
    pReply = (redisReply *)redisCommand(m_pRedisContext,"EXPIRE %s %d",strKey.c_str(), seconds);
    if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
    {
        if (pReply)
        {
            m_strErrDesc = pReply->str;
            freeReplyObject(pReply);
		}
		else
		{
			m_strErrDesc = "return NULL by command EXPIRE";
		}
		m_nErrCode = -1;
		m_bIsConnected = false;
		return false;
	}
	freeReplyObject(pReply);

	return true;
}


bool RedisClient::SetPExpire(const std::string &strKey, int million )
{
    REDIS_SAFE_ACCESS;

    redisReply *pReply = NULL;
    pReply = (redisReply *)redisCommand(m_pRedisContext,"PEXPIRE %s %d",strKey.c_str(), million);
    if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
    {
        if (pReply)
        {
            m_strErrDesc = pReply->str;
            freeReplyObject(pReply);
        }
        else
        {
            m_strErrDesc = "return NULL by command PEXPIRE";
        }
        m_nErrCode = -1;
        m_bIsConnected = false;
        return false;
    }
    freeReplyObject(pReply);

    return true;
}

int RedisClient::TTLKey(const std::string &strKey)
{


    REDIS_SAFE_ACCESS;

    int ttl= -1;
    redisReply *pReply = NULL;
    pReply = (redisReply *)redisCommand(m_pRedisContext,"TTL %s ",strKey.c_str());
    if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
    {
        if (pReply)
        {
            m_strErrDesc = pReply->str;
            freeReplyObject(pReply);
        }
        else
        {
            m_strErrDesc = "return NULL by command TTL";
        }
        m_nErrCode = -1;
        m_bIsConnected = false;
        return -1;
    }

    if ( REDIS_REPLY_NIL == pReply->type )
    {
        ttl = -1;
    }

    if ( REDIS_REPLY_INTEGER == pReply->type )
    {
        ttl = pReply->integer;
    }

    freeReplyObject(pReply);

    return ttl;

}


int RedisClient::PTTLKey(const std::string &strKey)
{


    REDIS_SAFE_ACCESS;

    int ttl= -1;
    redisReply *pReply = NULL;
    pReply = (redisReply *)redisCommand(m_pRedisContext,"PTTL %s ",strKey.c_str());
    if ((NULL == pReply) || (pReply->type == REDIS_REPLY_ERROR))
    {
        if (pReply)
        {
            m_strErrDesc = pReply->str;
            freeReplyObject(pReply);
        }
        else
        {
            m_strErrDesc = "return NULL by command TTL";
        }
        m_nErrCode = -1;
        m_bIsConnected = false;
        return -1;
    }

    if ( REDIS_REPLY_NIL == pReply->type )
    {
        ttl = -1;
    }

    if ( REDIS_REPLY_INTEGER == pReply->type )
    {
        ttl = pReply->integer;
    }

    freeReplyObject(pReply);

    return ttl;

}
