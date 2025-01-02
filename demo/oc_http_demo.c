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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('H', 'T', 'T', 'P')

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

void cb_http_response_handle(void *pHttpParam, INT8 *data, INT32 len)
{
    fibo_taskSleep(50);
    fibo_http_response_status_line((oc_http_param_t *)pHttpParam);

    OSI_PRINTFI("@@@ HTTP response code          : %d", ((oc_http_param_t *)pHttpParam)->respCode);
    OSI_PRINTFI("@@@ HTTP response version       : %s", ((oc_http_param_t *)pHttpParam)->respVersion);
    OSI_PRINTFI("@@@ HTTP response reason phrase : %s", ((oc_http_param_t *)pHttpParam)->respReasonPhrase);

    OSI_PRINTFI("@@@ HTTP Recive Data : %d %s", len, data);
}

void cb_http_header_handle(const UINT8 *name, const UINT8 *value)
{
    OSI_PRINTFI("Header: %s : %s", name, value);
}

INT32 http_file_print(const INT8 *pPath)
{
    INT32 fd = 0;
    UINT8 aBuff[1024] = {0};
    INT32 ret = 0;

    memset(aBuff, 0, 1024);
    fd = fibo_file_open(pPath, FS_O_RDONLY);
    if(fd == -1)
    {
        OSI_PRINTFI("%s,%d http file read error", __FUNCTION__, __LINE__);
        return 1;
    }

    ret = fibo_file_read(fd, aBuff, 1024);
    if(ret == -1)
    {
        OSI_PRINTFI("%s,%d http file read error", __FUNCTION__, __LINE__);
        fibo_file_close(fd);
        return ret;
    }

    fibo_file_close(fd);

    OSI_PRINTFI("%s,%d http len %d data %s", __FUNCTION__, __LINE__, strlen((const char *)aBuff), aBuff);
    return 0;
}

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    const char *url = "http://182.92.122.159/1M.txt";
    char *filepath = "/http/test";
    oc_http_param_t * pstHttpParam = NULL;
    INT32 i = 0;
    UINT8 ip[50];
    reg_info_t reg_info;
#if 0
    /*          PDP activates IPv4 or IPv6       */

    pdp_profile_t pdp_profile;
    char *pdp_type = "ip";  //"ip", //"IPV6"
    char *apn = "cmnet";
    memset(&pdp_profile, 0, sizeof(pdp_profile));
    pdp_profile.cid = 2;
    memcpy(pdp_profile.nPdpType, pdp_type, strlen((char *)pdp_type));
    memcpy(pdp_profile.apn, apn, strlen((char *)apn));

    while(1)
    {
        fibo_getRegInfo(&reg_info, 0);
        fibo_taskSleep(1000);
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        if (reg_info.nStatus == 1)
        {
            fibo_asyn_PDPActive(1, &pdp_profile, 0);
            //fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
            fibo_taskSleep(1000);
            break;
            OSI_PRINTFI("[%s-%d] http ip %s", __FUNCTION__, __LINE__, ip);
        }
    }
#else
    /*         PDP activates IPv4       */
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
            OSI_PRINTFI("[%s-%d] http ip %s", __FUNCTION__, __LINE__, ip);
        }
    }
