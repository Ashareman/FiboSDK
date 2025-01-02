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

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

static void test_ticks()
{
    UINT32 ticks = 0;
    ticks = fibo_getSysTick();
    OSI_LOGI(0, "before ticks = %u", ticks);
    fibo_taskSleep(1000);
    ticks = fibo_getSysTick();
    OSI_LOGI(0, "after ticks = %u", ticks);
}

static void test_mem()
{
    char *pt = (char *)fibo_malloc(51200);
    if (pt != NULL)
    {
        OSI_LOGI(0, "malloc address %u", (unsigned int)pt);
        fibo_free(pt);
    }
}

static void test_reset(int mode)
{
    if (mode == 0)
        fibo_softReset();
    else
        fibo_softPowerOff();
}

static void test_csenter()
{
    UINT32 status;
    status = fibo_enterCS();
    OSI_LOGI(0, "enter CS");
    fibo_exitCS(status);
    OSI_LOGI(0, "exit CS");
}

/* sem test begin */
UINT32 g_test_sem;
static void testSemThread(void *param)
{
    fibo_taskSleep(2000);
    OSI_LOGI(0, "execute sem_signal...");
    fibo_sem_signal(g_test_sem);
	fibo_taskSleep(1000);
	fibo_sem_signal(g_test_sem);
    fibo_thread_delete();
}
static void test_sem()
{
    g_test_sem = fibo_sem_new(0);

    fibo_thread_create(testSemThread, "test_sem", 1024, NULL, OSI_PRIORITY_NORMAL);

    fibo_sem_wait(g_test_sem);
    OSI_LOGI(0, "after sem_wait...");

	fibo_sem_try_wait(g_test_sem, 5000);
	OSI_LOGI(0, "after sem_try_wait...");

	fibo_sem_try_wait(g_test_sem, 5000);
	OSI_LOGI(0, "after sem_try_wait...");
    fibo_sem_free(g_test_sem);
}
/* sem test end */

/* test timer begin */
UINT32 g_test_timer = 0;
UINT32 g_test_timer2 = 0;

UINT32 g_period_timer = 0;
UINT32 g_period_timer2 = 0;

UINT32 g_test_rtc_timer2 = 0;
UINT32 g_period_rtc_timer2 = 0;

void timer_function(void *arg)
{
    OSI_LOGI(0, "timer function execute ...");
    fibo_timer_free(g_test_timer);
}

void test_timer()
{
    g_test_timer = fibo_timer_new(5000, timer_function, NULL);
}

void timer_function2(void *arg)
{
    OSI_LOGI(0, "timer function2 execute ...");
    fibo_timer_free(g_test_timer2);
}

void test_timer2()
{
    g_test_timer2 = fibo_timer_create(timer_function2, NULL);
    OSI_LOGI(0, "test_timer2 id: 0x%x", g_test_timer2);

    fibo_timer_start(g_test_timer2, 6000);
}

void timer_function_period(void *arg)
{
    static int nr = 5;
    OSI_LOGI(0, "timer period function execute ...");
    nr--;
    if(nr == 0)
        fibo_timer_free(g_period_timer);
}

void test_timer_period()
{
    g_period_timer = fibo_timer_period_new(3000, timer_function_period, NULL);
}

void timer_function_period2(void *arg)
{
    static int nr = 5;
    OSI_LOGI(0, "timer period function2 execute cnt: %d...", nr);
    nr--;
    if(nr == 0)
    {
        fibo_timer_free(g_period_timer2);
    }
}

void test_timer_period2()
{
    g_period_timer2 = fibo_timer_create(timer_function_period2, NULL);
    OSI_LOGI(0, "test_timer_period2 id: 0x%x", g_period_timer2);

    fibo_timer_period_start(g_period_timer2, 3000);
}

void rtc_timer_function2(void *arg)
{
    OSI_LOGI(0, "rtc timer function2 execute ...");
    fibo_timer_free(g_test_rtc_timer2);
}

