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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('D', 'B', 'L', 'E')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "bt_abs.h"
#include "bt_app.h"


//error code definitions
#define ATT_ERR_INVALID_HANDLE 0x01
#define ATT_ERR_READ_NOT_PERMITTED 0x02
#define ATT_ERR_WRITE_NOT_PERMITTED 0x03
#define ATT_ERR_INVALID_PDU 0x04
#define ATT_ERR_INSUFFICIENT_AUTHEN 0x05
#define ATT_ERR_REQUEST_NOT_SUPPORT 0x06
#define ATT_ERR_INVALID_OFFSET 0x07
#define ATT_ERR_INSUFFICIENT_AUTHOR 0x08
#define ATT_ERR_PREPARE_QUEUE_FULL 0x09
#define ATT_ERR_ATTRIBUTE_NOT_FOUND 0x0A
#define ATT_ERR_ATTRIBUTE_NOT_LONG 0x0B
#define ATT_ERR_INSUFFICIENT_EK_SIZE 0x0C
#define ATT_ERR_INVALID_ATTRI_VALUE_LEN 0x0D
#define ATT_ERR_UNLIKELY_ERROR 0x0E
#define ATT_ERR_INSUFFICIENT_ENCRYPTION 0x0F
#define ATT_ERR_UNSUPPORTED_GROUP_TYPE 0x10
#define ATT_ERR_INSUFFICIENT_RESOURCES 0x11
#define ATT_ERR_APPLICATION_ERROR 0x80

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}


typedef struct{
	UINT16 gatt_client_state ;
	UINT16 ble_handle_c;	
	gatt_prime_service_t gatt_client_prime_service;
	gatt_chara_t gatt_client_chara[5];
	uint8 chara_count;
	gatt_chara_desc_t gatt_client_chara_desc[5];
	uint8 chara_desc_count;
	uint8 gatt_client_demo_init;
	uint8 find_info_index;
	uint8 config_index;
	uint16 target_uuid;
	uint8 write_enable;
	uint8 connect;
	int first_time_scan;

}fibo_ble_client_param_t;


fibo_ble_client_param_t  fibo_ble_client_param;


void fibo_ble_client_param_init()
{
   memset(&fibo_ble_client_param,0,sizeof(fibo_ble_client_param_t));
   fibo_ble_client_param.target_uuid = 0x1811;
   //fibo_ble_client_param.target_uuid = 0xFFE3;
   fibo_ble_client_param.ble_handle_c = 0xFF;
   fibo_ble_client_param.first_time_scan = 1;
}

static int AddrU8IntToStrings(char *_src, char *_des)
{
	sprintf(_des, "%02x%s%02x%s%02x%s%02x%s%02x%s%02x", _src[0], ":", _src[1], ":", _src[2], ":", _src[3], ":", _src[4], ":", _src[5]);
	return 0;
}

static int AddrU8IntToStrings_bigend(char *_src, char *_des)
{
	sprintf(_des, "%02x%s%02x%s%02x%s%02x%s%02x%s%02x", _src[5], ":", _src[4], ":", _src[3], ":", _src[2], ":", _src[1], ":", _src[0]);
	return 0;
}


