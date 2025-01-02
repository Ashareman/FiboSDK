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
#include "oc_uart.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

uint8_t register_network_srand = 16;
#define MIDEA_DELAY_FILE_NAME "/delay.bin"
#define MIDEA_SRAND_RATIO 30

#define GHT_MIDEA_UART_DATA_MAX_LEN 257//128
#define GHT_MIDEA_PACKET_HEAD_LEN                   10
#define GHT_MIDEA_SYNC_HEADER                       0xAA
#define GHT_MIDEA_BROADCAST_DEVICE_TYPE             0xFF
#define GHT_MIDEA_WORK_SELF_TEST1                   0x14
#define GHT_MIDEA_WORK_NETWOR_ACCESS_SWITCH         0x64
#define GHT_MIDEA_WORK_MODEM_RESTART                0x82
#define GHT_MIDEA_WORK_FACTORY_RESET                0x83
#define GHT_MIDEA_WORK_NETWOR_ACCESS_SWITCH_LEN     (20)
#define GHT_MIDEA_APPLIANCE_GENERAL_SN_REQ_LEN      1
#define GHT_MIDEA_NO_AIR_CONDITION_DEVICE_SN        0x07
#define GHT_MIDEA_AIR_CONDITION_INFO                0x65
#define GHT_MIDEA_APPLIANCE_AIR_SN_REQ_LEN          20
#define GHT_MIDEA_APPLIANCE_TYPE_AND_INFO_REQ_LEN   20
#define GHT_MIDEA_APPLIANCE_TYPE_AND_INFO           0xA0
#define GHT_MIDEA_APPLIANCE_SN_LEN                  32
#define GHT_MIDEA_APPLIANCE_SN_HEAD_LEN             40
#define GHT_MIDEA_AIR_CONDITION_SN_LEN              29

struct ght_midea_uart_data
{
    uint8_t data_buf[GHT_MIDEA_UART_DATA_MAX_LEN];  
    uint16_t recv_data_len;
    uint16_t all_data_len;
};

struct ght_midea_uart_data    g_ght_midea_uart_data;
UINT32 ght_midea_event_lock;
uint8_t self_test_0x14 = 0;
static  uint8_t  g_ght_message_flag = 0;
struct  ght_midea_func_event* g_ght_event_proc = NULL;
static  uint8_t g_ght_home_appliance_type = 0xFF;
uint16_t g_ght_midea_query_sn_count = 0;
bool g_is_self_test = false;
uint8_t sn_valid_flag = 0;
UINT32	self_test_timer = 0;

typedef enum _GHT_MIDEA_EVENT_ID_
{
    GHT_MIDEA_EVENT_NONE = 0,
    GHT_MIDEA_EVENT_GENERAL_SN_REQ = 1,
    GHT_MIDEA_EVENT_UART_DATA_IND = 2,
    GHT_MIDEA_EVENT_AIR_SN_REQ = 3,
    GHT_MIDEA_EVENT_TYPE_AND_INFO_REQ = 4,
    GHT_MIDEA_EVENT_NETWORK_STATUS = 5,
    GHT_MIDEA_EVENT_START_GTOTA = 6,
    GHT_MIDEA_EVENT_NO_AIR_SN_REQ =7,
    GHT_MIDEA_EVENT_MAX
}GHT_MIDEA_EVENT_ID;

struct ght_midea_data_packet
{
    uint8_t sync_head;  
    uint8_t msg_len;
    uint8_t home_elec_type;
    uint8_t sync_check;
    uint8_t reserve[2];
    uint8_t message_flag;
    uint8_t frame_version;
    uint8_t home_elec_version;
    uint8_t msg_type_flag;
    uint8_t *data;
    uint8_t check;
};

struct ght_midea_func_event
{
    uint16_t  event_id;
    uint16_t  data_len;
    void      *data;
    struct   ght_midea_func_event* next;
};
typedef struct _FIBO_STATE_FLAG
{
    uint8_t data_send_recv_flag;
    uint8_t midea_fota_flag;
    uint8_t ght_is_first_req;
    uint8_t regFuncFlag;
    uint8_t getSnFlag;
    uint8_t is_waitip;
    uint8_t disableReset;
    uint8_t g_ght_login_complete;
    uint8_t ght_midea_module_cereg;
}FIBO_STATE_FLAG;
FIBO_STATE_FLAG     g_fibo_state_flag = {0,0,0,0,0,0,0,1,1};
int g_timer_id = 0;

