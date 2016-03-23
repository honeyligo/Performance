#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
using namespace std;

#include "../IPCManager.h"

const char* SERVER_PIPE_NAME = "/tmp/performance_profiler/_fifo";

void UsageHelp ()
{
	printf ("PerformanceTool Tool. Bit Internal Tool\n");
	printf ("Usage: PerformanceTool -help\n");
	printf ("Usage: PerformanceTool -pid pid.\n");
	printf ("Example: PerformanceTool -pid 2345.\n");	

	exit (0);
}

void UsageHelpInfo ()
{
	printf ("    <exit>:    Exit.\n");
	printf ("    <help>:    Show Usage help Info.\n");
	printf ("    <state>:   Show the state of the PerformanceTool.\n");
	printf ("    <enable>:  Force enable performance profiler.\n");
	printf ("    <disable>: Force disable performance profiler.\n");
	printf ("    <save>:    Save the results to file.\n");
}

void PerformanceToolClient(const string& idStr)
{
	char res[1024] ={0};
	char msg[1024] = {0};
	size_t msgLen = 0;

	string serverPipeName = SERVER_PIPE_NAME;
	serverPipeName += idStr;

	UsageHelpInfo();

	IPCClient client(serverPipeName.c_str());

	while (1)
	{
		printf("shell:>");
		scanf("%s", msg);

		if (strcmp(msg, "help") == 0)
		{
			UsageHelpInfo();
			continue;
		}
		if (strcmp(msg, "exit") == 0)
		{
			printf("Performance Tool Client Exit\n");
			break;
		}

		client.SendMsg(msg, strlen(msg));
		client.GetReplyMsg(res, 1024);

		printf("%s\n\n", res);
	}
}

int main(int argc, char** argv)
{
	string idStr;
	if (argc == 2 && !strcmp(argv[1], "-help"))
	{
		UsageHelpInfo();

	}
	else if (argc == 3 && !strcmp(argv[1], "-pid"))
	{
		idStr += argv[2];
	}
	else
	{
		UsageHelp();
	}

	PerformanceToolClient(idStr);

	return 0;
}