void Gatt_client_demo_run_statemachine(GATT_CLIENT_STATE next_state)
{
    UINT8 test_value[2] = {0};
    test_value[0] = 0x01;
    test_value[1] = 0x00;
	OSI_PRINTFI("[AUTO_BLE][%s:%d] next_state = %d", __FUNCTION__, __LINE__,next_state);
    switch (next_state)
    {
    case GATT_CLIENT_DISCOVER_PRME_SER:
        fibo_ble_client_param.gatt_client_state = GATT_CLIENT_DISCOVER_PRME_SER;
        fibo_ble_client_discover_primary_service_by_uuid(28, NULL, fibo_ble_client_param.target_uuid, fibo_ble_client_param.ble_handle_c);
        break;
    case GATT_CLIENT_READ_INCLUDE:
        fibo_ble_client_param.gatt_client_state = GATT_CLIENT_READ_INCLUDE;
        fibo_ble_client_find_include_service(28,fibo_ble_client_param.gatt_client_prime_service.startHandle, fibo_ble_client_param.gatt_client_prime_service.endHandle, fibo_ble_client_param.ble_handle_c);
        break;
    case GATT_CLIENT_READ_CHARA_DEC:
        fibo_ble_client_param.gatt_client_state = GATT_CLIENT_READ_CHARA_DEC;
        fibo_ble_client_discover_all_characteristic(28, fibo_ble_client_param.gatt_client_prime_service.startHandle, fibo_ble_client_param.gatt_client_prime_service.endHandle, fibo_ble_client_param.ble_handle_c);
        break;
    case GATT_CLIENT_FIND_INFO:
        fibo_ble_client_param.gatt_client_state = GATT_CLIENT_FIND_INFO;

        while (((fibo_ble_client_param.find_info_index + 1) < fibo_ble_client_param.gatt_client_prime_service.charNum) && ((fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index].valueHandle + 1) == fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index + 1].handle))
        {
            fibo_ble_client_param.find_info_index++;
        }

        //find information
        //midle of chara
        OSI_PRINTFI("[AUTO_BLE][%s:%d] find_info_index = 0x%x",__FUNCTION__, __LINE__, fibo_ble_client_param.find_info_index);
        OSI_PRINTFI("[AUTO_BLE][%s:%d] chara_count = 0x%x",__FUNCTION__, __LINE__, fibo_ble_client_param.chara_count);
        if (fibo_ble_client_param.find_info_index < (fibo_ble_client_param.chara_count - 1))
        {
            OSI_PRINTFI("[AUTO_BLE][%s:%d] (gatt_client_chara[find_info_index].valueHandle + 1) = 0x%x",__FUNCTION__, __LINE__, (fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index].valueHandle + 1));
            OSI_PRINTFI("[AUTO_BLE][%s:%d] (gatt_client_chara[find_info_index + 1].handle -1) = 0x%x",__FUNCTION__, __LINE__, (fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index + 1].handle - 1));
            fibo_taskSleep(500);
            fibo_ble_client_get_char_descriptor(28, (fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index].valueHandle + 1), (fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index + 1].handle - 1), fibo_ble_client_param.ble_handle_c);
        }
        else //end of chara
        {
            if (fibo_ble_client_param.gatt_client_prime_service.endHandle >= (fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index].valueHandle + 1))
            {
                
			    OSI_PRINTFI("[AUTO_BLE][%s:%d](gatt_client_chara[find_info_index].valueHandle + 1) = 0x%x", __FUNCTION__, __LINE__, (fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index].valueHandle + 1));
                OSI_PRINTFI("[AUTO_BLE][%s:%d] gatt_client_prime_service.endHandle = 0x%x",__FUNCTION__, __LINE__, fibo_ble_client_param.gatt_client_prime_service.endHandle);
                fibo_ble_client_get_char_descriptor(28, (fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index].valueHandle + 1), fibo_ble_client_param.gatt_client_prime_service.endHandle, fibo_ble_client_param.ble_handle_c);
            }
            else
            {
                fibo_ble_client_param.gatt_client_state = GATT_CLIENT_SET_CONFIG;
                Gatt_client_demo_run_statemachine(GATT_CLIENT_SET_CONFIG);
            }
        }
        break;
    case GATT_CLIENT_SET_CONFIG:
        for (; fibo_ble_client_param.config_index < fibo_ble_client_param.chara_desc_count; fibo_ble_client_param.config_index++)
        {
            if (fibo_ble_client_param.gatt_client_chara_desc[fibo_ble_client_param.config_index].uuid == 0x2902)
            {
				osiThreadSleep(500);
                fibo_ble_client_write_char_descriptor(28, fibo_ble_client_param.gatt_client_chara_desc[fibo_ble_client_param.config_index].handle, test_value, 2, fibo_ble_client_param.ble_handle_c);
            }
        }
        break;
    default:
        break;
    }
}