uint8_t g_ght_midea_network_access_switch[GHT_MIDEA_WORK_NETWOR_ACCESS_SWITCH_LEN] = {0};
uint8_t  g_ght_midea_appliance_sn[GHT_MIDEA_APPLIANCE_SN_LEN+1] = {0};
UINT32 midea_sn_timer = 0;
void ght_midea_uart_recv_data(hal_uart_port_t uart_port,UINT8 *buf, UINT16 data_len,void *arg);
void ght_midea_task_send_event(uint8_t event_id, void* data, uint16_t data_len);

void ght_midea_timer_query_sn_expire(void *arg)
{
#define GET_SN_MAX_TRY_TIMES  850
    if(g_fibo_state_flag.disableReset==1)
        return;
    OSI_PRINTFI("g_ght_midea_query_sn_count=%d",g_ght_midea_query_sn_count);

    if ((GET_SN_MAX_TRY_TIMES <= g_ght_midea_query_sn_count) && (g_fibo_state_flag.disableReset == 0))
    {
        OSI_PRINTFI("fibo_get sn error timeout reset");
        fibo_taskSleep(50);
        fibo_softReset();
    }

    else if ((0 == g_ght_midea_query_sn_count%2) && (g_ght_midea_query_sn_count<GET_SN_MAX_TRY_TIMES))
    {
        OSI_PRINTFI("send GHT_MIDEA_EVENT_GENERAL_SN_REQ ");
        ght_midea_task_send_event(GHT_MIDEA_EVENT_GENERAL_SN_REQ, NULL, 0);
    }
    else if((1 == g_ght_midea_query_sn_count%2) && (g_ght_midea_query_sn_count<GET_SN_MAX_TRY_TIMES))
    {
        OSI_PRINTFI("send GHT_MIDEA_EVENT_AIR_SN_REQ ");
        ght_midea_task_send_event(GHT_MIDEA_EVENT_AIR_SN_REQ, NULL, 0);
    }
}

uint8_t* ght_midea_packet_construction_uart_data(uint8_t message_len,
	                                            uint8_t type,
	                                            uint8_t sync_check,
	                                            uint8_t message_flag,
	                                            uint8_t frame_version,
	                                            uint8_t elec_version,
	                                            uint8_t type_flag,
	                                            uint8_t *data);
void ght_midea_func_init(void)
{
    ght_midea_event_lock = fibo_mutex_create();
    midea_sn_timer = fibo_timer_create(ght_midea_timer_query_sn_expire, NULL);
    OSI_LOGI(0, "ght_midea_sn_timer id: 0x%x", midea_sn_timer);
    fibo_timer_period_start(midea_sn_timer, 3000);
}
void fibo_stopselftesttimer(void *arg)
{
    g_is_self_test = false;
    OSI_LOGI(0, "g_is_self_test =  %d", g_is_self_test);
    fibo_timer_stop(self_test_timer);
    fibo_timer_free(self_test_timer);
}

void fibo_startselftesttimer()
{
    self_test_timer = fibo_timer_create(fibo_stopselftesttimer, NULL);
    g_is_self_test = true;
    OSI_LOGI(0, "g_is_self_test =  %d", g_is_self_test);
    fibo_timer_start(self_test_timer, 10*1000);
}

void timer_function(void *arg)
{
    int ret=-1;
    ret = fibo_cfun_one();
    if(-1 == ret)
    {
        OSI_LOGI(0, "fibo_cfun_one fail ...");
    }
    OSI_LOGI(0, "fibo_cfun_one success ...");
    fibo_timer_free(g_timer_id);
}

