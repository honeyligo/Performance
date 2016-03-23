#pragma once

#include <iostream>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <string>
#include <map>
#include <algorithm>

// C++11
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <pthread.h>

#include "IPCManager.h"

using namespace std;

typedef long long LongType;


//
// 获取当前线程id
//
static int GetThreadId()
{
	return pthread_self();
}

// 保存适配器抽象基类
class SaveAdapter
{
public:
	virtual int Save(char* format, ...) = 0;
};

// 控制台保存适配器
class ConsoleSaveAdapter : public SaveAdapter
{
public:
	virtual int Save(char* format, ...)
	{
		va_list argPtr;
		int cnt;

		va_start(argPtr, format);
		cnt = vfprintf(stdout, format, argPtr);
		va_end(argPtr);

		return cnt;
	}
};

// 文件保存适配器
class FileSaveAdapter : public SaveAdapter
{
public:
	FileSaveAdapter(const char* path)
		:_fOut(0)
	{
		_fOut = fopen(path, "w");
	}

	~FileSaveAdapter()
	{
		if (_fOut)
		{
			fclose(_fOut);
		}
	}

	virtual int Save(char* format, ...)
	{
		if (_fOut)
		{
			va_list argPtr;
			int cnt;

			va_start(argPtr, format);
			cnt = vfprintf(_fOut, format, argPtr);
			va_end(argPtr);

			return cnt;
		}

		return 0;
	}
private:
	FileSaveAdapter(const FileSaveAdapter&);
	FileSaveAdapter& operator==(const FileSaveAdapter&);

private:
	FILE* _fOut;
};

// 单例基类
//template<class T>
//class Singleton
//{
//public:
//	static T* GetInstance()
//	{
//		return _sInstance;
//	}
//protected:
//	Singleton()
//	{}
//
//	static T* _sInstance;
//};
//
//// 静态对象指针初始化，保证线程安全。 
//template<class T>
//T* Singleton<T>::_sInstance = new T();

// 单例基类
//template<class T>
//class Singleton
//{
//public:
//	static T* GetInstance()
//	{
//		return _sInstance;
//	}
//protected:
//	Singleton()
//	{}
//
//	static T* _sInstance;
//};
//
//// 静态对象指针初始化，保证线程安全。 
//template<class T>
//T* Singleton<T>::_sInstance = new T();

//
// 单例对象是静态对象，在内存管理的单例对象构造函数中会启动IPC的消息服务线程。
// 当编译成动态库时，程序main入口之前就会先加载动态库，而使用上面的方式，这时
// 就会创建单例对象，这时创建IPC的消息服务线程就会卡死。所以使用下面的单例模式，
// 在第一获取单例对象时，创建单例对象。
//

// 单例基类
template<class T>
class Singleton
{
public:
	static T* GetInstance()
	{
		//
		// 1.双重检查保障线程安全和效率。
		//
		if (_sInstance == NULL)
		{
			unique_lock<mutex> lock(_mutex);
			if (_sInstance == NULL)
			{
				_sInstance = new T();
			}
		}

		return _sInstance;
	}
protected:
	Singleton()
	{}

	static T* _sInstance;	// 单实例对象
	static mutex _mutex;	// 互斥锁对象
};

template<class T>
T* Singleton<T>::_sInstance = NULL;

template<class T>
mutex Singleton<T>::_mutex;

enum PP_CONFIG_OPTION
{
	PPCO_NONE = 0,					// 不做剖析
	PPCO_PROFILER = 2,				// 开启剖析
	PPCO_SAVE_TO_CONSOLE = 4,		// 保存到控制台
	PPCO_SAVE_TO_FILE = 8,			// 保存到文件
	PPCO_SAVE_BY_CALL_COUNT = 16,	// 按调用次数降序保存
	PPCO_SAVE_BY_COST_TIME = 32,	// 按调用花费时间降序保存
};

//
// 配置管理
//
class  OptionManager : public Singleton<OptionManager>
{
public:
	void SetOptions(int flag)
	{
		_flag = flag;
	}
	int GetOptions()
	{
		return _flag;
	}