void Gatt_client_demo_mtu_exchange_result_cb(UINT16 handle, UINT16 mtu)
{
    SCI_TRACE_LOW("[BLE_client]mtu_exchange - MTU = 0x%x", mtu);
    //after exchange mtu,we discover all primary service
    if ((fibo_ble_client_param.gatt_client_state == GATT_CLIENT_LINK_CONNECTED) && (fibo_ble_client_param.gatt_client_demo_init == TRUE))
        Gatt_client_demo_run_statemachine(GATT_CLIENT_DISCOVER_PRME_SER);
}

static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
	   
		//fibo_PDPActive /fibo_asyn_PDPActive  pdp active status report
		case GAPP_SIG_PDP_ACTIVE_IND:
		{
			UINT8 cid = (UINT8)va_arg(arg, int);
			OSI_PRINTFI("sig_res_callback  cid = %d", cid);
			va_end(arg);

		}
		break;

		//fibo_PDPRelease /fibo_asyn_PDPRelease pdp deactive status report
		case GAPP_SIG_PDP_RELEASE_IND:
		{
			UINT8 cid = (UINT8)va_arg(arg, int);
			OSI_PRINTFI("sig_res_callback  cid = %d", cid);
			va_end(arg);
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
		break;
		case GAPP_SIG_BLE_CONNECT_IND:
		{
			int connect_id = (int)va_arg(arg, int);
			int state = (int)va_arg(arg, int);
			UINT8 *addr = (UINT8 *)va_arg(arg, UINT8 *);
			UINT8 reason = (UINT8 )va_arg(arg, int);
			va_end(arg);
			OSI_PRINTFI("[AUTO_BLE][%s:%d]type=%d,state=%d,%p,%d", __FUNCTION__, __LINE__,connect_id,state,addr,reason);
			if(addr != NULL)
			{
				OSI_PRINTFI("[AUTO_BLE][%s:%d]type=%d,state=%d,%s,%d", __FUNCTION__, __LINE__,connect_id,state,addr,reason);
			}

			if(state == 0)
			{
				//fibo_ble_enable_dev(1); // open broadcast
			}
		}
		break;

		//PDP in active state, deactive indicator received from modem  
		case GAPP_SIG_PDP_DEACTIVE_ABNORMALLY_IND:
		{
		
			UINT8 cid = (UINT8)va_arg(arg, int);
			OSI_PRINTFI("[%s-%d]cid = %d", __FUNCTION__, __LINE__,cid);
			va_end(arg);
		}
		break;
		
	    default:
	    {
	        break;
	    }
    }
    OSI_LOGI(0, "test");
}

static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = sig_res_callback};
	
bt_status_t fibo_connection_state_change_cb(int conn_id, int connected, bdaddr_t *addr)
{
    //btgatt_interface_t *p_gatt_if = btgatt_interface;
    OSI_PRINTFI("[AUTO_BLE][%s:%d]connected=%d", __FUNCTION__, __LINE__,connected);
    switch (connected)
    {
    case TRUE: //GATT_CONNECT_CNF:
		OSI_PRINTFI("[AUTO_BLE][%s:%d],conn_id=%d", __FUNCTION__, __LINE__,conn_id);
        fibo_ble_client_param.ble_handle_c = conn_id;
        fibo_ble_client_param.gatt_client_state = GATT_CLIENT_LINK_CONNECTED;
		fibo_ble_client_param.connect = 1;
		fibo_ble_client_param.target_uuid = 0x1811;
        fibo_taskSleep(600);
        //fibo_ble_client_discover_all_primary_service(28,fibo_ble_client_param.ble_handle_c);
        fibo_ble_client_discover_primary_service_by_uuid(28,NULL,fibo_ble_client_param.target_uuid,fibo_ble_client_param.ble_handle_c);
        break;
    case FALSE: //GATT_DISCONNECT_IND:
		OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
		memset(&fibo_ble_client_param,0,sizeof(fibo_ble_client_param_t));
        fibo_ble_client_param.ble_handle_c = 0xFF;
        fibo_ble_client_param.gatt_client_state = GATT_CLIENT_LINK_DISCONNECT;
		fibo_ble_client_param.connect = 0;
		// maybe need scan again and connect again
		
        fibo_ble_scan_enable(1);
		fibo_ble_client_param.first_time_scan = 1;
        break;

    default:
        break;
    }
    return BT_SUCCESS;
}