uint8_t* ght_midea_packet_construction_uart_data(uint8_t message_len,
                                                uint8_t type,
                                                uint8_t sync_check,
                                                uint8_t message_flag,
                                                uint8_t frame_version,
                                                uint8_t elec_version,
                                                uint8_t type_flag,
                                                uint8_t *data)
{
    uint8_t* p_packet = NULL;
    uint8_t  data_len = 0, i = 0;
    uint8_t  check_sum = 0;

    if (message_len < GHT_MIDEA_PACKET_HEAD_LEN)
        return NULL;
    data_len = message_len - GHT_MIDEA_PACKET_HEAD_LEN;
    p_packet = (uint8_t*)fibo_malloc(message_len +1 + 1);
    OSI_PRINTFI("%s,%d: p_packet = %x", __FUNCTION__, __LINE__, (long)p_packet);
    memset(p_packet, 0x00, (message_len +1 + 1));
    p_packet[0] = 0xAA;
    p_packet[1] = message_len;
    p_packet[2] = type;
    p_packet[6] = message_flag;
    p_packet[8] = elec_version;
    p_packet[9] = type_flag;
    if (NULL != data)
    {
    memcpy(&p_packet[10], data, data_len);
    }
    //Minus head,minus check
    for(i = 1; i < message_len; i++)
    {
    check_sum += p_packet[i];
    }
    p_packet[message_len] = ~check_sum + 1;
    return p_packet;
}
void ght_midea_task_send_event(uint8_t event_id, void* data, uint16_t data_len)
{
    struct ght_midea_func_event* p_event = NULL;
    struct ght_midea_func_event* p_temp_event = NULL;

    fibo_mutex_lock(ght_midea_event_lock);
    p_event = (struct ght_midea_func_event*)fibo_malloc(sizeof(struct ght_midea_func_event));
    memset(p_event, 0, sizeof(struct ght_midea_func_event));
    p_event->next = NULL;
    if (NULL == g_ght_event_proc)
    {
        g_ght_event_proc = p_event;
    }
    else
    {
        p_temp_event = g_ght_event_proc;
        while (NULL != p_temp_event->next)
        {
            p_temp_event = p_temp_event->next;
        }
        p_temp_event->next = p_event;
    }
    p_event->event_id = event_id;
    p_event->data_len = data_len;
    p_event->data = data;
    p_event->next = NULL;
    OSI_PRINTFI("send event_id =%d",event_id);
    fibo_mutex_unlock(ght_midea_event_lock);
}
static struct ght_midea_func_event* ght_midea_task_event_get(void)
{
    struct ght_midea_func_event* p_tmp = NULL;

    fibo_mutex_lock(ght_midea_event_lock);
    p_tmp = g_ght_event_proc;
    if (NULL == p_tmp)
    {
        fibo_mutex_unlock(ght_midea_event_lock);
        return NULL;
    }
    if (NULL != p_tmp->next)
    {
        g_ght_event_proc = p_tmp->next;
        p_tmp->next = NULL;
    }
    else
    {
        g_ght_event_proc = NULL;
    }
    fibo_mutex_unlock(ght_midea_event_lock);

    return p_tmp;
}
uint8_t ght_midea_packet_analysis_uart_data(char* buf, uint16_t buf_len, struct ght_midea_data_packet* data_packet)
{
    uint8_t data_len = 0, check_sum = 0;
    uint8_t i = 0;

    if (NULL == buf || GHT_MIDEA_PACKET_HEAD_LEN > buf_len || NULL == data_packet)
    {
        return 1;
    }
    if (0xAA != buf[0])
    {
        return 2;
    }
    //Minus head,minus check
    for (i = 1; i < buf_len-1; i++)
    {
        check_sum += buf[i];
    }
    check_sum = ~check_sum + 1;
    if (check_sum != buf[buf_len-1])
    {
        return 3;
    }
    data_packet->msg_len = buf[1];
    data_packet->home_elec_type = buf[2];
    data_packet->sync_check = buf[3];
    data_packet->message_flag = buf[6];
    data_packet->frame_version = buf[7];
    data_packet->home_elec_version = buf[8];
    data_packet->msg_type_flag = buf[9];
    if (buf_len > (GHT_MIDEA_PACKET_HEAD_LEN+1))
    {
        data_len = buf_len - GHT_MIDEA_PACKET_HEAD_LEN - 1;
        data_packet->data = (uint8_t*)fibo_malloc(data_len+1);
        memcpy(data_packet->data, &buf[10], data_len);
    }
    data_packet->check = check_sum;

    return 0;
}

uint32_t set_delay_register_network_time()
{
    uint8_t srand = 0;
    INT32 fd = 0;
    INT32 ret = 0;

    fd = fibo_file_open(MIDEA_DELAY_FILE_NAME, FS_O_WRONLY | FS_O_CREAT);
    if(fd < 0)
    {
        OSI_LOGI(0, "%s,%d Delay file write error");
        return fd;
    }

    ret = fibo_file_write(fd, &srand, 1);
    if(ret < 0)
    {
        OSI_LOGI(0, "Delay file write error");
        fibo_file_close(fd);
        return ret;
    }

    ret = fibo_file_close(fd);
    if(ret < 0)
    {
        OSI_LOGI(0, "close file error");
        return ret;
    }
    return 0;
}


uint32_t get_delay_register_network_time()
{
    uint32_t rand_num = -1;
    uint8_t srand = register_network_srand;
    INT32 fd = 0;
    INT32 ret = 0;

    fd = fibo_file_open(MIDEA_DELAY_FILE_NAME, FS_O_RDONLY);
    if(fd < 0)
    {
        OSI_LOGI(0, "Delay file open error");
        return fd;
    }

    ret = fibo_file_read(fd, &srand, 1);
    if(ret < 0)
    {
        OSI_LOGI(0, "Delay file read error");
        fibo_file_close(fd);
        return ret;
    }

    ret = fibo_file_close(fd);
    if(ret < 0)
    {
        OSI_LOGI(0, "close file error");
        return ret;
    }
    rand_num = srand*MIDEA_SRAND_RATIO+ rand()%MIDEA_SRAND_RATIO;
    OSI_LOGI(0, "rand time =%d",rand_num);
    return rand_num;
}

