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

UINT8 certfile[] = 
"-----BEGIN CERTIFICATE-----\r\n\
MIIDlDCCAnwCFF3ckVBmbx4t5LeyHoV898/118CCMA0GCSqGSIb3DQEBCwUAMIGP\r\n\
MQswCQYDVQQGEwJDTjEPMA0GA1UECAwGc2hhbnhpMQ0wCwYDVQQHDAR4aWFuMRAw\r\n\
DgYDVQQKDAdmaWJvY29tMRAwDgYDVQQLDAdmaWJvY29tMRYwFAYDVQQDDA00Ny4x\r\n\
MTAuMjM0LjM2MSQwIgYJKoZIhvcNAQkBFhV2YW5zLndhbmdAZmlib2NvbS5jb20w\r\n\
HhcNMjAwODIwMTE0MTA2WhcNMzAwODE4MTE0MTA2WjB9MQswCQYDVQQGEwJDTjEP\r\n\
MA0GA1UECAwGc2hhbnhpMQ0wCwYDVQQHDAR4aWFuMRAwDgYDVQQKDAdmaWJvY29t\r\n\
MRYwFAYDVQQDDA00Ny4xMTAuMjM0LjM2MSQwIgYJKoZIhvcNAQkBFhV2YW5zLndh\r\n\
bmdAZmlib2NvbS5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCv\r\n\
Bl6QTBpt6pTe99Zd9KOzUxJQXbQCqtDeJdzmewpa2a0pqStPsMeB3+M0k3Ohsed9\r\n\
HAPCcm6+m7cIR2DlUe5i1njRHeoxHc2h1bhS0mXPJo4NgLFqIjqrfdeg+bBCj8h2\r\n\
zcXXH87KPzCbUXOTObT/rSULHG8cjm+tSnYgjWHjIrdOlBql5fgdXbf5eYICnI8Y\r\n\
RGJcWO+UDD7001XotZntVsgrrcei+cD8fI+6ivxFbiLLQ2CU5zzY0ZPmFvbRBwRK\r\n\
8oM8k8261wEjKsxUjiFYx6CYtfuCziVTleBqOtC9A0fSrp77ojovFS3umqqik5R1\r\n\
uCzQCOZZXpykyKL8OkRrAgMBAAEwDQYJKoZIhvcNAQELBQADggEBAEygLa3cPR45\r\n\
2abLUqO9Q5QqT286NhK8KeugKPgfb07TFhWbOoXkdaQ1mlTk11TlrsiXG+eJe7Fn\r\n\
TKrCNSJUnCurgma5CRqS1Yr+EWn874EE5JiGtSc/5Ga/KOP3KMKNnYWgJQIuMtnn\r\n\
gcuHaQ6G2OU+90FswylqFRC15cXau8GYVOupv0VpcmxDidwHMprZVNuEruxquUV/\r\n\
Xg5KuJA4gpIQkdVUdlkjA9U5C7IesX2LNvmAydjeR7NzmhGjytZnEbWdftYG2zoF\r\n\
GEKfB+x5uZjQMOXwlfPMFQmVuoZh+jpa53748d+Md3RGgg43E8otszHeICul0zSs\r\n\
PkFCb7GRWsQ=\r\n\
-----END CERTIFICATE-----";