bt_status_t fibo_discover_service_by_uuid_cb (void *parma)
{
    gatt_prime_service_t *p_service_info = (gatt_prime_service_t *)parma;

    fibo_ble_client_param.gatt_client_prime_service.startHandle = p_service_info->startHandle;
    fibo_ble_client_param.gatt_client_prime_service.endHandle = p_service_info->endHandle;
    fibo_ble_client_param.gatt_client_prime_service.uuid = fibo_ble_client_param.target_uuid;
    OSI_PRINTFI("[AUTO_BLE][%s:%d] startHandle = 0x%x",__FUNCTION__, __LINE__,fibo_ble_client_param.gatt_client_prime_service.startHandle);
    OSI_PRINTFI("[AUTO_BLE][%s:%d] endHandle = 0x%x", __FUNCTION__, __LINE__,fibo_ble_client_param.gatt_client_prime_service.endHandle);

    Gatt_client_demo_run_statemachine(GATT_CLIENT_READ_INCLUDE);
    return BT_SUCCESS;
}

bt_status_t fibo_discover_service_all_cb (void *parma)
{
   OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
   gatt_prime_service_t *gatt_prime_service = (gatt_prime_service_t *)parma;
   OSI_PRINTFI("[AUTO_BLE][%s:%d]uuid=%x,%d", __FUNCTION__, __LINE__,gatt_prime_service->uuid,gatt_prime_service->charNum);

   if(gatt_prime_service->uuid == fibo_ble_client_param.target_uuid)
   {
	   fibo_ble_client_discover_all_characteristic(28,gatt_prime_service->startHandle,gatt_prime_service->endHandle,fibo_ble_client_param.ble_handle_c);

   }
   OSI_PRINTFI("[AUTO_BLE][%s:%d]uuid=%x", __FUNCTION__, __LINE__,gatt_prime_service->uuid);
   return BT_SUCCESS;
}
bt_status_t fibo_char_des_data(void *param)
{
    att_server_t *pServer = (att_server_t *)param;
    UINT8 i = 0;
    UINT8 fmt = pServer->lastRsp.payLoad[0];
    UINT8 length = pServer->lastRsp.length;

    OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
    //fmt:1 - 16 bit uuid
    if (fmt == 1)
    {
        while (i < (length - 1) / 4)
        {
            fibo_ble_client_param.gatt_client_chara_desc[fibo_ble_client_param.chara_desc_count].handle = (pServer->lastRsp.payLoad[i * 4 + 2] << 8) | pServer->lastRsp.payLoad[i * 4 + 1];
            fibo_ble_client_param.gatt_client_chara_desc[fibo_ble_client_param.chara_desc_count].uuid = (pServer->lastRsp.payLoad[i * 4 + 4] << 8) | pServer->lastRsp.payLoad[i * 4 + 3];
            fibo_ble_client_param.chara_desc_count++;
            i++;
			OSI_PRINTFI("[AUTO_BLE][%s:%d] handle = 0x%x", __FUNCTION__, __LINE__,fibo_ble_client_param.gatt_client_chara_desc[fibo_ble_client_param.chara_desc_count].handle);
        }
        //update number
        fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index].descNum = fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index].descNum + i;
        //end of find infor of this time
        if ((fibo_ble_client_param.gatt_client_chara_desc[fibo_ble_client_param.chara_desc_count - 1].handle == fibo_ble_client_param.gatt_client_prime_service.endHandle) || (fibo_ble_client_param.gatt_client_chara_desc[fibo_ble_client_param.chara_desc_count - 1].handle == (fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index + 1].handle - 1)))
        {
            //update list
            fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index].pDescList = &fibo_ble_client_param.gatt_client_chara_desc[fibo_ble_client_param.chara_desc_count - fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.find_info_index].descNum];
            fibo_ble_client_param.find_info_index++;
            Gatt_client_demo_run_statemachine(GATT_CLIENT_FIND_INFO);
        }
    }
    return BT_SUCCESS;
}