uint32_t ght_attach_start()
{
    int srand;
    int ret_imsi = -1;
    char imsi[20] = {0};
    int imsi_handle = 0;

    ret_imsi = fibo_get_imsi((UINT8 *)imsi);
    OSI_LOGI(0, "fibo_get_imsi result: %d", ret_imsi);
    if(0 != ret_imsi)
    {
        OSI_LOGI(0, "fibo_get_imsi failed");
        return -1;
    }
    for(int i=0;i<15;i++)
    {
        imsi[i] -= 0x30;
        OSI_LOGI(0, "fibo_get_imsi: %d", imsi[i]);
    }
    imsi_handle = imsi[11]*1000+imsi[12]*100+imsi[13]*10+imsi[14];
    OSI_LOGI(0, "imsi_handle: %d", imsi_handle);

    //set_delay_register_network_time();
    srand = get_delay_register_network_time();
    if(srand < 0)
    {
        OSI_LOGI(0, "get_delay_register_network_time failed");
        srand = register_network_srand*MIDEA_SRAND_RATIO+ rand()%MIDEA_SRAND_RATIO;
        OSI_LOGI(0, "srand %d",srand);
    }
    g_timer_id = fibo_timer_new((1+(imsi_handle%srand))*1000,timer_function,NULL);
    if(0 == g_timer_id)
    {
        OSI_LOGI(0, "fibo_timer_new failed");
        return -1;
    }
    return 0;
}