#endif
    for(i = 0; i < 1; i++)
    {
        /*=================================================*/
        /*         http head test, use user callback       */
        /*=================================================*/
        fibo_taskSleep(1000);
        pstHttpParam = fibo_http_new();
        if(NULL == pstHttpParam)
        {
            goto EXIT;
        }
        /* prepare http get param */
        pstHttpParam->timeout = 30;
        pstHttpParam->cbReadBody = (http_read_body_callback)cb_http_response_handle;
        pstHttpParam->respCode = 500;
        pstHttpParam->bIsIpv6 = false;
        pstHttpParam->enHttpReadMethod = OC_USER_CALLBACK;
        if(strlen(filepath) <= OC_HTTP_FILE_PATH_LEN)
        {
            strncpy((char *)pstHttpParam->filepath, filepath, strlen(filepath));
        }

        memset(pstHttpParam->url, 0, OC_HTTP_URL_LEN + 1);
        if(strlen(url) <= OC_HTTP_URL_LEN)
        {
            strncpy((char *)pstHttpParam->url, url, strlen(url));
        }
        else
        {
            goto EXIT;
        }
        fibo_http_head(pstHttpParam, NULL);
        fibo_taskSleep(1000);
        OSI_PRINTFI("HTTP response code          : %d", pstHttpParam->respCode);
        OSI_PRINTFI("HTTP response version       : %s", pstHttpParam->respVersion);
        OSI_PRINTFI("HTTP response reason phrase : %s", pstHttpParam->respReasonPhrase);
        OSI_PRINTFI("HTTP response body len      : %d", pstHttpParam->respContentLen);

        fibo_http_response_header_foreach(pstHttpParam, cb_http_header_handle);

        if(OC_SAVE_FILE == pstHttpParam->enHttpReadMethod)
        {
            http_file_print(filepath);
        }
        fibo_http_delete(pstHttpParam);
        pstHttpParam = NULL;

        /*=================================================*/
        /*        http get test, use user callback         */
        /*=================================================*/
        fibo_taskSleep(1000);
        pstHttpParam = fibo_http_new();
        if(NULL == pstHttpParam)
        {
            goto EXIT;
        }
        /* prepare http get param */
        pstHttpParam->timeout = 30;
        pstHttpParam->cbReadBody = (http_read_body_callback)cb_http_response_handle;
        pstHttpParam->respCode = 500;
        pstHttpParam->bIsIpv6 = false;
        pstHttpParam->enHttpReadMethod = OC_SAVE_FILE;
        if(strlen(filepath) <= OC_HTTP_FILE_PATH_LEN)
        {
            strncpy((char *)pstHttpParam->filepath, filepath, strlen(filepath));
        }

        memset(pstHttpParam->url, 0, OC_HTTP_URL_LEN + 1);
        if(strlen(url) <= OC_HTTP_URL_LEN)
        {
            strncpy((char *)pstHttpParam->url, url, strlen(url));
        }
        else
        {
            goto EXIT;
        }
        fibo_http_get(pstHttpParam, NULL);
        fibo_taskSleep(1000);
        OSI_PRINTFI("HTTP response code          : %d", pstHttpParam->respCode);
        OSI_PRINTFI("HTTP response version       : %s", pstHttpParam->respVersion);
        OSI_PRINTFI("HTTP response reason phrase : %s", pstHttpParam->respReasonPhrase);
        OSI_PRINTFI("HTTP response body len      : %d", pstHttpParam->respContentLen);

        fibo_http_response_header_foreach(pstHttpParam, cb_http_header_handle);

        if(OC_SAVE_FILE == pstHttpParam->enHttpReadMethod)
        {
            http_file_print(filepath);
        }
        fibo_http_delete(pstHttpParam);
        pstHttpParam = NULL;

        /*=================================================*/
        /*           http post test, save to file          */
        /*=================================================*/

        fibo_taskSleep(1000);
        /* http post */
        pstHttpParam = fibo_http_new();
        if(NULL == pstHttpParam)
        {
            goto EXIT;
        }
        /* prepare http get param */
        pstHttpParam->timeout = 30;
        pstHttpParam->cbReadBody = cb_http_response_handle;
        pstHttpParam->respCode = 500;
        pstHttpParam->bIsIpv6 = false;
        pstHttpParam->enHttpReadMethod = OC_SAVE_FILE;
        if(strlen(filepath) <= OC_HTTP_FILE_PATH_LEN)
        {
            strncpy((char *)pstHttpParam->filepath, filepath, strlen(filepath));
        }

        memset(pstHttpParam->url, 0, OC_HTTP_URL_LEN + 1);
        if(strlen((const char *)url) <= OC_HTTP_URL_LEN)
        {
            strncpy((char *)pstHttpParam->url, url, strlen(url));
        }
        else
        {
            goto EXIT;
        }
#if 1
        fibo_http_post_data_file(pstHttpParam, NULL, (uint32_t)"123456", filepath);
        //fibo_http_post(pstHttpParam, (UINT8 *)"000000", NULL);
#else
        //The header file specified by HTTP post
        //The number of Content-Length in post_Head is Post_Data of Length
        const char *Post_Head = "POST / HTTP/1.1\r\nAccept: application/json\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: 29\r\nHost: 111.231.250.105\r\n\r\n";
        const char *Post_Data = "{\"device_mac\":\"121212121212\"}";
        fibo_http_post(pstHttpParam, (UINT8 *)Post_Data, Post_Head);
        //fibo_http_post_ex(pstHttpParam, Post_Data, Post_Head, strlen(Post_Data));
#endif
        OSI_PRINTFI("HTTP response code          : %d", pstHttpParam->respCode);
        OSI_PRINTFI("HTTP response version       : %s", pstHttpParam->respVersion);
        OSI_PRINTFI("HTTP response reason phrase : %s", pstHttpParam->respReasonPhrase);
        OSI_PRINTFI("HTTP response body len      : %d", pstHttpParam->respContentLen);
        fibo_taskSleep(1000);

        fibo_http_response_header_foreach(pstHttpParam, cb_http_header_handle);
        if(OC_SAVE_FILE == pstHttpParam->enHttpReadMethod)
        {
            http_file_print(filepath);
        }
        fibo_http_delete(pstHttpParam);
        pstHttpParam = NULL;
    }
EXIT:
    fibo_http_delete(pstHttpParam);
    fibo_thread_delete();
}

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    prvInvokeGlobalCtors();
    fibo_thread_create(prvThreadEntry, "mythread", 1024 * 20, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