UINT8 keyfile[] = 
"-----BEGIN RSA PRIVATE KEY-----\r\n\
MIIEowIBAAKCAQEArwZekEwabeqU3vfWXfSjs1MSUF20AqrQ3iXc5nsKWtmtKakr\r\n\
T7DHgd/jNJNzobHnfRwDwnJuvpu3CEdg5VHuYtZ40R3qMR3NodW4UtJlzyaODYCx\r\n\
aiI6q33XoPmwQo/Ids3F1x/Oyj8wm1Fzkzm0/60lCxxvHI5vrUp2II1h4yK3TpQa\r\n\
peX4HV23+XmCApyPGERiXFjvlAw+9NNV6LWZ7VbIK63HovnA/HyPuor8RW4iy0Ng\r\n\
lOc82NGT5hb20QcESvKDPJPNutcBIyrMVI4hWMegmLX7gs4lU5XgajrQvQNH0q6e\r\n\
+6I6LxUt7pqqopOUdbgs0AjmWV6cpMii/DpEawIDAQABAoIBADrTbjcbjQqCfJTQ\r\n\
QdmEXvznn9EpHVaEKP1xRmSk2B8e6GeHN0pqhTOul0PVh1jCXaacIttY8MXZulPr\r\n\
AbMxrWjE4wiOAGePt8x7857KnnNYZwg8x+R/Kq729eFh6o8EmoDrumIKi8tIH8Mk\r\n\
Ri8mhyIkBL5OST4U1Y4t57QbMNpRA3bSAIoD6/QxaZ9t2/m7IUyClf1KKFbXCaTL\r\n\
6FZh68mXoJpPNV75rXTq9TNYtVO2k7h6oW8iuu5UwnQQpXBkLloOm6VMmFRCFuC+\r\n\
fh2LBgxTEkFdfDJnJpeuEVCc558zPonLvKVmD8rkaCLHIETePm9R5JOC/Rv9ROH1\r\n\
tBgrQwkCgYEA1DoL2Qu1A2xg8jJfHPOqv17vlrxa4ENq2xG1yG0siKOpd6X1MRwc\r\n\
dc3uh5DAA/80KTn7xFArB1KHweosUGIO9fmU/gkvXfLlGQnNsgIdKFZvFyM1sTP5\r\n\
fdXcsJS5fKAcC1CsvPVvpxJyiT3LhGCRO4hfmcl6jJ9OzsnW/eplIh8CgYEA0x//\r\n\
csZ0V1OoXQeTsJ5QRod8Qh0dPwkLqDQsVZyb4bllO2TkC/dLyqVowijpHxBh5Elq\r\n\
jCa/jjNSYGo4hiKdvlriYEFTDebDC3SXxdcv86ixC0vH4zKti8NDKhunVIG/8fCi\r\n\
iqvB/Tjf7SQEGWRqjykyLRVeH4kDF03kLJW3TDUCgYEAh4wzeQMzL+aO3OIzQYiX\r\n\
6/a0y++tk0M8AoODOWoRYYw2dwb2XdF4k/1ddhSLr4HWTOaN2Uri0KBzuPTaLNUU\r\n\
fSJVeRNgv36duKo8SI91FAhwl7STXIS3uxlXBSlYdzLD9q4mReH02B6+LM3dKMWM\r\n\
vRtTBCRdM2ekrAraV/7XbT0CgYBAy+dIwKPgUWqw8qxfXpdgriBy4iChwhLz0t9w\r\n\
fxpQkugA7JwZGBMI5O9b99ZklFCXEflDfnj4GcRElxU2BdXIIHit9h6Ze6ONFoGm\r\n\
VL8A11tPDjkQ//LHnGw2tjoK86+Hf8VDLifhod0IGS+w42LZAVnHAHHc1948/sjy\r\n\
7hhNqQKBgBs5xypuEz9MMof6+6vxFAWDmTMuZr7CVrgR3AaksHHD1w5Pz0J6wp6I\r\n\
kOs3Yx8d14lzom1voiO/MxBpiFssymxLl/0XAtY6NwwMYpvxgGntjABZJsTZ+xsv\r\n\
SVqKuwoZ7c0EXnBRKTcWwIhN/YgrEZ2ljqnseDAARdXFEY+Ga070\r\n\
-----END RSA PRIVATE KEY-----";