uint8_t ght_midea_packet_analysis_message_type(int32_t server_fd,
                                             struct ght_midea_data_packet* data_packet,
                                             char* buf, 
                                             uint16_t buf_len)
{
    uint8_t* p_buf = NULL;
    int port =0;
    int ret = -1;

    OSI_PRINTFI("data_packet->msg_type_flag=%d",data_packet->msg_type_flag);
    switch(data_packet->msg_type_flag)
    {
        /*<<NB-IoTƒ£øÈ¥Æø⁄Õ®—∂–≠“È 1.0.1>>µ⁄3.3Ω⁄ end*/
        /*<<NB-IoTƒ£øÈ¥Æø⁄Õ®—∂–≠“È 1.0.1>>µ⁄3.4Ω⁄ begin*/
        case GHT_MIDEA_NO_AIR_CONDITION_DEVICE_SN:
        {
            OSI_PRINTFI("packet_analysis receive:GHT_MIDEA_NO_AIR_CONDITION_DEVICE_SN, data_packet->msg_len=%d",
            data_packet->msg_len);
            int 	 data_in_len=0;
            int 	 data_len=32;
            int ret =0;

            if(data_packet->msg_len != GHT_MIDEA_APPLIANCE_SN_LEN+10)
                break;
            if((g_fibo_state_flag.g_ght_login_complete==0))
            {
                break;
            }
            memset(g_ght_midea_appliance_sn, 0, GHT_MIDEA_APPLIANCE_SN_LEN+1);
            data_in_len = data_packet->msg_len - GHT_MIDEA_PACKET_HEAD_LEN;
            //ret = ms_sn_decrypt(data_packet->data, data_in_len, g_ght_midea_appliance_sn, &data_len);
            if (0 == ret)
            {
                sn_valid_flag = 1;
                if(self_test_0x14  == 1)
                {
                    ght_midea_task_send_event(GHT_MIDEA_EVENT_NETWORK_STATUS, NULL, 0);
                }
                memcpy(g_ght_midea_appliance_sn, data_packet->data, GHT_MIDEA_APPLIANCE_SN_LEN);
                OSI_PRINTFI("Get SN %s",g_ght_midea_appliance_sn);
            }
            else
            {
                OSI_PRINTFI("GHT_MIDEA_NO_AIR_CONDITION_DEVICE_SN failed");
                break;
            }
            g_ght_midea_query_sn_count = 0xFFFF;
            g_ght_home_appliance_type = data_packet->home_elec_type;
            if(midea_sn_timer!= 0)
            {
                fibo_timer_stop(midea_sn_timer);
                fibo_timer_free(midea_sn_timer);
                midea_sn_timer= 0;
            }
            //ÂÆ∂Áîµ‰∏ãÂèëSNÂêéÂºÄÂßãÂêØÂä®ÈöèÊú∫Âª∂Êó∂Ê≥®ÁΩë
            ret = ght_attach_start();
            if(-1 == ret)
            {
                OSI_LOGI(0, "ght_attach_start failed");
            }
        }
        break;
        case GHT_MIDEA_AIR_CONDITION_INFO:
        {
            OSI_PRINTFI("packet_analysis receive:GHT_MIDEA_AIR_CONDITION_INFO, data_packet->msg_len=%d,buf_len =%d",
                data_packet->msg_len,buf_len);
            int      data_in_len=0;
            int      data_len= GHT_MIDEA_APPLIANCE_SN_LEN;
            char	 device_id[6] = {0X00,0X00,0X00,0X00,0X00,0X00};
            char     air_sn[GHT_MIDEA_APPLIANCE_SN_LEN] = {0};
            int ret =0;
            if(data_packet->msg_len <= 5+10)
                break;
            if((g_fibo_state_flag.g_ght_login_complete==0))
            {
                break;
            }
            data_in_len = data_packet->msg_len - GHT_MIDEA_PACKET_HEAD_LEN;
            // PRINT_BUF(data_packet->data,data_in_len);
            memset(g_ght_midea_appliance_sn, 0, GHT_MIDEA_APPLIANCE_SN_LEN+1);
            //ret = ms_sn_decrypt(data_packet->data, data_in_len, air_sn, &data_len);
            if (0 == ret)
            {
                sn_valid_flag = 1;
                if(self_test_0x14  == 1)
                {
                    ght_midea_task_send_event(GHT_MIDEA_EVENT_NETWORK_STATUS, NULL, 0);
                }
                memcpy(g_ght_midea_appliance_sn, air_sn,data_len);
                OSI_PRINTFI("Get SN %s",g_ght_midea_appliance_sn);
            }
            else
            {
                OSI_PRINTFI("GHT_MIDEA_AIR_CONDITION_INFO failed");
                break;
            }
            g_ght_midea_query_sn_count = 0xFFFF;
            if(midea_sn_timer!= 0)
            {
                fibo_timer_stop(midea_sn_timer);
                fibo_timer_free(midea_sn_timer);
                midea_sn_timer= 0;
            }
            g_ght_home_appliance_type = data_packet->home_elec_type;

            //ÂÆ∂Áîµ‰∏ãÂèëSNÂêéÂºÄÂßãÂêØÂä®ÈöèÊú∫Âª∂Êó∂Ê≥®ÁΩë
            ret = ght_attach_start();
            if(-1 == ret)
            {
                OSI_LOGI(0, "ght_attach_start failed");
            }

        }
        case GHT_MIDEA_WORK_SELF_TEST1:
        {
            uint8_t result = 0;
            OSI_PRINTFI("packet_analysis receive:0x14");
            p_buf = ght_midea_packet_construction_uart_data((GHT_MIDEA_PACKET_HEAD_LEN+1)
                                                            , g_ght_home_appliance_type
                                                            , 0
                                                            , g_ght_message_flag
                                                            , 0, 0
                                                            , data_packet->msg_type_flag, &result);
            fibo_hal_uart_put(port,p_buf, GHT_MIDEA_PACKET_HEAD_LEN+1+1);
            self_test_0x14 = 1;
            OSI_PRINTFI("uart send:%x",data_packet->msg_type_flag);
            g_ght_message_flag++;
            if (NULL != p_buf)
            {
                fibo_free(p_buf);
                p_buf = NULL;
            }
            ght_midea_task_send_event(GHT_MIDEA_EVENT_NETWORK_STATUS, NULL, 0);
        }
        break;
        case GHT_MIDEA_WORK_NETWOR_ACCESS_SWITCH:
        {
            OSI_PRINTFI("packet_analysis receive:GHT_MIDEA_WORK_NETWOR_ACCESS_SWITCH");

            memset(g_ght_midea_network_access_switch, 0, sizeof(g_ght_midea_network_access_switch));
            g_ght_midea_network_access_switch[4] = 0x03;
            g_ght_midea_network_access_switch[5] = 0x03;
            g_ght_midea_network_access_switch[6] = 0x00;

            p_buf = ght_midea_packet_construction_uart_data((GHT_MIDEA_PACKET_HEAD_LEN+GHT_MIDEA_WORK_NETWOR_ACCESS_SWITCH_LEN)
                                                            , g_ght_home_appliance_type
                                                            , 0
                                                            , g_ght_message_flag
                                                            , 0, 0
                                                            , data_packet->msg_type_flag, g_ght_midea_network_access_switch);
            fibo_hal_uart_put(port,p_buf, GHT_MIDEA_PACKET_HEAD_LEN+GHT_MIDEA_WORK_NETWOR_ACCESS_SWITCH_LEN+1);
            OSI_PRINTFI("uart send:%x",data_packet->msg_type_flag);
            fibo_startselftesttimer();
            g_ght_message_flag++;
            if (NULL != p_buf)
            {
                fibo_free(p_buf);
                p_buf = NULL;
            }
        }
        break;
        case GHT_MIDEA_WORK_MODEM_RESTART:
        {
            OSI_PRINTFI("packet_analysis receive:GHT_MIDEA_WORK_MODEM_RESTART");
            uint8_t modem_restart = 0;
            p_buf = ght_midea_packet_construction_uart_data((GHT_MIDEA_PACKET_HEAD_LEN+1)
                                                            , g_ght_home_appliance_type
                                                            , 0
                                                            , g_ght_message_flag
                                                            , 0, 0
                                                            , data_packet->msg_type_flag, &modem_restart);
            fibo_hal_uart_put(port,p_buf,GHT_MIDEA_PACKET_HEAD_LEN+1+1);
            OSI_PRINTFI("uart send:%x",data_packet->msg_type_flag);

            g_ght_message_flag++;
            if (NULL != p_buf)
            {
                fibo_free(p_buf);
                p_buf = NULL;
            }

            fibo_taskSleep(1000);
            fibo_softReset();
        }
        break;
        case GHT_MIDEA_WORK_FACTORY_RESET:
        {
            OSI_PRINTFI("packet_analysis receive:GHT_MIDEA_WORK_FACTORY_RESET");
            uint8_t factory_reset = 0;
            p_buf = ght_midea_packet_construction_uart_data((GHT_MIDEA_PACKET_HEAD_LEN+1)
                                                            , g_ght_home_appliance_type
                                                            , 0
                                                            , g_ght_message_flag
                                                            , 0, 0
                                                            , data_packet->msg_type_flag, &factory_reset);
            fibo_hal_uart_put(port,p_buf,GHT_MIDEA_PACKET_HEAD_LEN+1+1);
            OSI_PRINTFI("uart send:%x",data_packet->msg_type_flag);

            g_ght_message_flag++;
            if (NULL != p_buf)
            {
                fibo_free(p_buf);
                p_buf = NULL;
            }
            fibo_taskSleep(1000);
            fibo_softReset();
        }
        break;
        default:
        OSI_PRINTFI("packet_analysis receive:default");
        break;
    }

    return 0;
}
static void ght_midea_task_event_process(struct ght_midea_func_event *event)
{
    int    server_fd = 0;
    uint8_t *p_buf = NULL;
    int port =0;

    switch(event->event_id)
    {
        /*<<NB-IoTƒ£øÈ¥Æø⁄Õ®—∂–≠“È 1.0.1>>µ⁄3.4Ω⁄ begin*/
        case GHT_MIDEA_EVENT_GENERAL_SN_REQ:
        {
            OSI_PRINTFI("ght_midea_task_event_process:GHT_MIDEA_EVENT_GENERAL_SN_REQ");
            uint8_t ch_sn[GHT_MIDEA_APPLIANCE_GENERAL_SN_REQ_LEN] = {0};
            if (0xFFFF == g_ght_midea_query_sn_count)
            {
                break;
            }
            p_buf = ght_midea_packet_construction_uart_data(GHT_MIDEA_PACKET_HEAD_LEN+GHT_MIDEA_APPLIANCE_GENERAL_SN_REQ_LEN
                                                            , g_ght_home_appliance_type
                                                            , 0
                                                            , g_ght_message_flag
                                                            , 0, 0
                                                            , GHT_MIDEA_NO_AIR_CONDITION_DEVICE_SN, ch_sn);
            fibo_hal_uart_put(port,p_buf,GHT_MIDEA_PACKET_HEAD_LEN+GHT_MIDEA_APPLIANCE_GENERAL_SN_REQ_LEN+1);
            OSI_PRINTFI("ght_midea_task_event_process event_id:%x",event->event_id);
            g_ght_message_flag++;
            if (NULL != p_buf)
            {
                fibo_free(p_buf);
                p_buf = NULL;
            }
            g_ght_midea_query_sn_count++;
            break;
        }
        case GHT_MIDEA_EVENT_AIR_SN_REQ:
        {
            OSI_PRINTFI("ght_midea_task_event_process:GHT_MIDEA_EVENT_AIR_SN_REQ");
            uint8_t sn[GHT_MIDEA_APPLIANCE_AIR_SN_REQ_LEN] = {0};
            if (0xFFFF == g_ght_midea_query_sn_count)
            {
                break;
            }
            p_buf = ght_midea_packet_construction_uart_data(GHT_MIDEA_PACKET_HEAD_LEN+GHT_MIDEA_APPLIANCE_AIR_SN_REQ_LEN
	                                                        , g_ght_home_appliance_type
	                                                        , 0
	                                                        , g_ght_message_flag
	                                                        , 0, 0
	                                                        , GHT_MIDEA_AIR_CONDITION_INFO, sn);
            fibo_hal_uart_put(port,p_buf,GHT_MIDEA_PACKET_HEAD_LEN+GHT_MIDEA_APPLIANCE_AIR_SN_REQ_LEN+1);
            OSI_PRINTFI("ght_midea_task_event_process event_id:%x",event->event_id);
            g_ght_message_flag++;
            if (NULL != p_buf)
            {
                fibo_free(p_buf);
                p_buf = NULL;
            }
            g_ght_midea_query_sn_count++;
            break;
        }
        case GHT_MIDEA_EVENT_TYPE_AND_INFO_REQ:
        {
            OSI_PRINTFI("ght_midea_task_event_process:GHT_MIDEA_EVENT_TYPE_AND_INFO_REQ");
            uint8_t type_and_info[GHT_MIDEA_APPLIANCE_TYPE_AND_INFO_REQ_LEN] = {0};
            p_buf = ght_midea_packet_construction_uart_data(GHT_MIDEA_PACKET_HEAD_LEN+GHT_MIDEA_APPLIANCE_TYPE_AND_INFO_REQ_LEN
                                                            , g_ght_home_appliance_type
                                                            , 0
                                                            , g_ght_message_flag
                                                            , 0, 0
                                                            , GHT_MIDEA_APPLIANCE_TYPE_AND_INFO, type_and_info);
            fibo_hal_uart_put(port,p_buf,GHT_MIDEA_PACKET_HEAD_LEN+GHT_MIDEA_APPLIANCE_TYPE_AND_INFO_REQ_LEN+1);
            OSI_PRINTFI("ght_midea_task_event_process event_id:%x",event->event_id);
            g_ght_message_flag++;
            if (NULL != p_buf)
            {
                fibo_free(p_buf);
                p_buf = NULL;
            }
            break;
        }
        /*<<NB-IoTƒ£øÈ¥Æø⁄Õ®—∂–≠“È 1.0.1>>µ⁄3.6Ω⁄ end*/
        case GHT_MIDEA_EVENT_UART_DATA_IND:
        {
            struct ght_midea_data_packet data_packet;
            OSI_PRINTFI("ght_midea_task_event_process:GHT_MIDEA_EVENT_UART_DATA_IND,len =%d",event->data_len);
            //PRINT_BUF(event->data, event->data_len);
            memset(&data_packet, 0, sizeof(struct ght_midea_data_packet));
            if (0 != ght_midea_packet_analysis_uart_data((char*)event->data, event->data_len, &data_packet))
            {
                OSI_PRINTFI("GHT_MIDEA_EVENT_UART_packet_analysis_uart_data_failed!");
                break;
            }
            OSI_PRINTFI("uart recv:%x",data_packet.msg_type_flag);
            ght_midea_packet_analysis_message_type(server_fd
                                                    , &data_packet
                                                    , (char*)event->data
                                                    , event->data_len);

            if(event != NULL)
            {
                if(event->data != NULL)
                {
                    fibo_free(event->data);
                    event->data = NULL;
                }
                fibo_free(event);
                event = NULL;
            }
            if (NULL != data_packet.data)
            {
                fibo_free(data_packet.data);
                data_packet.data = NULL;
            }
            break;
        }
        default:
        break;
    }
    if(event != NULL)
    {
        if(event->data != NULL)
        {
            fibo_free(event->data);
            event->data = NULL;
        }
        fibo_free(event);
        event = NULL;
    }
}
void midea_uart_init()
{
    hal_uart_config_t drvcfg = {0};
    drvcfg.baud = 9600;
    drvcfg.parity = HAL_UART_NO_PARITY;
    drvcfg.data_bits = HAL_UART_DATA_BITS_8;
    drvcfg.stop_bits = HAL_UART_STOP_BITS_1;
    drvcfg.rx_buf_size = UART_RX_BUF_SIZE;
    drvcfg.tx_buf_size = UART_TX_BUF_SIZE;

    int port = 0;
    fibo_hal_uart_init(port, &drvcfg, ght_midea_uart_recv_data, NULL);
}

