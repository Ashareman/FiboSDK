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


UINT32 g_pdp_active_sem;
bool   test_pdp_active_flag;

UINT32 g_pdp_deactive_sem;
bool   test_pdp_deactive_flag;

UINT32 g_pdp_asyn_active_sem;
bool   test_pdp_asyn_active_flag;

UINT32 g_pdp_asyn_deactive_sem;
bool   test_pdp_asyn_deactive_flag;

void ntp_callback(void *param)
{
	fibo_ntp_rsp rsp;
	memcpy(&rsp, param, sizeof(fibo_ntp_rsp));
	OSI_PRINTFI("[%s-%d]result:%d, timestamp=%d", __FUNCTION__, __LINE__, rsp.result, rsp.time);
}

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    int ret = 0;
    fibo_taskSleep(20000);
	OSI_PRINTFI("[%s-%d] osiThreadCurrent() = %p", __FUNCTION__, __LINE__,osiThreadCurrent());

	UINT8 ip[50];
	UINT8 cid_status;
	
	memset(&ip, 0, sizeof(ip));
	ret = fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
	
	OSI_PRINTFI("[%s-%d] ret = %d,ip=%s", __FUNCTION__, __LINE__, ret,ip);

	fibo_taskSleep(1000);

	memset(&ip, 0, sizeof(ip));
	if (0 == fibo_PDPStatus(1, ip,&cid_status, 0))
	{
		OSI_PRINTFI("[%s-%d] ip = %s,cid_status=%d", __FUNCTION__, __LINE__, ip,cid_status);
	}

	for(int i = 0; i < 5; i++)
	{
		ret = fibo_sock_ntp("ntp.aliyun.com", 123, 0, ntp_callback);
		OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__,ret);
		fibo_taskSleep(5*1000);
	}
	
	while(1)
	{
		fibo_taskSleep(1000);
		OSI_PRINTFI("[%s-%d] ", __FUNCTION__, __LINE__);
	}
	OSI_PRINTFI("[%s-%d] ", __FUNCTION__, __LINE__);
	fibo_thread_delete();

}

static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
	//fibo_PDPActive  ip address resopnse event
	case GAPP_SIG_PDP_ACTIVE_ADDRESS:
	{
		UINT8 cid = (UINT8)va_arg(arg, int);
		char *addr = (char *)va_arg(arg, char *);
		if(addr != NULL)
		{
			OSI_PRINTFI("sig_res_callback  cid = %d, addr = %s ", cid, addr);
		}
		else 
		{
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
		va_end(arg);
	}
	break;

    //fibo_getHostByName event
	case GAPP_SIG_DNS_QUERY_IP_ADDRESS:
	{   
	    char *host = (char *)va_arg(arg, char *);
        char *ipv4_addr = (char *)va_arg(arg, char *);
		char *ipv6_addr = (char *)va_arg(arg, char *);

		if(host != NULL)
		{
            OSI_PRINTFI("sig_res_callback, host = %s ", host);
		}
		if(ipv4_addr != NULL)
		{
			OSI_PRINTFI("sig_res_callback, ipv4_addr = %s ", ipv4_addr);
		}
		if(ipv6_addr != NULL)
		{
			OSI_PRINTFI("sig_res_callback, ipv6_addr = %s ", ipv6_addr);
		}
		else
		{
			OSI_PRINTFI("sig_res_callback, ip_addr is NULL");
		}

		va_end(arg);
	}
	break;

	//fibo_PDPActive /fibo_asyn_PDPActive  pdp active status report
	case GAPP_SIG_PDP_ACTIVE_IND:
	{
		UINT8 cid = (UINT8)va_arg(arg, int);
		OSI_PRINTFI("sig_res_callback  cid = %d", cid);
		va_end(arg);
		if(g_pdp_active_sem && test_pdp_active_flag)
		{	
		    
		    fibo_sem_signal(g_pdp_active_sem);
			test_pdp_active_flag = 0;
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
		if(g_pdp_asyn_active_sem && test_pdp_asyn_active_flag)
		{	
		    
		    fibo_sem_signal(g_pdp_asyn_active_sem);
			test_pdp_asyn_active_flag = 0;
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
	}
	break;

	//fibo_PDPRelease /fibo_asyn_PDPRelease pdp deactive status report
	case GAPP_SIG_PDP_RELEASE_IND:
	{
		UINT8 cid = (UINT8)va_arg(arg, int);
		OSI_PRINTFI("sig_res_callback  cid = %d", cid);
		va_end(arg);
		if(g_pdp_deactive_sem && test_pdp_deactive_flag)
		{	
		    
		    fibo_sem_signal(g_pdp_deactive_sem);
			test_pdp_deactive_flag = 0;
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
		if(g_pdp_asyn_deactive_sem && test_pdp_asyn_deactive_flag)
		{	
		    
		    fibo_sem_signal(g_pdp_asyn_deactive_sem);
			test_pdp_asyn_deactive_flag = 0;
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
	}
	break;

	//GAPP_SIG_PDP_ACTIVE_OR_DEACTIVE_FAIL_IND
	case GAPP_SIG_PDP_ACTIVE_OR_DEACTIVE_FAIL_IND:
	{
	
		UINT8 cid = (UINT8)va_arg(arg, int);
		UINT8 state = (UINT8)va_arg(arg, int);
		OSI_PRINTFI("[%s-%d]cid = %d,state = %d", __FUNCTION__, __LINE__,cid,state);
		va_end(arg);
	}

	//PDP in active state, deactive indicator received from modem  
	case GAPP_SIG_PDP_DEACTIVE_ABNORMALLY_IND:
	{
	
		UINT8 cid = (UINT8)va_arg(arg, int);
		OSI_PRINTFI("[%s-%d]cid = %d", __FUNCTION__, __LINE__,cid);
		va_end(arg);
	}
		
    default:
    {
        break;
    }
    }
    OSI_LOGI(0, "test");
}

static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = sig_res_callback};

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    prvInvokeGlobalCtors();
    fibo_thread_create(prvThreadEntry, "mythread", 1024*8*2, NULL, OSI_PRIORITY_NORMAL);
    return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
