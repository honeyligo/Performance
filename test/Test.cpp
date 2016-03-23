#include<iostream>
using namespace std;

// C++11
#include<thread>


#include "../Performance.h"

// 1.测试基本功能
void Test1()
{
	PERFORMANCE_EE_BEGIN(PP1, "PP1");

	for(int i = 0; i < 1000000000; i++);
	
	PERFORMANCE_EE_END(PP1);

	PERFORMANCE_EE_BEGIN(PP2, "PP2");

	for(int i = 0; i < 100000000; i++);

	PERFORMANCE_EE_END(PP2);
}

int main()
{
	SET_PERFORMANCE_OPTIONS(
		PPCO_PROFILER | PPCO_SAVE_TO_FILE | PPCO_SAVE_BY_COST_TIME);

	Test1();
	getchar();
	return 0;
}