static void ght_midea_task_thread(void)
{
    int ret = -1;
    struct ght_midea_func_event *p_event = NULL;
    midea_uart_init();
    ght_midea_func_init();
    while (1)
    {
        if(g_fibo_state_flag.getSnFlag==0||g_fibo_state_flag.is_waitip ==1)
        {
            p_event = ght_midea_task_event_get();
            if (!p_event) 
            {
                fibo_taskSleep(5);  //50
                continue;
            }
            OSI_PRINTFI("get event id=%d   data_len=%d 000aa",p_event->event_id,p_event->data_len);
            ght_midea_task_event_process(p_event);
            fibo_taskSleep(5);   //50
        }
        else
            fibo_taskSleep(25);  // 100
    }
}

void ght_midea_uart_recv_data(hal_uart_port_t uart_port,UINT8 *buf, UINT16 data_len,void *arg)
{
    uint16_t recv_data_len = 0;

    OSI_PRINTFI("ght_midea_uart_recv_data=%x:%x:%x:%x    data_len=%d",buf[0],buf[1],buf[2],buf[3],data_len);

    if (0xFF == buf[0] ||0xff == buf[0])
    {
        int port =0;
        OSI_PRINTFI("###ready into at cmd mode###");
        fibo_hal_uart_deinit(port);
    }
    if((0x61 == buf[0] &&0x74 == buf[1])||(0x41 == buf[0] &&0x54 == buf[1]))
    {
        int port =0;
        OSI_PRINTFI("###[recv at/AT]ready into at cmd mode###");
        fibo_hal_uart_deinit(port);
    }
    if (0xAA == buf[0])
    {
        memset(&g_ght_midea_uart_data, 0, sizeof(struct ght_midea_uart_data));
        if (buf[1] < GHT_MIDEA_PACKET_HEAD_LEN)
        {
            OSI_PRINTFI("packet head length error");
            return;
        }
        g_ght_midea_uart_data.all_data_len = buf[1] + 1;
        recv_data_len = (data_len < (GHT_MIDEA_UART_DATA_MAX_LEN-1))? data_len : (GHT_MIDEA_UART_DATA_MAX_LEN-1);
        memcpy(g_ght_midea_uart_data.data_buf, buf, recv_data_len);
        g_ght_midea_uart_data.recv_data_len += recv_data_len;
    }
    else if (g_ght_midea_uart_data.recv_data_len < g_ght_midea_uart_data.all_data_len)
    {
        if (g_ght_midea_uart_data.recv_data_len >= (GHT_MIDEA_UART_DATA_MAX_LEN-1))
        {
            recv_data_len = 0;
        }
        else
        {
            recv_data_len = 
            ((g_ght_midea_uart_data.recv_data_len + data_len) > (GHT_MIDEA_UART_DATA_MAX_LEN-1))? 
            (GHT_MIDEA_UART_DATA_MAX_LEN-1-g_ght_midea_uart_data.recv_data_len): data_len;
        }
        memcpy((g_ght_midea_uart_data.data_buf+g_ght_midea_uart_data.recv_data_len), buf, recv_data_len);
        g_ght_midea_uart_data.recv_data_len += recv_data_len;
    }

    if (g_ght_midea_uart_data.recv_data_len >= g_ght_midea_uart_data.all_data_len
    && g_ght_midea_uart_data.all_data_len > 0)
    {
        char* data_buf = fibo_malloc(g_ght_midea_uart_data.all_data_len+1);
        memcpy(data_buf, g_ght_midea_uart_data.data_buf, g_ght_midea_uart_data.all_data_len);
        ght_midea_task_send_event(GHT_MIDEA_EVENT_UART_DATA_IND, data_buf, g_ght_midea_uart_data.all_data_len);
        OSI_PRINTFI("send event:GHT_MIDEA_EVENT_UART_DATA_IND   g_ght_midea_uart_data.all_data_len=%d",
        g_ght_midea_uart_data.all_data_len);
        memset(&g_ght_midea_uart_data, 0, sizeof(struct ght_midea_uart_data));
        return;
    }
    else if (g_ght_midea_uart_data.recv_data_len)
    {
        return;
    }
    return;
}
void* appimg_enter(void *param)
{
    int ret = -1;

    OSI_LOGI(0, "application image enter, param 0x%x", param);
    fibo_taskSleep(2*1000);//wait for inition of CFW interface„ÄÅSIM
    ret = fibo_cfun_zero();
    OSI_LOGI(0, "fibo_cfun_zero result: %d", ret);
    
    fibo_thread_create(ght_midea_task_thread, "MIDEA_MAIN_TASK", 1024*8, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