UINT8 trustfile[] =
"-----BEGIN CERTIFICATE-----\r\n\
MIID9TCCAt2gAwIBAgIJAOEIwHHcR9K7MA0GCSqGSIb3DQEBBQUAMIGPMQswCQYD\r\n\
VQQGEwJDTjEPMA0GA1UECAwGc2hhbnhpMQ0wCwYDVQQHDAR4aWFuMRAwDgYDVQQK\r\n\
DAdmaWJvY29tMRAwDgYDVQQLDAdmaWJvY29tMRYwFAYDVQQDDA00Ny4xMTAuMjM0\r\n\
LjM2MSQwIgYJKoZIhvcNAQkBFhV2YW5zLndhbmdAZmlib2NvbS5jb20wIBcNMTkw\r\n\
ODMwMDY1MjUwWhgPMjExOTA4MDYwNjUyNTBaMIGPMQswCQYDVQQGEwJDTjEPMA0G\r\n\
A1UECAwGc2hhbnhpMQ0wCwYDVQQHDAR4aWFuMRAwDgYDVQQKDAdmaWJvY29tMRAw\r\n\
DgYDVQQLDAdmaWJvY29tMRYwFAYDVQQDDA00Ny4xMTAuMjM0LjM2MSQwIgYJKoZI\r\n\
hvcNAQkBFhV2YW5zLndhbmdAZmlib2NvbS5jb20wggEiMA0GCSqGSIb3DQEBAQUA\r\n\
A4IBDwAwggEKAoIBAQC6UMQfxHL0oW9pY1cGvq5QPdw8OU7dX2YsCbPdEiXePKce\r\n\
E6AN3IKqOuZhEd1iIypXG2AywzIu9bd5w1d4COjjSC/Tpf2AKYw+jqfxHsQAvSKt\r\n\
Rvwp1wrS5IvWy8yEG9lNpyVJHBUWlVpU/tUf02MYYU5xUBS+mJE9Tc10j7kd/uV7\r\n\
aEfM0pYhm7VmHPWDHXeXj3LKYigjttNxUgpDh2UVpq6ejzzHA5T/k2+XtKtWu7Pb\r\n\
ag6lONzz6Zxya9htVLBy7I4uTFrcRPxNgc/KF2BuwEVc4rqGUZ4vpVdwmCyKGIua\r\n\
fvit1nsvnhvYMu01HhWuK6e3IO6hOpeyR1wk75ofAgMBAAGjUDBOMB0GA1UdDgQW\r\n\
BBTT9RodyqsY/C2WS/7k8GFWidQrlTAfBgNVHSMEGDAWgBTT9RodyqsY/C2WS/7k\r\n\
8GFWidQrlTAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUAA4IBAQCkg9dUYBRs\r\n\
uqCz71Q75B2n768icIeMfQmf969fNVK/mwaUnFxjqq+4Xw3zADdUdnTZ8FEfjAt2\r\n\
LQaxmsiGlM3KmUhXy/k1xKypIu2KecxEX/NqdG02SYcBmrIAP6ZxOxyyJZXbPRBt\r\n\
11W3e9+MsRFjRNDxvi5xPcBTu7padUXf7gZp/U8RTc9r0RzsTJu0oFx1Vl6B9m9Z\r\n\
4Ae7EshqUrGbnQMJ9XinPVMhuPB4UTc5H9F9ZEswkd/8fK1kXE2aD9LOUD3ITpfH\r\n\
h4UBb/UX3VY2eoLC6T5FzPggAcyxU/S2svZaq2+fSWvA7WpEYmTvzQTeT+y1BaUW\r\n\
9SoOHidKUkQe\r\n\
-----END CERTIFICATE-----";