void test_rtc_timer2()
{
    g_test_rtc_timer2 = fibo_rtc_irq_create(rtc_timer_function2, NULL);
    OSI_LOGI(0, "rtc timer2 id: 0x%x", g_test_rtc_timer2);

    fibo_rtc_irq_start(g_test_rtc_timer2, 1000000);
}

void rtc_timer_function_period2(void *arg)
{
    static int nr = 10;
    OSI_LOGI(0, "rtc timer period function2 execute cnt: %d...", nr);
    nr--;
    if(nr == 0)
    {
        fibo_timer_free(g_period_rtc_timer2);
    }
}

void test_rtc_timer_period2()
{
    g_period_rtc_timer2 = fibo_rtc_irq_create(rtc_timer_function_period2, NULL);
    OSI_LOGI(0, "rtc test_timer_period2 id: 0x%x", g_period_rtc_timer2);

    fibo_rtc_irq_period_start(g_period_rtc_timer2, 8000);
}


/* test timer end */

void test_rand()
{
    srand(1000);
    for (int i = 0; i < 10; i++)
    {
        OSI_LOGI(0, "rand %d : %d", i, rand());
    }
}

/* test queue */
UINT32 g_queue_id = 0;
static void testQueueThread(void *param)
{
	int value = 8888;
    fibo_taskSleep(2000);
    fibo_queue_put(g_queue_id, &value, 0);
    fibo_thread_delete();
}

void test_queue()
{
	int value = 0;
	g_queue_id = fibo_queue_create(5, sizeof(int));
    fibo_thread_create(testQueueThread, "test_queue", 4096, NULL, OSI_PRIORITY_NORMAL);
	fibo_queue_get(g_queue_id, (void *)&value, 0);
	OSI_LOGI(0, "test queue value = %d", value);
}

UINT32 g_queue_id2 = 0;
static void testQueueThread2(void *param)
{
    int i = 0;
	int value = 1111;
    fibo_taskSleep(2000);

    for (i = 1; i < 10; i++)
    {
        value = 1111 *i;
        fibo_queue_put_to_front(g_queue_id2, &value, 0);
        OSI_LOGI(0, "test queue2 value put to front(%d) = %d", i, value);
    }

    fibo_thread_delete();
}

void test_queue2()
{
    int value = 0;
    g_queue_id2 = fibo_queue_create(10, sizeof(int));
    fibo_thread_create(testQueueThread2, "test_queue2", 4096, NULL, OSI_PRIORITY_NORMAL);

    fibo_taskSleep(5000);

    for (int i = 1; i < 10; i++)
    {
        fibo_queue_get(g_queue_id2, (void *)&value, 0);
        OSI_LOGI(0, "test queue2 value get(%d) = %d", i, value);
    }
}

/* test queue end */

static void nullThread(void *param)
{
	for (int n = 0; n < 5; n++)
    {
        OSI_LOGI(0, "null Thread in %d", n);
        fibo_taskSleep(1000);
    }
	fibo_thread_delete();
}


static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    test_ticks();
    test_mem();
    test_csenter();
    test_sem();
    test_timer();
    test_timer2();
    test_rtc_timer2();
    test_rand();
	test_queue();
    test_queue2();
	test_timer_period();
    test_timer_period2();
    test_rtc_timer_period2();

    for (int n = 0; n < 10; n++)
    {
        OSI_LOGI(0, "hello world %d", n);
        fibo_taskSleep(1000);
    }
    fibo_taskSleep(1000);
    //test_reset(0);

	UINT32 thread_id = 0;
    fibo_thread_create_ex(nullThread, "nullthread", 
    						1024, NULL, OSI_PRIORITY_NORMAL, &thread_id);
	OSI_LOGI(0, "nullThread id = 0x%x", thread_id);
    
    fibo_thread_delete();
}

void* appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
