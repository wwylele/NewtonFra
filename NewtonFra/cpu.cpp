/*
cpu.cpp
CPU占用率计算模块
*/

#include <Windows.h>

/// 时间转换  
static LONGLONG file_time_2_utc(const FILETIME* ftime)
{
	LARGE_INTEGER li;

	//assert(ftime);
	li.LowPart=ftime->dwLowDateTime;
	li.HighPart=ftime->dwHighDateTime;
	return li.QuadPart;
}


/// 获得CPU的核数  
static int get_processor_number()
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return (int)info.dwNumberOfProcessors;
}


static int get_cpu_usage()
{
	//cpu数量  
	static int processor_count_=-1;
	//上一次的时间  
	static LONGLONG last_time_=0;
	static LONGLONG last_system_time_=0;


	FILETIME now;
	FILETIME creation_time;
	FILETIME exit_time;
	FILETIME kernel_time;
	FILETIME user_time;
	LONGLONG system_time;
	LONGLONG time;
	LONGLONG system_time_delta;
	LONGLONG time_delta;

	int cpu=-1;


	if(processor_count_ == -1)
	{
		processor_count_=get_processor_number();
	}

	GetSystemTimeAsFileTime(&now);

	if(!GetProcessTimes(GetCurrentProcess(),&creation_time,&exit_time,
		&kernel_time,&user_time))
	{
		// We don't assert here because in some cases (such as in the Task   Manager)
			// we may call this function on a process that has just exited but   

			//we have
			// not yet received the notification.  
			return -1;
	}
	system_time=(file_time_2_utc(&kernel_time) + file_time_2_utc(&user_time))

		/
		processor_count_;
	time=file_time_2_utc(&now);

	if((last_system_time_ == 0) || (last_time_ == 0))
	{
		// First call, just set the last values.  
		last_system_time_=system_time;
		last_time_=time;
		return -1;
	}

	system_time_delta=system_time - last_system_time_;
	time_delta=time - last_time_;

	//assert(time_delta != 0);

	if(time_delta == 0)
		return -1;

	// We add time_delta / 2 so the result is rounded.  
	cpu=(int)((system_time_delta * 100 + time_delta / 2) / time_delta);
	last_system_time_=system_time;
	last_time_=time;
	return cpu;
}

int GetCpuPercentage(){
	int i=get_cpu_usage();
	return max(min(i,100),0);
}