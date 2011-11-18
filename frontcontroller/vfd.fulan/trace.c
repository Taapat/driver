
/**********************************文件头部注释************************************/
//
//
//  					Copyright (C), 2005-2010, AV Frontier Tech. Co., Ltd.
//
//
// 文件名：		trace.c
//
// 创建者：		mjj
//
// 创建日期：	2007/12/12
//
// 文件描述：	[T]ask [R]eporting [A]nd [C]urrent [E]valuation
//
// 当前维护者：(  由API  组指定人员负责维护)
//
//  最后更新：	2007/12/12
//
//												 
/*****************************************************************************************/


/************************************文件包含****************************************/
#include <linux/module.h>

//#include "ywdefs.h"
#include "trace.h"

/************************************宏定义*******************************************/


/************************************数据结构****************************************/

int trace_level = TRACE_ERROR | TRACE_FATAL|TRACE_INFO;
module_param(trace_level, int , S_IRUGO);

int YWTRACE_Init(void)
{
    return 0;    
}


int YWTRACE_Print (const u32 level, const char * format, ...)
{
	va_list args;
	int r=0;
	if(trace_level & level)
	{
		va_start(args, format);
		if (level & (TRACE_ERROR | TRACE_FATAL))
		{
			printk("---------------");
		}
		r = vprintk(format, args);
		va_end(args);
	}
	return r;
}

//EXPORT_SYMBOL(YWTRACE_Init);
//EXPORT_SYMBOL(YWTRACE_Print);


/* ------------------------------- End of file ---------------------------- */





