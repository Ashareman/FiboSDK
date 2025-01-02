/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Y', 'A', 'P')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static void prvThreadEntry(void *param)
{
    UINT32 threadid = 0;
    INT32 usage = 0;
    OSI_LOGI(0, "cpu usage start");
    fibo_cpu_usage_init(30*1000);
    while(1)
    {
	OSI_LOGI(0, "cpu usage read");
	threadid = fibo_thread_id();
        usage = fibo_get_cpu_usage(CPU_USAGE_IDLE_TASK);
	OSI_PRINTFI("[CPU_USAGE][%s:%d] idle %d", __FUNCTION__, __LINE__, usage);
	usage = fibo_get_cpu_usage(threadid);
        OSI_PRINTFI("[CPU_USAGE][%s:%d] current thread %d", __FUNCTION__, __LINE__, usage);
	fibo_taskSleep(30000);
    }
    OSI_LOGI(0, "cpu usage end");
}

void* appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    fibo_thread_create(prvThreadEntry, "mythread", 1024, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