bt_status_t fibo_char_data (void *parma, UINT8 more_data)
{
    att_server_t *pServer = (att_server_t *)parma;
    UINT8 i = 0;
    UINT8 pair_len = pServer->lastRsp.payLoad[0];
    OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);

    while (i < (pServer->lastRsp.length - 1) / pair_len)
    {
        fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.chara_count].handle = (pServer->lastRsp.payLoad[i * pair_len + 2] << 8) | pServer->lastRsp.payLoad[i * pair_len + 1];
        fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.chara_count].properties = pServer->lastRsp.payLoad[i * pair_len + 3];
        fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.chara_count].valueHandle = (pServer->lastRsp.payLoad[i * pair_len + 5] << 8) | pServer->lastRsp.payLoad[i * pair_len + 4];
        fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.chara_count].uuid = (pServer->lastRsp.payLoad[i * pair_len + 7] << 8) | pServer->lastRsp.payLoad[i * pair_len + 6];
        OSI_PRINTFI("[AUTO_BLE][%s:%d]att_handle = 0x%x",__FUNCTION__, __LINE__, fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.chara_count].handle);
        OSI_PRINTFI("[AUTO_BLE][%s:%d]properties = 0x%x", __FUNCTION__, __LINE__,fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.chara_count].properties);
        OSI_PRINTFI("[AUTO_BLE][%s:%d]value_handle = 0x%x", __FUNCTION__, __LINE__,fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.chara_count].valueHandle);
        OSI_PRINTFI("[AUTO_BLE][%s:%d]char_uuid = 0x%x", __FUNCTION__, __LINE__,fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.chara_count].uuid);

        fibo_ble_client_param.chara_count++;
        i++;
    }

    fibo_ble_client_param.gatt_client_prime_service.charNum = fibo_ble_client_param.gatt_client_prime_service.charNum + i;
    OSI_PRINTFI("[AUTO_BLE][%s:%d]charNum = 0x%x",__FUNCTION__, __LINE__, fibo_ble_client_param.gatt_client_prime_service.charNum);
    //read this prime service characteristic dedaration done
    //todo check (chara_count-1)<0
    if (fibo_ble_client_param.gatt_client_chara[fibo_ble_client_param.chara_count - 1].valueHandle == fibo_ble_client_param.gatt_client_prime_service.endHandle)
    {
        //find information request
        fibo_ble_client_param.find_info_index = 0;
        Gatt_client_demo_run_statemachine(GATT_CLIENT_FIND_INFO);
    }
    return BT_SUCCESS;
}
bt_status_t fibo_read_cb (void *parma)
{
   OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
   att_server_t *att_server = (att_server_t *)parma;
   OSI_PRINTFI("[AUTO_BLE][%s:%d],length=%d,buffer=%s", __FUNCTION__, __LINE__,att_server->lastRsp.length,att_server->lastRsp.payLoad);
   return BT_SUCCESS;
}
bt_status_t fibo_read_blob_cb (void *parma)
{
   OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
   att_server_t *att_server = (att_server_t *)parma;
   return BT_SUCCESS;
}
bt_status_t fibo_read_multi_cb (void *parma)
{
   OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
   att_server_t *att_server = (att_server_t *)parma;
   return BT_SUCCESS;
}
bt_status_t fibo_recv_notification_cb (void *parma)
{
    OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	att_server_t *att_server = (att_server_t *)parma;
	OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%s", __FUNCTION__, __LINE__,att_server->lastReqPdu.attValuelen,att_server->lastReqPdu.attValue);
	OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%s", __FUNCTION__, __LINE__,att_server->lastNoti.length,&att_server->lastNoti.payLoad[2]);
	for(int i =0;i<att_server->lastNoti.length;i++)
	{
		OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%x", __FUNCTION__, __LINE__,att_server->lastNoti.length,att_server->lastNoti.payLoad[i]);
	}
	
	return BT_SUCCESS;
}
bt_status_t fibo_recv_indication_cb (void *parma)
{
    OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	att_server_t *att_server = (att_server_t *)parma;
	OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%s", __FUNCTION__, __LINE__,att_server->lastReqPdu.attValuelen,att_server->lastReqPdu.attValue);
	OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%s", __FUNCTION__, __LINE__,att_server->lastNoti.length,&att_server->lastNoti.payLoad[2]);
	for(int i =0;i<att_server->lastNoti.length;i++)
	{
		OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%x", __FUNCTION__, __LINE__,att_server->lastNoti.length,att_server->lastNoti.payLoad[i]);
	}

	return BT_SUCCESS;
}

