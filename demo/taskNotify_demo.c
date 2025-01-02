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

static osiTaskHandle_t task1_id = NULL;
static osiTaskHandle_t task2_id = NULL;

static void prvTask1(void)
{
    while (1)
    {
        fibo_taskSleep(5000);

        /*向prvTask2(),发送通知，使其解除阻塞状态 */
        OSI_PRINTFI("[prvTask1] give notification to task2");
        osiTaskNotifyGive(task2_id);

        OSI_PRINTFI("[prvTask1] waiting for notification...");
        /* 等待prvTask2()的通知，进入阻塞 */
        //osiTaskNotifyTake(pdTRUE, portMAX_DELAY);
        osiTaskNotifyWait(0x00, 0xffffffff, NULL, portMAX_DELAY);
        OSI_PRINTFI("[prvTask1] got notification.");
    }
}

static void prvTask2(void)
{
    uint32_t  ret = 0;

    while (1)
    {
        /* 等待prvTask1()的通知，进入阻塞 */
        OSI_PRINTFI("[prvTask2] waiting for notification...");
        ret = osiTaskNotifyTake(pdTRUE, portMAX_DELAY);
        OSI_PRINTFI("[prvTask2] got notification. ret: %d", ret);

        fibo_taskSleep(5000);

        OSI_PRINTFI("[prvTask2] give notification to task1");
        /*向prvTask1(),发送通知，使其解除阻塞状态 */
        osiTaskNotify(task1_id, 1, osiSetBits);
        //osiTaskNotifyGive(task1_id);
    }
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

    fibo_thread_create_ex(prvTask1, "prvTask1", 1024*4, NULL, OSI_PRIORITY_NORMAL, (UINT32 *)&task1_id);
    fibo_thread_create_ex(prvTask2, "prvTask2", 1024*4, NULL, OSI_PRIORITY_NORMAL, (UINT32 *)&task2_id);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