	OptionManager()
		:_flag(PPCO_NONE)
	{}
private:
	int _flag;
};

///////////////////////////////////////////////////////////////////////////
// 资源统计

// 资源统计信息
struct ResourceInfo
{
	LongType _peak;	 // 最大峰值
	LongType _avg;	 // 平均值

	LongType _total;  // 总值
	LongType _count;  // 次数

	ResourceInfo()
		: _peak(0)
		,_avg(0)
		, _total(0)
		,_count(0)
	{}

	void Update(LongType value);
	void Serialize(SaveAdapter& SA) const;
};

// 资源统计
class ResourceStatistics
{
public:
	ResourceStatistics();
	~ResourceStatistics();

	// 开始统计
	void StartStatistics();

	// 停止统计
	void StopStatistics();

	// 获取CPU/内存信息 
	const ResourceInfo& GetCpuInfo();
	const ResourceInfo& GetMemoryInfo();

private:
	void _Statistics();
	void _UpdateStatistics();

public:

	int	_cpuCount;				// CPU个数

	LongType _lastSystemTime;	// 最近的系统时间
	LongType _lastKernelTime;	// 最近的内核时间

	int _pid;					// 进程ID


	ResourceInfo _cpuInfo;				// CPU信息
	ResourceInfo _memoryInfo;			// 内存信息

	atomic<int> _refCount;				// 引用计数
	mutex _lockMutex;					// 线程互斥锁
	condition_variable _condVariable;	// 控制是否进行统计的条件变量
	std::thread _statisticsThread;			// 统计线程
};

//////////////////////////////////////////////////////////////////////
// IPC在线控制监听服务

class IPCMonitorServer : public Singleton<IPCMonitorServer>
{
	friend class Singleton<IPCMonitorServer>;
	typedef void(*CmdFunc) (string& reply);
	typedef map<string, CmdFunc> CmdFuncMap;

public:
	// 启动IPC消息处理服务线程
	void Start();

protected:
	// IPC服务线程处理消息的函数
	void OnMessage();

	//
	// 以下均为观察者模式中，对应命令消息的的处理函数
	//
	static void GetState(string& reply);
	static void Enable(string& reply);
	static void Disable(string& reply);
	static void Save(string& reply);

	IPCMonitorServer();
private:
	std::thread	_onMsgThread;			// 处理消息线程
	CmdFuncMap _cmdFuncsMap;		// 消息命令到执行函数的映射表
};

///////////////////////////////////////////////////////////////////////////
// 代码段剖析

//
// 性能剖析节点
//
struct  PerformanceNode
{
	string _fileName;	// 文件名
	string _function;	// 函数名
	int	   _line;		// 行号
	string _desc;		// 附加描述

	PerformanceNode(const char* fileName, const char* function,
		int line, const char* desc);

	//
	// 做map键值，所以需要重载operator<
	// 做unorder_map键值，所以需要重载operator==
	//
	bool operator<(const PerformanceNode& p) const;
	bool operator==(const PerformanceNode& p) const;

	// 序列化节点信息到容器
	void Serialize(SaveAdapter& SA) const;
};

// hash算法
static size_t BKDRHash(const char *str)
{
	unsigned int seed = 131; // 31 131 1313 13131 131313
	unsigned int hash = 0;

	while (*str)
	{
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF);
}

// 实现PerformanceNodeHash仿函数做unorder_map的比较器
class PerformanceNodeHash
{
public:
	size_t operator() (const PerformanceNode& p)
	{
		string key = p._function;
		key += p._fileName;
		key += p._line;

		return BKDRHash(key.c_str());
	}
};

//
// 性能剖析段
//
class  PerformanceSection
{
	//typedef unordered_map<thread::id, LongType> StatisMap;
	typedef unordered_map<int, LongType> StatisMap;

	friend class Performance;
public:
	PerformanceSection()
		:_totalRef(0)
		, _rsStatistics(0)
		, _totalCallCount(0)
		, _totalCostTime(0)
	{}