static void prvThreadEntry(void *param)
{	
    UINT8 ip[50] = {0};
    reg_info_t reg_info;	
    UINT8 ctxid = 0;
	UINT8 send_buffer[32] = {0};
	INT32 sock = -1;
	INT32 ret = -1;
	UINT8 buf[64];
	
	while(1)
    {
        fibo_getRegInfo(&reg_info, 0);
        fibo_taskSleep(1000);
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        if (reg_info.nStatus == 1)
        {
            fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
            fibo_taskSleep(1000);
            break;
            OSI_PRINTFI("[%s-%d] ssl ip %s", __FUNCTION__, __LINE__, ip);
        }
    }

    OSI_LOGI(0, "application thread enter, param 0x%x", param);
	
    srand(100);

    OSI_LOGI(0, "******dahua ssl fuction test begin******");
	
	//一、单/双向认证
	OSI_LOGI(0, "dahua ssl test(signal/double) begin");
	fibo_set_ssl_log(true);//open ssl log
	for(ctxid = 1; ctxid <= 6; ctxid ++)
    {
        if(ctxid % 2 != 0)//单向认证
        {
		   OSI_LOGI(0, "******dahua ssl(singal) fuction test ssl_ctx_id %d******", ctxid);
        }
		else
        {
		   OSI_LOGI(0, "******dahua ssl(double) fuction test ssl_ctx_id %d******", ctxid);
        }
		srand(100);
        
        //1.set version
        fibo_ssl_set_ver_ctx(ctxid, 4);
		
        //2.set check mode
        if(ctxid % 2 != 0)//单向认证
        {
           fibo_ssl_set_chkmode_ctx(ctxid, 0);
        }
		else
		{
           fibo_ssl_set_chkmode_ctx(ctxid, 1);
        }
		
        //3.load certificate
        if (1 == ctxid || 2 == ctxid)
        {
		   OSI_LOGI(0, "dahua ssl: load certificate from ram");
		   fibo_write_ssl_file_from_ram(ctxid, "TRUSTFILE", trustfile, sizeof(trustfile) - 1);
           fibo_write_ssl_file_from_ram(ctxid, "CERTFILE", certfile, sizeof(certfile) - 1);
           fibo_write_ssl_file_from_ram(ctxid, "KEYFILE", keyfile, sizeof(keyfile) - 1);
        }
		else
		{
		   OSI_LOGI(0, "dahua ssl: load certificate from fs");
		   fibo_write_ssl_file_from_fs(ctxid, "CERTFILE", "/FFS/certfile", strlen("/FFS/certfile"), NULL, 0);
		   fibo_write_ssl_file_from_fs(ctxid, "KEYFILE", "/FFS/keyfile", strlen("/FFS/keyfile"), NULL, 0);
		   fibo_write_ssl_file_from_fs(ctxid, "TRUSTFILE", "/FFS/cafile", strlen("/FFS/cafile"), NULL, 0);
		}
		
        //4.create ssl socket
        sock = fibo_ssl_sock_create_ctx(ctxid);
        if (-1 == sock)
        {
            OSI_LOGI(0, "dahua create ssl sock failed");
            fibo_thread_delete();
        }
        
        //5.connect
        OSI_LOGI(0, "dahua fibo_ssl_sock_connect %d", sock);
		if(ctxid % 2 != 0)//单向认证
		{
		    ret = fibo_ssl_sock_connect_ctx(ctxid, sock, "47.110.234.36", 8887);
		}
		else
		{
		    ret = fibo_ssl_sock_connect_ctx(ctxid, sock, "47.110.234.36", 8888);
		}
        OSI_LOGI(0, "dahua fibo_ssl_sock_connect_ctx %d", ret);
		
        //6.send
        memset(send_buffer, 0 , sizeof(send_buffer));
		snprintf((char *)send_buffer, sizeof(send_buffer), "daha test ssl_ctxid %d",ctxid);
        ret = fibo_ssl_sock_send_ctx(ctxid, sock, send_buffer, strlen((const char*)send_buffer));
        OSI_LOGI(0, "dahua fibo_ssl_sock_send_ctx %d", ret);
    
        //7.recv
        memset(buf, 0 ,sizeof(0));
        ret = fibo_ssl_sock_recv_ctx(ctxid, sock, buf, sizeof(buf));
        OSI_LOGI(0, "dahua fibo_ssl_sock_recv_ctx ret = %d", ret);
        if (ret > 0)
        {
            OSI_LOGI(0, "dahua fibo_ssl_sock_recv_ctx = %s", (char *)buf);
        }
		
        //8.get err code
        ret = fibo_get_ssl_errcode_ctx(ctxid);
        OSI_LOGI(0, "dahua fibo_get_ssl_errcode_ctx ret = %d", ret);
		
        //9.close
        ret = fibo_ssl_sock_close_ctx(ctxid, sock);	
        OSI_LOGI(0, "dahua fibo_ssl_sock_close_ctx ret = %d", ret);

		//10.clear session
        ret = fibo_ssl_session_clear_ctx(ctxid);
        OSI_LOGI(0, "dahua fibo_ssl_session_clear_ctx ret = %d", ret);
		
		fibo_taskSleep(2000);
    }		
	fibo_set_ssl_log(false);//close ssl log
	OSI_LOGI(0, "dahua ssl test(signal/double) end");

	//二、PSK功能验证/SNI功能测试/设置私约密码	
	OSI_LOGI(0, "dahua ssl test(complex) begin");
	for (ctxid = 1; ctxid <= 6; ctxid++)
	{
	    UINT8 psk_key[32] = {0};	
	    UINT8 psk_id[32] = {0};
		
        srand(100);
        
        //1.set version
        fibo_ssl_set_ver_ctx(ctxid, 4);
		
        //2.set check mode
        fibo_ssl_set_chkmode_ctx(ctxid, 1);
		
        //3.load certificat
        /*ctxid为1，是测试PSK，PSK方式不需要加载证书*/
        if(1 != ctxid)
        {
           fibo_write_ssl_file_from_fs(ctxid, "CERTFILE", "/FFS/certfile", strlen("/FFS/certfile"), NULL, 0);
           if(2 == ctxid)//私钥密码设置
           {
              fibo_write_ssl_file_from_fs(ctxid, "KEYFILE", "/FFS/keyfile", strlen("/FFS/keyfile"), "psk_key_001", strlen("psk_key_001"));
           }
           else
           {
              fibo_write_ssl_file_from_fs(ctxid, "KEYFILE", "/FFS/keyfile", strlen("/FFS/keyfile"), NULL, 0);
           }
           fibo_write_ssl_file_from_fs(ctxid, "TRUSTFILE", "/FFS/cafile", strlen("/FFS/cafile"), NULL, 0);
        }

        //设置psk
        if(1 == ctxid)
        {
           /*ctxid为1，是测试PSK，测试PSK时对于支持的算法，将不支持psk的算法全部关闭，将支持psk的算法全部打开*/
           for(int i = 0;  i <= 137; i++)
           {
              fibo_ssl_set_cipher_ctx(ctxid, i, 0);
           }

           for(int i = 110;  i <= 137; i++)
           {
              fibo_ssl_set_cipher_ctx(ctxid, i, 1);
           }
		   
		   //设置psk的key和id
		   OSI_LOGI(0, "******dahua ssl(psk) fuction test ssl_ctx_id %d******", ctxid);
           memset(psk_key, 0, sizeof(psk_key));
           memset(psk_id, 0, sizeof(psk_id));
   		   snprintf((char *)psk_key, sizeof(psk_key), "0A000%d", ctxid);
   		   snprintf((char *)psk_id, sizeof(psk_id), "0B000%d", ctxid);
           fibo_ssl_set_psk_id_key_ctx(ctxid,(const char *)psk_key, strlen((const char *)psk_key), (const char *)psk_id, strlen((const char*)psk_id));
        }

        //设置SNI
		if(3 == ctxid)
        {
		   OSI_LOGI(0, "******dahua ssl(close SNI) fuction test ssl_ctx_id %d******", ctxid);
           fibo_ssl_sni_switch_ctx(ctxid, 0);
        }
		else if(4 == ctxid)
		{
		   OSI_LOGI(0, "******dahua ssl(open SNI) fuction test ssl_ctx_id %d******", ctxid);
		   fibo_ssl_sni_switch_ctx(ctxid, 1);
		}

		//设置cipher list
		if(5 == ctxid)
        {
           UINT16 i_temp = 1;
           for(; i_temp <= 137; i_temp++)
           {
               fibo_ssl_set_cipher_ctx(ctxid, i_temp, 1);
           }
		   OSI_LOGI(0, "******dahua ssl(cipher list,open all cipher) fuction test ssl_ctx_id %d******", ctxid);
        }
		else if(6 == ctxid)
		{
		   UINT16 i_temp = 1;
           for(; i_temp <= 137; i_temp++)
           {
               fibo_ssl_set_cipher_ctx(ctxid, i_temp, 0);
           }
		   OSI_LOGI(0, "******dahua ssl(cipher list,close all cipher) fuction test ssl_ctx_id %d******", ctxid);
		}
		
        //4.create ssl socket
        sock = fibo_ssl_sock_create_ctx(ctxid);
        if (-1 == sock)
        {
            OSI_LOGI(0, "dahua create ssl sock failed");
            fibo_thread_delete();
        }
        
        //5.connect
        OSI_LOGI(0, "dahua fibo_ssl_sock_connect %d", sock);
		ret = fibo_ssl_sock_connect_ctx(ctxid, sock, "47.110.234.36", 8888);
        OSI_LOGI(0, "dahua fibo_ssl_sock_connect_ctx %d", ret);
		
        //6.send
        memset(send_buffer, 0 , sizeof(send_buffer));
		snprintf((char *)send_buffer, sizeof(send_buffer), "daha test ssl_ctxid %d",ctxid);
        ret = fibo_ssl_sock_send_ctx(ctxid, sock, send_buffer, strlen((const char*)send_buffer));
        OSI_LOGI(0, "dahua fibo_ssl_sock_send_ctx %d", ret);
    
        //7.recv
        memset(buf, 0 ,sizeof(0));
        ret = fibo_ssl_sock_recv_ctx(ctxid, sock, buf, sizeof(buf));
        OSI_LOGI(0, "dahua fibo_ssl_sock_recv_ctx ret = %d", ret);
        if (ret > 0)
        {
            OSI_LOGI(0, "dahua fibo_ssl_sock_recv_ctx = %s", (char *)buf);
        }
		
        //8.get err code
        //ret = fibo_get_ssl_errcode_ctx(ctxid);
        OSI_LOGI(0, "dahua fibo_get_ssl_errcode_ctx ret = %d", ret);
		
        //9.close
        ret = fibo_ssl_sock_close_ctx(ctxid, sock);	
        OSI_LOGI(0, "dahua fibo_ssl_sock_close_ctx ret = %d", ret);

		//10.clear session
        ret = fibo_ssl_session_clear_ctx(ctxid);
        OSI_LOGI(0, "dahua fibo_ssl_session_clear_ctx ret = %d", ret);
		
		fibo_taskSleep(2000);
    }
	OSI_LOGI(0, "dahua ssl test(complex) end");
	
	//三、会话复用功能测试	
	OSI_LOGI(0, "dahua ssl test(session plex) begin");
	UINT8 ctxid_temp;
	for (ctxid_temp = 1; ctxid_temp <= 4; ctxid_temp++)
	{
		OSI_LOGI(0, "******dahua ssl(session plex) fuction test ssl_ctx_id_temp %d******", ctxid_temp);
        srand(100);
		
		ctxid = ctxid_temp % 2 + 1;
        
        //1.set version
        fibo_ssl_set_ver_ctx(ctxid, 4);
		
        //2.set check mode
        if(1 == ctxid)
        {
           fibo_ssl_set_chkmode_ctx(ctxid, 1);
        }
		else if(2 == ctxid)
        {
           fibo_ssl_set_chkmode_ctx(ctxid, 0);
        }
		
        //3.load certificat
		fibo_write_ssl_file_from_fs(ctxid, "CERTFILE", "/FFS/certfile", strlen("/FFS/certfile"), NULL, 0);
		fibo_write_ssl_file_from_fs(ctxid, "KEYFILE", "/FFS/keyfile", strlen("/FFS/keyfile"), NULL, 0);
		fibo_write_ssl_file_from_fs(ctxid, "TRUSTFILE", "/FFS/cafile", strlen("/FFS/cafile"), NULL, 0);

        //设置session plex switch
        if(1 == ctxid)
        {
           OSI_LOGI(0, "dahua ssl ctxid %d close session plex function", ctxid);
           fibo_ssl_set_session_plex_ctx(ctxid, 0);
        }
		else if(2 == ctxid)
		{
		   OSI_LOGI(0, "dahua ssl ctxid %d open session plex function", ctxid);
		   fibo_ssl_set_session_plex_ctx(ctxid, 1);
		}

        //4.create ssl socket
        sock = fibo_ssl_sock_create_ctx(ctxid);
        if (-1 == sock)
        {
            OSI_LOGI(0, "dahua create ssl sock failed");
            fibo_thread_delete();
        }
        
        //5.connect
        OSI_LOGI(0, "dahua fibo_ssl_sock_connect %d", sock);
		if(1 == ctxid)
        {
            OSI_LOGI(0, "dahua ssl,close session plex function,host = 47.110.234.36, port = 8888");
		    ret = fibo_ssl_sock_connect_ctx(ctxid, sock, "47.110.234.36", 8888);
		}
		else if(2 == ctxid)
		{
            OSI_LOGI(0, "dahua ssl,open session plex function,host = xatest.fibocom.com,port = 990");
		    ret = fibo_ssl_sock_connect_ctx(ctxid, sock, "xatest.fibocom.com", 990);
		}
        OSI_LOGI(0, "dahua fibo_ssl_sock_connect_ctx %d", ret);
		
        //6.send
        memset(send_buffer, 0 , sizeof(send_buffer));
		snprintf((char *)send_buffer, sizeof(send_buffer), "daha test ssl_ctxid %d",ctxid);
        ret = fibo_ssl_sock_send_ctx(ctxid, sock, send_buffer, strlen((const char*)send_buffer));
        OSI_LOGI(0, "dahua fibo_ssl_sock_send_ctx %d", ret);
    
        //7.recv
        memset(buf, 0 ,sizeof(0));
        ret = fibo_ssl_sock_recv_ctx(ctxid, sock, buf, sizeof(buf));
        OSI_LOGI(0, "dahua fibo_ssl_sock_recv_ctx ret = %d", ret);
        if (ret > 0)
        {
            OSI_LOGI(0, "dahua fibo_ssl_sock_recv_ctx = %s", (char *)buf);
        }
		
        //8.get err code
        //ret = fibo_get_ssl_errcode_ctx(ctxid);
        OSI_LOGI(0, "dahua fibo_get_ssl_errcode_ctx ret = %d", ret);
		
        //9.close
        ret = fibo_ssl_sock_close_ctx(ctxid, sock);	
        OSI_LOGI(0, "dahua fibo_ssl_sock_close_ctx ret = %d", ret);

		//10.clear session
		if(1 == ctxid)
        {
           OSI_LOGI(0, "dahua ssl ctxid %d close session plex function", ctxid);
           ret = fibo_ssl_session_clear_ctx(ctxid);
           OSI_LOGI(0, "dahua fibo_ssl_session_clear_ctx ret = %d", ret);
		}
		
		fibo_taskSleep(2000);
    }  	
	OSI_LOGI(0, "dahua ssl test(session plex) end");
    OSI_LOGI(0, "******dahua ssl fuction test end******");

    fibo_thread_delete();
}

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024 * 4, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
