#ifndef REDIS_CLIENT_2934083_H
#define REDIS_CLIENT_2934083_H

#include <string.h>
#include <vector>
#include <string>
#include <mutex>
#include <map>

//#define  REDIS_SAFE_ACCESS		\
//	std::unique_lock <std::mutex> redis_lock(m_mutex);

#define  REDIS_SAFE_ACCESS		;	

struct redisReply;
struct redisContext;

typedef struct S_Geo_Search_Result
{
	std::string 	member_name;
	double 	pos_lon;
	double	pos_lat;
	double	dist;

}T_Geo_Search_Result, *P_Geo_Search_Result;

class RedisClient
{
public:
	RedisClient();
	~RedisClient();
public:
	void Test(std::string strNum);

	bool CheckStatus();

	int  StartHeartBeatThread(int nHeartSeconds);

	//连接redis服务器，成功返回0，1连接失败，2密码错误, 超时时间为毫秒
	int ConnectWithTimeout(const std::string& strIP,int nPort,int nTimeout, const std::string& strPasswd);

	//判断是不是存在key
	int ExistKey(const std::string &strKey);
	//设置一个key值
	bool SetAKey(const std::string strKey, const std::string strData);
	bool SetAKeyUsingBinarySafeAPI(unsigned char* pkey, int iKeyLen, unsigned char* pData, int iDataLen);
	//获取一个数据
	bool GetData(std::string strKey, std::string &strGetData);

	int  MGetData(const std::vector<std::string>& arrayKeys, std::map<std::string, std::string>& mapValues);
	//对key值加1
	bool	Incr_IntData(std::string strKey);
	//删除一个数据
	bool	DelData(std::string strKey);

	//一个key对应list结构数据
	bool	PushData(std::string strListKey, std::string strData);
	bool	PopData(std::string strListKey, std::string& strData);

	//获取一个对对应的一组数据,iBeginIndex从0开始，iEndIndex=-1表示全部
	bool	GetRangeData(std::string strListKey, int iBeginIndex, int iEndIndex, std::vector<std::string>& vecGetData);

	//集合操作
	//一个key对应一个集合数据
	bool	PushDataToSet(std::string strKey, std::string strData);
	//从Set中POP一个数据
	bool	PopDataFromSet(std::string strKey, std::string &strData);
	//获取key指定的集合的全部数据
	bool	GetAllDataFromSet(std::string strKey, std::vector<std::string>& vecGetData);
	//删除key指定的集合中的指定元素
	bool	DelDataFromSet(std::string strKey, std::string strData);

	//散列操作Struct
	//散列的写入操作
	bool   PushDataToStruct(std::string strKey, std::map<std::string, std::string>& mapStruct);
	//散列的读取操作
	bool   GetDataFromStruct(std::string strKey, std::map<std::string, std::string>& mapStruct);

	// write member to geo 
	bool   PushMemberDataToGeo(const std::string& strKey, const std::string& strMember, double pos_lon, double pos_lat);
	// read member from geo
	bool   GetMemberDataFromGeo(const std::string& strKey, const std::string& strMember, double& pos_lon, double& pos_lat);
	// search members clear position
	int    GetMembersCloseGeo(const std::string& strKey, double pos_lon, double pos_lat, double distance_m, std::vector<T_Geo_Search_Result>& members);
	// caculate distance between two members(result unit : m)
	double GetDistanceByTwoMembers(const std::string& strKey, const std::string& strMember1, const std::string& strMember2);


	//设置ket的过期时间
	bool SetExpire(const std::string &strKey, int seconds ); //秒
	bool SetPExpire(const std::string &strKey, int million);//毫秒

	int TTLKey(const std::string &strKey);
	int PTTLKey(const std::string &strKey);

	bool	IsConnected();
	int		ReConnect();

	std::string	GetErrorInfo() 
	{
		return m_strErrDesc;
	}

	redisContext* GetResidContext()
	{
		return m_pRedisContext;
	}

protected:
	void  RunHeartBeat();
protected:
	redisContext*		m_pRedisContext;
	bool				m_bIsConnected;//是否连接

	std::string			m_strIP;
	int					m_nPort;
	int					m_nTimeout;			//  单位 ： 毫秒
	std::string			m_strPasswd;

	int					m_nHeartInterSecs;	//  心跳频率， 单位： 秒， 间隔一段时间检查状态
	bool				m_bHeartStarted;	//  心跳线程是否已启动

	int					m_nErrCode;
	std::string			m_strErrDesc;

	std::mutex			m_mutex;
};

#endif

