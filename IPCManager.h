#pragma once

#include<string.h>
#include<unistd.h>
#include<sys/ipc.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

using namespace std;

// 记录错误日志
static void RecordErrorLog(const char* errMsg, int line)
{
	printf("%s. ErrorId:%d, Line:%d\n", errMsg, errno, line);
}

#define RECORD_ERROR_LOG(errMsg)	\
	RecordErrorLog(errMsg, __LINE__)

class NamePipeSender
{
public:
	NamePipeSender(const char* pipeName)
		:_pipeName(pipeName)
	{}

	bool Connect()
	{
		return true;
	}

	bool SendMsg(const char* msg, size_t msgLen, size_t& realSize)
	{

		int pipe_fd = open(_pipeName.c_str(), O_WRONLY| O_NONBLOCK);
		int ret = write(pipe_fd, msg, msgLen);
		if (ret == 0)
		{
			return false;
		}

		realSize = ret;
		close(pipe_fd);
		return true;
	}

	bool GetReplyMsg(char* msg, size_t msgLen, size_t realLen)
	{
		int pipe_fd = open(_pipeName.c_str(), O_RDONLY);
		int ret = read(pipe_fd, msg, msgLen);
		if (ret == 0)
		{
			return false;
		}
		msg[ret] = '\0';

		realLen = ret;
		close(pipe_fd);
		return true;
	}
private:
	string _pipeName;
};

class NamePipeReceiver
{
public:
	NamePipeReceiver(const char* pipeName)
		:_pipeName(pipeName)
	{}

	bool Listen()
	{

		int ret= 0;
		ret = mkfifo(_pipeName.c_str(), 0777);
		if (ret != 0)
		{
			RECORD_ERROR_LOG(to_string((long long)ret).c_str());
			return false;
		}

		return true;
	}

	bool ReceiverMsg(char* msg, size_t msgLen, size_t &realLen)
	{

		int pipe_fd = open(_pipeName.c_str(), O_RDONLY);
		int ret = read(pipe_fd, msg, msgLen);
		if (ret == 0)
		{
			return false;
		}
		msg[ret] = '\0';

		realLen = ret;
		close(pipe_fd);
		return true;
	}

	bool SendReplyMsg(const char* msg, size_t msgLen, size_t& realSize)
	{

		int pipe_fd = open(_pipeName.c_str(), O_WRONLY| O_NONBLOCK);
		int ret = write(pipe_fd, msg, msgLen);
		if (ret == 0)
		{
			return false;
		}

		realSize = ret;
		close(pipe_fd);
		return true;
	}

private:
	string _pipeName;
};

// IPC客户端
class IPCClient
{
public:
	IPCClient(const char* serverName)
		:_sender(serverName)
	{}

	~IPCClient()
	{}

	// 发送消息给服务端
	void SendMsg(char* buf, size_t bufLen)
	{
		size_t realLen;
		if (!_sender.Connect())
		{
			RECORD_ERROR_LOG("Client Connect Error\n");
		}

		if (!_sender.SendMsg(buf, bufLen, realLen))
		{
			RECORD_ERROR_LOG("Client SendMsg Error\n");
		}
	}

	// 获取服务端回复消息
	void GetReplyMsg(char* buf, size_t bufLen)
	{
		size_t realLen = 0;
		if (!_sender.GetReplyMsg(buf, bufLen, realLen))
		{
			RECORD_ERROR_LOG("Client GetReplyMsg Error\n");
		}
	}

private:
	NamePipeSender _sender;			// 发送者
};

// IPC服务端
class IPCServer
{
public:
	IPCServer(const char* serverName)
		:_receiver(serverName)
	{
		
	}

	~IPCServer()
	{}

	bool Listen()
	{
		return _receiver.Listen();
	}

	// 接收客户端消息
	void ReceiverMsg(char* buf, size_t bufLen)
	{
		size_t realLen;
		if (!_receiver.ReceiverMsg(buf, bufLen, realLen))
		{
			RECORD_ERROR_LOG("Server ReceiverMsg Error\n");
		}
	}

	// 回复客户端消息
	void SendReplyMsg(const char* buf, size_t bufLen)
	{
		size_t realLen;
		if (!_receiver.SendReplyMsg(buf, bufLen, realLen))
		{
			RECORD_ERROR_LOG("Server SendReplyMsg Error\n");
		}
	}
private:
	NamePipeReceiver _receiver;		// 接收者
};