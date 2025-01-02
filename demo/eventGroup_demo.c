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

#define pdFALSE         ((osiBaseType_t)0)
#define pdTRUE          ((osiBaseType_t)1)
#define portMAX_DELAY   ((osiTickType_t)0xffffffffUL)

/* 创建事件标志 */
#define EVENT_0     (1<<0)
#define EVENT_1     (1<<1)
#define EVENT_2     (1<<2)

osiEventGroupHandle_t p_handle = NULL;
UINT32 g_evtGroup_sem = 0;

static void demoEventBitReadingTask(void)
{
    osiEventBits_t eventRet;
    const osiEventBits_t bitsToWaitFor = ( EVENT_0 | EVENT_1 | EVENT_2 );

    g_evtGroup_sem = fibo_sem_new(0);
    if (0 < g_evtGroup_sem)
    {
        fibo_sem_wait(g_evtGroup_sem);
    }
    else
    {
        OSI_PRINTFE("[BitReadingRask] fibo_sem_new failed!");
        fibo_thread_delete();
        return;
    }

    while (1)
    {
        OSI_PRINTFI("[BitReadingRask] Wait...");
        eventRet = osiEventGroupWaitBits(p_handle,
                                         bitsToWaitFor,
                                         pdTRUE,
                                         pdFALSE,
                                         portMAX_DELAY);

        if (0 != (eventRet & EVENT_0))
        {
            //事件0发生
            OSI_PRINTFI("[BitReadingRask] Event bit 0 was set");
        }
        else if (0 != (eventRet & EVENT_1))
        {
            //事件1发生
            OSI_PRINTFI("[BitReadingRask] Event bit 1 was set");
        }
        else if (0 != (eventRet & EVENT_2))
        {
            //事件2发生
            OSI_PRINTFI("[BitReadingRask] Event bit 2 was set");
        }
    }
}

static void demoEventBitSettingISR(void* arg)
{
    osiBaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* 事件2发生 */
    osiEventGroupSetBitsFromISR(p_handle, EVENT_2, &xHigherPriorityTaskWoken);
}

static void demoEventBitSettingTask(void)
{
    do
    {
        /* 创建一个事件组 */
        p_handle = osiEventGroupCreate();
        if (NULL == p_handle)
        {
            OSI_PRINTFE("[BitSettingTask] osiEventGroupCreate failed!");
            break;
        }

        fibo_taskSleep(1000);
        if (0 < g_evtGroup_sem)
        {
            fibo_sem_signal(g_evtGroup_sem);
        }

        /* 设置事件状态 */
        OSI_PRINTFE("[BitSettingTask] Setting...");
        fibo_taskSleep(5000);
        (void)osiEventGroupSetBits(p_handle, EVENT_1);    /* 事件1发生 */

        fibo_taskSleep(5000);
        (void)osiEventGroupSetBits(p_handle, EVENT_0);    /* 事件0发生 */

        /* 定时中断 */
        fibo_setRtc_Irq(5000000, demoEventBitSettingISR, NULL);

        fibo_taskSleep(10000);

        /* 清除事件 */
        OSI_PRINTFE("[BitSettingTask] Clearing...");
        osiEventGroupClearBits(p_handle, EVENT_0);
        osiEventGroupClearBits(p_handle, EVENT_1);
        osiEventGroupClearBits(p_handle, EVENT_2);

    }while (0);

    OSI_PRINTFI("[BitSettingTask] demoEventBitSettingTask exit!");
    fibo_thread_delete();
}

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(demoEventBitReadingTask, "BitReadingRask", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    fibo_thread_create(demoEventBitSettingTask, "BitSettingTask", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