	void Begin(int threadId);
	void End(int threadId);

	void Serialize(SaveAdapter& SA);
private:
	mutex _mutex;					// 互斥锁
	StatisMap _beginTimeMap;		// 开始时间统计
	StatisMap _costTimeMap;			// 花费时间统计
	LongType _totalCostTime;		// 总花费时间

	StatisMap _refCountMap;			// 引用计数(解决剖析段首尾不匹配，递归函数内部段剖析)
	LongType _totalRef;				// 总的引用计数

	StatisMap _callCountMap;		// 调用次数统计
	LongType _totalCallCount;		// 总的调用次数

	ResourceStatistics* _rsStatistics;	// 资源统计线程对象
};

class  Performance : public Singleton<Performance>
{
public:

	friend class Singleton<Performance>;

	//
	// unordered_map内部使用hash_table实现（时间复杂度为），map内部使用红黑树。
	// unordered_map操作的时间复杂度为O(1)，map为O(lgN)，so! unordered_map更高效。
	// http://blog.chinaunix.net/uid-20384806-id-3055333.html
	//
	//typedef unordered_map<PerformanceNode, PerformanceSection*, PerformanceNodeHash> PerformanceMap;
	typedef map<PerformanceNode, PerformanceSection*> PerformanceMap;

	//
	// 创建剖析段
	//
	PerformanceSection* CreateSection(const char* fileName,
		const char* funcName, int line, const char* desc, bool isStatistics);

	static void OutPut();
protected:

	static bool CompareByCallCount(PerformanceMap::iterator lhs,
		PerformanceMap::iterator rhs);
	static bool CompareByCostTime(PerformanceMap::iterator lhs,
		PerformanceMap::iterator rhs);

	Performance();

	// 输出序列化信息
	void _OutPut(SaveAdapter& SA);
private:
	time_t  _beginTime;
	mutex _mutex;
	PerformanceMap _ppMap;
};

// 添加性能剖析段开始
#define ADD_PERFORMANCE_SECTION_BEGIN(sign, desc, isStatistics) \
	PerformanceSection* PPS_##sign = NULL;						\
	if (OptionManager::GetInstance()->GetOptions()&PPCO_PROFILER)		\
	{																	\
		PPS_##sign = Performance::GetInstance()->CreateSection(__FILE__, __FUNCTION__, __LINE__, desc, isStatistics);\
		PPS_##sign->Begin(GetThreadId());								\
	}

// 添加性能剖析段结束
#define ADD_PERFORMANCE_SECTION_END(sign)	\
	do{												\
		if(PPS_##sign)								\
			PPS_##sign->End(GetThreadId());			\
	}while(0);

//
// 剖析【效率】开始
// @sign是剖析段唯一标识，构造出唯一的剖析段变量
// @desc是剖析段描述
//
#define PERFORMANCE_EE_BEGIN(sign, desc)	\
	ADD_PERFORMANCE_SECTION_BEGIN(sign, desc, false)

//
// 剖析【效率】结束
// @sign是剖析段唯一标识
//
#define PERFORMANCE_EE_END(sign)	\
	ADD_PERFORMANCE_SECTION_END(sign)

//
// 剖析【效率&资源】开始。
// ps：非必须情况，尽量少用资源统计段，因为每个资源统计段都会开一个线程进行统计
// @sign是剖析段唯一标识，构造出唯一的剖析段变量
// @desc是剖析段描述
//
#define PERFORMANCE_EE_RS_BEGIN(sign, desc)	\
	ADD_PERFORMANCE_SECTION_BEGIN(sign, desc, true)

//
// 剖析【效率&资源】结束
// ps：非必须情况，尽量少用资源统计段，因为每个资源统计段都会开一个线程进行统计
// @sign是剖析段唯一标识
//
#define PERFORMANCE_EE_RS_END(sign)		\
	ADD_PERFORMANCE_SECTION_END(sign)

//
// 设置剖析选项
//
#define SET_PERFORMANCE_OPTIONS(flag)		\
	OptionManager::GetInstance()->SetOptions(flag)