int write_enable = 0;
bt_status_t fibo_write_cb (void *parma)
{
    OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	att_req_pdu_t *att_req_pdu = (att_req_pdu_t *)parma;
	fibo_ble_client_param.write_enable =1;
	return BT_SUCCESS;
}
bt_status_t fibo_write_rsp_cb (void *parma)
{
     OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	 UINT16 *aclHandle = (UINT16 *)parma;
	 return BT_SUCCESS;
}


bt_status_t fibo_scan_cb (void *pdu,UINT8 len)
{
    //OSI_PRINTFI("[AUTO_BLE][%s:%d],adv_len=%d", __FUNCTION__, __LINE__,adv_len);
	st_scan_report_info *scan_report = (st_scan_report_info *)pdu;
	OSI_PRINTFI("[AUTO_BLE][%s:%d] %s,%d,addr_type:%d", __FUNCTION__, __LINE__,scan_report->name,scan_report->name_length,scan_report->addr_type);

	if(scan_report->name_length > 0)
	{
	    
		//if(!strncasecmp("D0000031",scan_report->name,8))
		if(!strncasecmp("8910_ble",scan_report->name,8))
		{		    
			if(scan_report->bdAddress.addr[0]==0 && scan_report->bdAddress.addr[1]==0 && scan_report->bdAddress.addr[2]==0 && scan_report->bdAddress.addr[3]==0 && scan_report->bdAddress.addr[4]==0 && scan_report->bdAddress.addr[5]==0)
			{
                //searching on going
                OSI_PRINTFI("[AUTO_BLE][%s:%d] %s,addr_type:%d", __FUNCTION__, __LINE__,scan_report->name,scan_report->addr_type);
				fibo_ble_scan_enable(1);
			}
			else
			{
			    
				if(fibo_ble_client_param.first_time_scan)
				{
				    fibo_ble_client_param.first_time_scan = 0;
					fibo_ble_scan_enable(0);
					fibo_taskSleep(2000);
					char addr[30] = {0};
					AddrU8IntToStrings(scan_report->bdAddress.addr,addr);
					OSI_PRINTFI("[AUTO_BLE][%s:%d] %s,add=%s,addr_type:%d", __FUNCTION__, __LINE__,scan_report->name,addr,scan_report->addr_type);
					fibo_ble_connect(1,scan_report->addr_type,addr);

				}

			}

		}
	}
	 return BT_SUCCESS;
	
}
void fibo_smp_pair_success_cb (void)
{
     OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	 return ;
}
void fibo_smp_pair_failed_cb(void)
{
     OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	 return ;
}
void fibo_att_error_cb(UINT8 error_code)
{
    SCI_TRACE_LOW("[BLE_client] Gatt_client_demo_att_error_cb error = 0x%x", error_code);
    switch (error_code)
    {
    case ATT_ERR_INVALID_HANDLE:
        break;

    case ATT_ERR_READ_NOT_PERMITTED:
        break;

    case ATT_ERR_WRITE_NOT_PERMITTED:
        break;

    case ATT_ERR_INVALID_PDU:
        break;

    case ATT_ERR_INSUFFICIENT_AUTHEN:
        break;

    case ATT_ERR_REQUEST_NOT_SUPPORT:
        break;

    case ATT_ERR_INVALID_OFFSET:
        break;

    case ATT_ERR_INSUFFICIENT_AUTHOR:
        break;

    case ATT_ERR_PREPARE_QUEUE_FULL:
        break;

    case ATT_ERR_ATTRIBUTE_NOT_FOUND:

        switch (fibo_ble_client_param.gatt_client_state)
        {
        case GATT_CLIENT_DISCOVER_PRME_SER:
			OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
            //p_gatt_if->client->disconnect(ble_handle_c);
            fibo_ble_connect(fibo_ble_client_param.ble_handle_c,0,NULL);
            break;
        case GATT_CLIENT_READ_INCLUDE:
            //gatt_client_state = GATT_CLIENT_READ_INCLUDE_DONE;
            OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
            Gatt_client_demo_run_statemachine(GATT_CLIENT_READ_CHARA_DEC);
            break;
        case GATT_CLIENT_READ_CHARA_DEC:
            //gatt_client_state = GATT_CLIENT_READ_CHARA_DEC_DONE;
            fibo_ble_client_param.find_info_index = 0;
			OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
            Gatt_client_demo_run_statemachine(GATT_CLIENT_FIND_INFO);
            break;
		case GATT_CLIENT_FIND_INFO:
			//gatt_client_state = GATT_CLIENT_READ_CHARA_DEC_DONE;
            fibo_ble_client_param.find_info_index = 0;
			OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
            Gatt_client_demo_run_statemachine(GATT_CLIENT_FIND_INFO);
        default:
            break;
        }

        break;

    case ATT_ERR_ATTRIBUTE_NOT_LONG:
        break;

    case ATT_ERR_INSUFFICIENT_EK_SIZE:
        break;

    case ATT_ERR_INVALID_ATTRI_VALUE_LEN:
        break;

    case ATT_ERR_UNLIKELY_ERROR:
        break;

    case ATT_ERR_INSUFFICIENT_ENCRYPTION:
        break;

    case ATT_ERR_UNSUPPORTED_GROUP_TYPE:
        break;

    case ATT_ERR_INSUFFICIENT_RESOURCES:
        break;

    default:
        if ((error_code >= 0x12) && (error_code <= 0x7f))
        {
            //Reserved for future
        }
        else //0x80<=error_code<=0xff
        {
            //Defined by higher layer application
        }
        break;
    }
}

static fibo_ble_client_callbacks_t fibo_ble_client_callback={
	.connection_state_change_cb = fibo_connection_state_change_cb,
	.discover_service_by_uuid_cb = fibo_discover_service_by_uuid_cb,
	.discover_service_all_cb = fibo_discover_service_all_cb,
	.char_des_data = fibo_char_des_data,
	.char_data = fibo_char_data,
	.read_cb = fibo_read_cb,
	.read_blob_cb = fibo_read_blob_cb,
	.read_multi_cb = fibo_read_multi_cb,
	.recv_notification_cb = fibo_recv_notification_cb,
	.recv_indication_cb = fibo_recv_indication_cb,
	.write_cb = fibo_write_cb,
	.write_rsp_cb = fibo_write_rsp_cb,
	.scan_cb = fibo_scan_cb,
	.smp_pair_success_cb = fibo_smp_pair_success_cb,
	.smp_pair_failed_cb = fibo_smp_pair_failed_cb,
	.att_error_cb = fibo_att_error_cb,
};

static fibo_ble_btgatt_callback_t fibo_ble_btgatt_callback={
	.client = &fibo_ble_client_callback,
	.server = NULL,
};

#if 0
/*
pdu[0]:event type
pdu[1]:address type
pdu[2--7]:peer address
pdu[8]:data length
pdu[9--(8 + data_lenght)]:data // broadcast data
pdu[9 + data_lenght]:rssi
*/
//pdu[9--(8 + data_lenght)]:data // broadcast data  for example : 0201060C09383931305f626c655f6466 
//02 --- LENGTH
//01 --- TYPE
//06 --- value
//0c --- LENGTH
//09 --- TYPE(??)
//383931305f626c655f6466(8910_ble_df) ---value
bt_status_t scan_broadcast_cb_f(void *data,uint8 adv_len)
{

    uint8 buffer[64] = {0};
	memcpy(buffer,(uint8 *)data,adv_len);
    for(int i = 0;i< adv_len;i++)
	{
	   OSI_PRINTFI("[AUTO_BLE][%s:%d]data_length = %d, broadcast: %x", __FUNCTION__, __LINE__,adv_len,buffer[9+i]);
	}
}
#endif

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);

	uint32_t chipid = 0;
	uint32_t metaid = 0;

	fibo_taskSleep(10000);
	fibo_get_chipid(&chipid,&metaid);
	OSI_PRINTFI("[AUTO_BLE][%s:%d]chipid=0x%x,metaid=0x%x", __FUNCTION__, __LINE__,chipid,metaid);
	fibo_taskSleep(1000);
	
    fibo_ble_client_param_init();
    fibo_bt_onoff(1);
	fibo_taskSleep(2000);
	//fibo_ble_get_broadcast_data(scan_broadcast_cb_f);
	fibo_ble_set_read_name(0,(uint8_t *)"8910_ble_client",0); // set ble name 
	fibo_taskSleep(2000);
	fibo_ble_client_server_int(&fibo_ble_btgatt_callback);
	//fibo_ble_set_scan_param(1, 96,48, 0);
	fibo_ble_scan_enable(1);
	fibo_ble_client_param.first_time_scan = 1;
	fibo_taskSleep(20000);
	while(1)
	{
        OSI_LOGI(0, "hello world");
        fibo_taskSleep(10000);
        for(int i = 0;i< fibo_ble_client_param.chara_count;i++)
        {
             if(fibo_ble_client_param.gatt_client_chara[i].properties | ATT_CHARA_PROP_WRITE  && fibo_ble_client_param.connect)
             {
                   	uint8 test_val[6] = {0};
					#if 0
					test_val[0]= 0xEF;
					test_val[1]= 0xFE;
					test_val[2]= 0x00;
					test_val[3]= 0x00;
					test_val[4]= 0xC6;
					test_val[5]= 0xC7;
					#endif
					
					test_val[0]= 0x33;
					test_val[1]= 0x34;
					test_val[2]= 0x35;
					test_val[3]= 0x36;
					test_val[4]= 0x37;
					test_val[5]= 0x38;
					
					fibo_ble_client_write_char_value(28,fibo_ble_client_param.gatt_client_chara[i].valueHandle,test_val,sizeof(test_val),fibo_ble_client_param.ble_handle_c);
					OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
					fibo_taskSleep(1000);
			 }
			 fibo_taskSleep(1000);
			 
			 if(fibo_ble_client_param.gatt_client_chara[i].properties | ATT_CHARA_PROP_READ && fibo_ble_client_param.connect)
             {
	
					//fibo_ble_client_read_char_value_by_uuid(28,NULL,fibo_ble_client_param.gatt_client_chara[i].uuid,fibo_ble_client_param.gatt_client_chara[i].valueHandle+1,fibo_ble_client_param.gatt_client_chara[i].valueHandle-1,fibo_ble_client_param.ble_handle_c);
					OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
					fibo_taskSleep(1000);
					fibo_ble_client_read_char_value_by_handle(28,fibo_ble_client_param.gatt_client_chara[i].valueHandle,0,fibo_ble_client_param.ble_handle_c);
					OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
					fibo_taskSleep(1000);
			 }
			 
		}
	}
    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}

