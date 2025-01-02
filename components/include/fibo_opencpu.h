
#ifndef _FIBO_OPENCPU_H_
#define _FIBO_OPENCPU_H_

#include <stdbool.h>

#include "osi_api.h"
#include "osi_log.h"
#include "cfw.h"
#include "at_cfg.h"
#include "lwip/netdb.h"

#include "at_cfw.h"

#include "lwip/ip_addr.h"
#include "lwip/sockets.h"
#include "ppp_interface.h"
#ifndef CONFIG_FIBOCOM_MAX_APP
#include "oc_mqtt.h"
#endif
#include "oc_uart.h"
#include "oc_adc.h"
#include "fibo_opencpu_comm.h"
#include "cfw_dispatch.h"
#include "drv_lcd.h"
#include "image_sensor.h"
#include "fibo_common.h"
#include "audio_types.h"
#include "drv_charger.h"
#include "drv_spi.h"
#include "bluetooth.h"
#include "mbedtls/aes.h"
#include "mbedtls/des.h"
#include "mbedtls/rsa.h"
#include "mbedtls/md5.h"
//#include "att.h"
//#include "gatt.h"
#include "expat.h"
#include "oc_ccid.h"
#include "oc_mbedtls.h"
#include "oc_ble.h"
#include "drv_tp.h"
#include "drv_mipi_lcd.h"
//#include "task.h"

/* FOTA ERROR CODE */
#define OTA_ERRCODE_MEMLEAK				(-1)
#define OTA_ERRCODE_FSREAD				(-2)
#define OTA_ERRCODE_FSWRITE				(-3)
#define OTA_ERRCODE_APP_CHECKFAIL		(-4)	
#define OTA_ERRCODE_FM_CHECKFAIL		(-5)
#define OTA_ERRCODE_APPFOTA_CHECKFAIL	(-6)
#if defined(CONFIG_FIBOCOM_LIANDI) && defined(CONFIG_FIBOCOM_OPENCPU)
#define OTA_ERRCODE_DOWNLOAD_FAIL		(-7)
#endif



/*ERROR CODE*/
#define GAPP_RET_OK (0)
#define GAPP_RET_PARAMS_ERROR (-1)
#define GAPP_RET_NOT_INIT (-2)
#define GAPP_RET_MEMORY_ERROR (-3)
#define GAPP_RET_OPTION_NOT_SUPPORT (-4)
#define GAPP_RET_TIMEOUT (-5)
#define GAPP_RET_UNKNOW_ERROR (-6)
#define GAPP_RET_FILE_ALREADY_OPEN_ERROR (-7)
#define GAPP_RET_NOT_ENOUGH_SPACE_ERROR (-8)
#define GAPP_RET_NOT_VALID_STATE_ERROR (-9)


#define FS_O_RDONLY  O_RDONLY
#define FS_O_WRONLY  O_WRONLY 
#define FS_O_RDWR    O_RDWR
#define FS_O_APPEND  O_APPEND
#define FS_O_CREAT   O_CREAT
#define FS_O_TRUNC   O_TRUNC
#define FS_O_EXCL    O_EXCL
#define FS_O_SYNC    O_SYNC

#define FS_SEEK_SET  SEEK_SET
#define FS_SEEK_CUR  SEEK_CUR
#define FS_SEEK_END  SEEK_END


// TTS
#ifndef CTTS_STRING_ENCODE_TYPE_UTF8
#define CTTS_STRING_ENCODE_TYPE_UTF8 (0)
#endif
#ifndef CTTS_STRING_ENCODE_TYPE_GB2312
#define CTTS_STRING_ENCODE_TYPE_GB2312 (1)
#endif
#ifndef CTTS_STRING_ENCODE_TYPE_UNICODE
#define CTTS_STRING_ENCODE_TYPE_UNICODE (2)
#endif
#ifndef CTTS_TEXT_MAX_LEN
#define CTTS_TEXT_MAX_LEN (1024)
#endif
#define TTS_SPEED_MIN -32768 /* slowest voice speed */
#define TTS_SPEED_NORMAL 0   /* normal voice speed (default) */
#define TTS_SPEED_MAX +32767 /* fastest voice speed */

#define TTS_PITCH_MIN -32768 /* lowest voice tone */
#define TTS_PITCH_NORMAL 0   /* normal voice tone (default) */
#define TTS_PITCH_MAX +32767 /* highest voice tone */

typedef enum {
    GAPP_TTS_SUCCESS = 0,
    GAPP_TTS_ERROR = 1,
    GAPP_TTS_PARAMS_ERROR = -1,
    GAPP_TTS_BUSY = -2,
    GAPP_TTS_MEMORY_ERROR = -3
}tts_status_e;

typedef void (*OpencpuAsyncCtxDestroy_t)(void *cmd);
typedef void (*auPlayerCallback_t)(void);

typedef struct asyn_opencpu_event
{
    INT8 nSim;
	UINT8 cid;
	UINT8 state;
    INT16 uti;
    UINT8 ip[THE_APN_MAX_LEN];
    void *async_ctx;
    OpencpuAsyncCtxDestroy_t async_ctx_destroy;
} asyn_opencpu_event_t;

typedef struct pdp_profile
{
    INT8 active_state;
    INT8 cid;
    UINT8 nPdpType[THE_APN_MAX_LEN];
    UINT8 apn[THE_APN_MAX_LEN];
    INT8 d_comp;
    INT8 h_comp;
    INT8 pdpDns[THE_PDP_DNS_MAX_LEN * THE_PDP_DNS_NUM];
    UINT8 pdpAddrSize;
    UINT8 apnUserSize;
    UINT8 apnPwdSize;
    UINT8 apnUser[THE_APN_USER_MAX_LEN];
    UINT8 apnPwd[THE_APN_PWD_MAX_LEN];
    UINT8 pdpAddr[THE_PDP_ADDR_MAX_LEN];
    UINT8 nAuthProt;
} pdp_profile_t;

typedef struct
{
    INT8 value; ///< integer value
    char *str;  ///< string value
} opencpuValueStrMap_t;

typedef struct
{
    UINT16 sin_port; //save as network sequence
    struct ip_addr sin_addr;
} GAPP_TCPIP_ADDR_T;

typedef struct
{
    uint8_t act_state;
    uint8_t cid_count;
    uint8_t cid_index;
    uint8_t cids[AT_PDPCID_MAX];
} cgactAsyncCtx_t;

typedef struct
{
    UINT8 test;
    UINT32 case_number;
} test_demo_t;

#define GAPP_IPPROTO_TCP 0
#define GAPP_IPPROTO_UDP 1
#define GAPP_IPPROTO_SEC 2

typedef struct
{
    uint8_t sec;   ///< Second
    uint8_t min;   ///< Minute
    uint8_t hour;  ///< Hour
    uint8_t day;   ///< Day
    uint8_t month; ///< Month
    uint16_t year;  ///< Year
    uint8_t wDay;  ///< Week Day
} hal_rtc_time_t;
typedef struct
{
    char text[CONFIG_ATS_ALARM_TEXT_LEN + 1];
    uint8_t has_timezone;
    int8_t timezone;
    uint8_t silent;
} atAlarmInfo_t;
typedef enum
{
    SAPP_RTC_INT_DISABLED,
    SAPP_RTC_INT_PER_SEC,
    SAPP_RTC_INT_PER_MIN,
    SAPP_RTC_INT_PER_HOUR,
} SAPP_RTC_MODE_T;
typedef enum
{
    AUDIO_VOICE_VOLUME,
    AUDIO_PLAY_VOLUME,
    AUDIO_TONE_VOLUME,
} AUDIO_VOLUME_MODE_T;

typedef enum
{
    AEE_TONE_RING_G5 = 1568,/*1568.0Hz*/
    AEE_TONE_RING_A5 = 880,/*880.0Hz*/
    AEE_TONE_RING_E5 = 1318,/*1318.5Hz*/
} AUDIO_TONE_EX_T;

typedef enum
{
    BOOTCAUSE_BY_SOFT,
    BOOTCAUSE_BY_PIN_RST,
    BOOTCAUSE_BY_PIN_PWR,
    BOOTCAUSE_BY_CHARGE,
    BOOTCAUSE_BY_WDG,
} POWER_ON_CAUSE;


typedef struct{
	CFW_TSM_CURR_CELL_INFO pCurrCellInfo;
    CFW_TSM_ALL_NEBCELL_INFO pNeighborCellInfo;
}gsm_cellifo_t;


typedef struct{
	uint8_t curr_rat;
	cfw_nwCapLte_t lte_cellinfo;
	gsm_cellifo_t  gsm_cellinfo;
}opencpu_cellinfo_t;


typedef struct
{
	uint32_t tac;
	uint32_t cell_id;
}lte_scell_info_t;

typedef struct
{
	uint32_t lac;
	uint32_t cell_id;
}gsm_scell_info_t;



typedef struct
{
    uint8_t curr_rat;
	uint8_t nStatus;
	lte_scell_info_t lte_scell_info;
	gsm_scell_info_t gsm_scell_info;
	
}reg_info_t;
typedef int32_t      cis_evt_t;
typedef struct
{
    cis_evt_t eid;
    void *param;
    
}onenet_param_t;

/* OpenCPU HTTP API */
#ifdef CONFIG_L610_CN_62_80_JIALIAN
#define OC_HTTP_URL_LEN 256
#else
#define OC_HTTP_URL_LEN 256
#endif
#define OC_HTTP_FILE_PATH_LEN 64

typedef void (*http_read_body_callback)(void *pstHttpParam, INT8 *data, INT32 len);
typedef void (*http_read_header_callback)(const UINT8 *name, const UINT8* value);

typedef enum {
    OC_USER_CALLBACK = 0,
    OC_SAVE_FILE = 1
}oc_http_read_method_e;

typedef struct _ocHttpParam
{
    uint8_t                 url[OC_HTTP_URL_LEN + 1];               /* Http URL */
    uint32_t                timeout;                                /* Request timeout */
    uint8_t                 filepath[OC_HTTP_FILE_PATH_LEN + 1];    /* Http read filepath */
    bool                    bIsIpv6;                                 /* Http is Ipv6, default is Ipv4 */
    oc_http_read_method_e   enHttpReadMethod;                       /* Http is use userdef callback */
    int32_t                 respContentLen;                         /* Http response body len */
    int32_t                 respCode;                               /* Http response code */
    uint8_t                 *respVersion;                           /* Http response version */
    uint8_t                 *respReasonPhrase;                      /* Http response reason phrase */
    void                    *pHttpInfo;                             /* Http info */
    http_read_body_callback cbReadBody;                             /* Http response callback */
}oc_http_param_t;


#define ATC_MAX_BT_NAME_STRING_LEN 164
#define ATC_MAX_BT_ADDRESS_STRING_LEN 20
#define ATC_BT_NAME_ASCII 0           //the bt name encode in ascii
#define ATC_BT_NAME_UCS2 1            //the bt name encode in ucs2
#define ATC_BT_TX_TIMEOUT (30 * 1000) //input timeout
#define FIBO_BT_DEVICE_NUM 30

typedef struct _FIBO_BT_DEVICE_INFO
{
    char name[ATC_MAX_BT_NAME_STRING_LEN];
    char addr[ATC_MAX_BT_ADDRESS_STRING_LEN];
    uint32_t dev_class;
    uint32_t rssi;
} FIBO_BT_DEVICE_INFO;

extern int32_t fibo_play_tone_ex(AUDIO_TONE_EX_T freq);

typedef struct FIBO_BT_DEVICE{
	FIBO_BT_DEVICE_INFO bt_device[FIBO_BT_DEVICE_NUM];
}FIBO_BT_DEVICE_T;

typedef struct _ble_whitelist{
	uint8_t type;
	char addr[ATC_MAX_BT_ADDRESS_STRING_LEN];
}ble_whitelist;

typedef struct fibo_ble_whilelist{
	uint8_t whitelist_num;
	ble_whitelist stWhiteList[FIBO_BT_DEVICE_NUM];
}fibo_ble_whilelist_t;



extern atCfwCtx_t gAtCfwCtx;
typedef void (*key_callback)(void* param);
typedef void (*usb_input_callback_t)(atDevice_t *th, void *buf, size_t size, void *arg);
typedef int (*Report_UsbDev_Event) (int msg);

#define MAX_AP_NAME_LEN 99
#define MAX_AP_ADDR_LEN 20
#define MAX_AP_NUM 30

typedef struct fibo_wifiscan_info{
	int8_t  ap_rssi;
	uint8_t ap_name[MAX_AP_NAME_LEN];
	uint8_t ap_addr[MAX_AP_ADDR_LEN];
}fibo_wifiscan_ap_info_t;

typedef struct fibo_wifiscan_res{
	int8_t  ap_num;
	fibo_wifiscan_ap_info_t fibo_wifiscan_ap_info[MAX_AP_NUM];
}fibo_wifiscan_res_t;

typedef struct
{
	uint8_t result;
	uint32_t time;
}fibo_ntp_rsp;

#if defined (CONFIG_FIBOCOM_LIANDI) && defined (CONFIG_FIBOCOM_OPENCPU)
typedef  struct  CaseInfo
{
    uint32_t  boot_cause;
    uint32_t  swrst_ctrl;
#if defined(CONFIG_LG610_CN_10_92_LIANDI_ZFB) || defined(CONFIG_LG610_CN_10_92_2048_MIPI)
    const void *export_api;
#endif
}CaseInfo;

extern bool fibo_GetBootCause(CaseInfo  *pInfo);
#endif
/* function defination */

/* OSI function */
extern void fibo_taskSleep(UINT32 msec);
extern INT32 fibo_thread_create(void *pvTaskCode, INT8 *pcName, UINT32 usStackDepth, void *pvParameters, UINT32 uxPriority);
extern INT32 fibo_thread_create_ex(void *pvTaskCode, INT8 *pcName, 
								UINT32 usStackDepth, void *pvParameters, 
								UINT32 uxPriority, UINT32* pThreadId);
extern void fibo_thread_delete(void);
extern void fibo_thread_suspend(UINT32 uThread);
extern void fibo_thread_resume(UINT32 uThread);
//delete specify thread
extern void fibo_specify_thread_delete(UINT32 uThread);
extern UINT32 fibo_thread_id(void);
extern UINT32 fibo_get_thread_stack_remaining_size(osiThread_t *thread);

extern UINT32 fibo_getSysTick(void);
extern void *fibo_malloc(UINT32 size);
extern INT32 fibo_free(void *buffer);
extern INT32 fibo_softReset(void);
extern INT32 fibo_softPowerOff(void);
extern UINT32 fibo_sem_new(UINT8 inivalue);
extern void fibo_sem_free(UINT32 semid);
extern void fibo_sem_wait(UINT32 semid);
extern bool fibo_sem_try_wait(UINT32 semid, UINT32 timeout);
extern void fibo_sem_signal(UINT32 semid);
extern UINT32 fibo_enterCS(void);
extern void fibo_exitCS(UINT32 status);
extern INT32 fibo_setRTC(hal_rtc_time_t *time);
extern INT32 fibo_getRTC(hal_rtc_time_t *time);
extern UINT32 fibo_queue_create(UINT32 msg_count, UINT32 msg_size);
extern INT32 fibo_queue_put(UINT32 msg_id, const void *msg, UINT32 timeout);
extern INT32 fibo_queue_put_to_front(UINT32 msg_id, const void *msg, UINT32 timeout);
extern INT32 fibo_queue_get(UINT32 msg_id, void *msg, UINT32 timeout);
extern INT32 fibo_queue_put_isr(UINT32 msg_id, const void *msg);
extern INT32 fibo_queue_put_to_front_isr(UINT32 msg_id, const void *msg);
extern INT32 fibo_queue_get_isr(UINT32 msg_id, void *msg);
extern void fibo_queue_delete(UINT32 msg_id);
extern UINT32 fibo_queue_space_available(UINT32 msg_id);
extern void fibo_queue_reset(UINT32 msg_id);

extern UINT32 fibo_mutex_create(void);
extern void fibo_mutex_lock(UINT32 mutex_id);
extern INT32 fibo_mutex_try_lock(UINT32 mutex_id, UINT32 timeout);
extern void fibo_mutex_unlock(UINT32 mutex_id);
extern void fibo_mutex_delete(UINT32 mutex_id);
extern INT32 fibo_rng_generate(void *buf, UINT32 len);

//extern INT32 fibo_setRTC_timerMode(SAPP_RTC_MODE_T mode);
//extern INT32 fibo_setRTC_timerSwitch(INT32 on);
//extern INT32 fibo_setRTC_timerCB(void (*cb)(void *), void *user_data);

/* timer function */
extern UINT32 fibo_timer_create(void (*fn)(void *arg), void *arg);
extern bool fibo_timer_start(UINT32 timer_id, UINT32 ms);
extern bool fibo_timer_period_start(UINT32 timer_id, UINT32 ms);
extern UINT32 fibo_rtc_irq_create(void (*fn)(void *arg), void *arg);
extern bool fibo_rtc_irq_start(UINT32 timer_id, UINT32 us);
extern bool fibo_rtc_irq_period_start(UINT32 timer_id, UINT32 ms);
extern UINT32 fibo_timer_new(UINT32 ms, void (*fn)(void *arg), void *arg);
extern UINT32 fibo_timer_period_new(UINT32 ms, void (*fn)(void *arg), void *arg);
extern INT32 fibo_timer_free(UINT32 timerid);
extern bool fibo_timer_stop(UINT32 timerid);
extern UINT32 fibo_setRtc_Irq(UINT32 us,void(* fn)(void * arg),void * arg);
extern UINT32 fibo_setRtc_Irq_period(UINT32 ms, void (*fn)(void *arg), void *arg);

/*pdp & sockets*/
extern INT32 fibo_PDPActive(UINT8 active, UINT8 *apn, UINT8 *username, UINT8 *password, UINT8 nAuthProt, CFW_SIM_ID nSimID, UINT8 *ip);
extern INT32 fibo_asyn_PDPActive(UINT8 active, pdp_profile_t *pdp_profile, CFW_SIM_ID nSimID);
extern INT32 fibo_PDPRelease(UINT8 deactive, CFW_SIM_ID nSimID);
extern INT32 fibo_asyn_PDPRelease(UINT8 deactive, INT32 cid, CFW_SIM_ID nSimID);
extern INT32 fibo_PDPStatus(INT8 cid, UINT8 *ip, UINT8 *cid_status,CFW_SIM_ID nSimID);
extern INT32 fibo_sock_create(INT32 nProtocol);
extern INT32 fibo_sock_create_ex(int domain, int type, int protocol);
extern INT32 fibo_get_socket_error(void);
extern INT32 fibo_sock_available_buffer(INT32 sock);

extern INT32 fibo_sock_close(INT32 sock);
extern INT32 fibo_sock_connect(INT32 sock, GAPP_TCPIP_ADDR_T *addr);
extern INT32 fibo_sock_listen(INT32 sock);
extern INT32 fibo_sock_bind(INT32 sock, GAPP_TCPIP_ADDR_T *addr);
extern INT32 fibo_sock_accept(INT32 sock, GAPP_TCPIP_ADDR_T *addr);
extern INT32 fibo_sock_send(INT32 sock, UINT8 *buff, UINT16 len);
extern INT32 fibo_sock_send2(INT32 sock, UINT8 *buff, UINT16 len, GAPP_TCPIP_ADDR_T *dest);
extern INT32 fibo_sock_recv(INT32 sock, UINT8 *buff, UINT16 len);
extern INT32 fibo_sock_recvFrom(INT32 sock, UINT8 *buff, UINT16 len, GAPP_TCPIP_ADDR_T *src);
extern INT32 fibo_getHostByName(char *hostname,ip_addr_t *addr,uint8_t nCid, CFW_SIM_ID nSimID);
extern INT32 fibo_sock_setOpt(INT32 sock, INT32 level, INT32 optname, const void *optval, INT32 optlen);
extern INT32 fibo_sock_getOpt(INT32 sock, INT32 level, INT32 optname, void *optval, INT32 *optlen);
extern INT32  fibo_sock_lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset,struct timeval *timeout);
extern INT32 fibo_multi_pdn_cfg(uint8_t protocol_id, uint8_t cid, int32_t reserved);
extern INT32 fibo_get_ip_info_by_cid(uint8_t cid, uint8_t *ip_type, struct sockaddr_in *ip_addr, struct sockaddr_in6 *ip6_addr);

extern INT32 fibo_get_global_DNS(char *pri_dns, char *sec_dns);
extern INT32 fibo_set_global_DNS(const char *pri_dns, const char *sec_dns);

extern INT32 fibo_getCellInfo(opencpu_cellinfo_t *opencpu_cellinfo_ptr,CFW_SIM_ID nSimID);
extern INT32 fibo_getCellInfo2();
extern INT32 fibo_getRegInfo(reg_info_t *reg_info,CFW_SIM_ID nSimID);
extern INT32 fibo_vsnprintf(INT8 *buff, UINT32 n, const INT8 *fmt, va_list arg);

//lbs
extern INT32 fibo_lbs_set_key(char *key);
extern INT32 fibo_lbs_get_gisinfo(UINT8 type);


//uart
extern INT32 fibo_hal_uart_init(hal_uart_port_t uart_port, hal_uart_config_t *uart_config, uart_input_callback_t recv_cb, void *arg);
extern INT32 fibo_hal_uart_put(hal_uart_port_t uart_port, UINT8 *buff, UINT32 len);
//extern INT32 fibo_hal_uart_data_to_send(int uart_port);
extern INT32 fibo_hal_uart_deinit(hal_uart_port_t uart_port);

//adc
extern INT32 fibo_hal_adc_init(void);
extern INT32 fibo_hal_adc_deinit(void);
extern INT32 fibo_hal_adc_get_data_polling(hal_adc_channel_t channel, UINT32 *data);

//mqtt
extern INT32 fibo_mqtt_set(INT8 *usr, INT8 *pwd);
extern INT32 fibo_mqtt_will(INT8 *topic, UINT8 qos, BOOL retain, INT8 *message);
extern INT32 fibo_mqtt_connect(INT8 *ClientID, INT8 *DestAddr, UINT16 DestPort, UINT8 CLeanSession, UINT16 keepalive, UINT8 UseTls);
extern INT32 fibo_mqtt_pub(INT8 *topic, UINT8 qos, BOOL retain, INT8 *message, UINT16 msglen);
extern INT32 fibo_mqtt_sub(INT8 *topic, UINT8 qos);
extern INT32 fibo_mqtt_unsub(INT8 *topic);
extern INT32 fibo_mqtt_close(void);
extern UINT32 fibi_mqtt_set_psk(const INT8 *psk_id, const INT8 *psk_key);
extern UINT32 fibi_mqtt_set_tls_ver(INT32 ver);

extern INT32 fibo_mqtt_set1(UINT8 id, INT8 *usr, INT8 *pwd);
extern INT32 fibo_mqtt_will1(UINT8 id, INT8 *topic, UINT8 qos, BOOL retain, INT8 *message);
extern INT32 fibo_mqtt_connect1(UINT8 id, INT8 *ClientID, INT8 *DestAddr, UINT16 DestPort, UINT8 CLeanSession, UINT16 keepalive, UINT8 UseTls);
extern INT32 fibo_mqtt_pub1(UINT8 id, INT8 *topic, UINT8 qos, BOOL retain, INT8 *message, UINT16 msglen);
extern INT32 fibo_mqtt_sub1(UINT8 id, INT8 *topic, UINT8 qos);
extern INT32 fibo_mqtt_unsub1(UINT8 id, INT8 *topic);
extern INT32 fibo_mqtt_close1(UINT8 id);

//other
extern INT32 fibo_watchdog_enable(UINT32 sec);
extern INT32 fibo_watchdog_disable(void);
extern INT32 fibo_watchdog_feed(void);
extern INT32 fibo_sleep_lock_once();
extern INT32 fibo_sleep_unlock_once();
extern INT32 fibo_cfun_one();
extern INT32 fibo_cfun_zero();
extern INT32 fibo_get_ccid(uint8_t *ccid);
extern INT32 fibo_get_sim_status(uint8_t *pucSimStatus);
//extern INT32 fibo_set_mcal_dev(UINT8 ucDevValue);
//extern INT32 fibo_set_mcal(UINT8 ucBandIndex);
extern INT32 fibo_disable_irq_start(void);
extern INT32 fibo_disable_irq_end(uint32_t irq_status);
extern INT8 fibo_get_cgmm(UINT8 *p_cgmm_info, UINT8 cgmm_info_size);

/*at*/
INT32 fibo_at_send(const UINT8 *cmd, UINT16 len);

extern INT32 fibo_textTrace(INT8 *fmt, ...);
extern INT32 fibo_get_imsi(UINT8 *imsi);
extern INT32 fibo_get_imsi_asyn(void (*cb)(UINT8 *imsi));
extern INT32 fibo_get_csq(INT32* rssi, INT32* ber);
extern INT32  fibo_rtc_start_timer(UINT32* sleep_rtc_handle,UINT32 time_out_100ms,bool  is_periodic,void (*callback)(void* ));
extern INT32  fibo_rtc_delete_timer(UINT32 sleep_rtc_handle);

extern INT32 fibo_gpio_cfg(SAPP_IO_ID_T id,SAPP_GPIO_CFG_T cfg);
extern INT32 fibo_gpio_get(SAPP_IO_ID_T id,UINT8 * level);
extern INT32 fibo_gpio_mode_set(SAPP_IO_ID_T id,UINT8 mode);
extern INT32 fibo_gpio_pull_disable(SAPP_IO_ID_T id);
extern INT32 fibo_gpio_set(SAPP_IO_ID_T id,UINT8 level);
extern INT32 fibo_gpio_isr_deinit(SAPP_IO_ID_T id);
extern INT32 fibo_gpio_isr_init(SAPP_IO_ID_T id,oc_isr_t * isr_cfg);
extern INT32 fibo_gpio_pull_up_or_down(SAPP_IO_ID_T id,bool is_pull_up);
extern INT32 fibo_lpg_switch(UINT8 ucLpgSwitch);
extern INT32 fibo_setSleepMode(UINT8 time);
extern INT32 fibo_keyout_setLevel(SAPP_IO_ID_T id, UINT8 level);



extern INT32 fibo_file_open(const INT8 * pathname,UINT32 opt);
extern INT32 fibo_file_close(INT32 fd);
extern INT32 fibo_file_read(INT32 fd,UINT8 * buff,UINT32 size);
extern INT32 fibo_file_write(INT32 fd,UINT8 * buff,UINT32 size);
extern INT32 fibo_file_seek(INT32 fd,INT32 offset,UINT8 opt);
extern INT32 fibo_file_rename(const INT8 * lpOldName,const INT8 * lpNewName);
extern INT32 fibo_file_rmdir(const char * name);
extern INT32 fibo_file_mkdir(const char * name);
extern INT32 fibo_file_exist(const INT8 * pszFileName);
extern INT32 fibo_file_getSize(const INT8 * pszFileName);
extern INT32 fibo_file_delete(const INT8 * pathName);
extern INT32 fibo_get_heapinfo(uint32_t * size,uint32_t * avail_size,uint32_t * max_block_size);
extern INT32 fibo_file_getFreeSize(void);
extern INT32 fibo_ffsmountExtflash(UINT32 uladdr_start, UINT32 ulsize, char * dir, UINT8 spi_pin_sel, bool format_on_fail, bool force_format);
extern INT32 fibo_file_stat(const char *path, struct stat *st);
extern INT32 fibo_file_closedir(DIR *pdir);
extern INT32 fibo_file_fsync(int fd);
extern INT32 fibo_file_unlink(const char *path);
extern DIR *fibo_file_opendir(const char *name);
extern struct dirent *fibo_file_readdir(DIR *pdir);
extern INT32 fibo_file_telldir(DIR *pdir);
extern void fibo_file_seekdir(DIR *pdir, long loc);
extern INT32 fibo_file_creat(const char *path, mode_t mode);
extern INT32 fibo_file_fstat(int fd ,struct stat *st);
extern INT32 fibo_file_feof(const char *file_path,int fd );
extern INT32 fibo_file_ftell(int fd);
extern INT32 fibo_filedir_exist(const char *file_path);
extern INT32 fibo_file_unzip(const char *file_path,const char *zip_path);
extern INT32 fibo_file_differdir(const char *file_path);








//ssl
void fibo_set_ssl_chkmode(UINT8 mode);
INT32 fibo_get_ssl_errcode(void);
INT32 fibo_write_ssl_file(INT8 *type_str, UINT8 *buff, UINT16 len);
void fibo_ssl_default(void);
INT32 fibo_ssl_sock_create(void);
INT32 fibo_ssl_sock_close(INT32 sock);
INT32 fibo_ssl_sock_connect(INT32 sock, const char *addr, UINT16 port);
INT32 fibo_ssl_sock_send(INT32 sock, UINT8 *buff, UINT16 len);
INT32 fibo_ssl_sock_recv(INT32 sock, UINT8 *buff, UINT16 len);
UINT32 fibo_ssl_sock_get_fd(INT32 sock);

//i2c
typedef struct
{
   int i2c_name;
   int value;
}HAL_I2C_BPS_T;

extern INT32 i2c_open(HAL_I2C_BPS_T bps);
extern INT32 i2c_close(void);
extern INT32  i2c_send_data(UINT32 slaveAddr,UINT32 memAddr,bool is16bit,UINT8 *pData,UINT8 datalen);
extern INT32 i2c_get_data(UINT32 slaveAddr,UINT32 memAddr,bool is16bit,UINT8 *pData,UINT8 datalen);

//Lcd
extern INT32 fibo_lcd_register(lcdCfg_t * lcd, void * data);
extern INT32 fibo_lcd_init(void);
extern INT32 fibo_lcd_deinit(void);
extern INT32 fibo_lcd_FrameTransfer(const lcdFrameBuffer_t * pstFrame, const lcdDisplay_t * pstWindow);
extern INT32 fibo_lcd_SetOverLay(const lcdFrameBuffer_t * pstFrame);
extern INT32 fibo_lcd_Sleep(BOOL mode);
extern INT32 fibo_lcd_SetPixel(UINT16 ulx, UINT16 uly, UINT16 ulcolor);
extern INT32 fibo_lcd_FillRect(const lcdDisplay_t * pstWindow, UINT16 ulBgcolor);
extern INT32 fibo_lcd_SetBrushDirection(lcdDirect_t direct_type);
extern INT32 fibo_lcd_DrawLine(UINT16 ulSx, UINT16 ulSy, UINT16 ulEx, UINT16 ulEy, UINT16 ulColor);
extern INT32 fibo_lcd_Getinfo(uint32_t * puldevid, uint32_t * pulwidth, uint32_t * pulheight);
extern INT32 fibo_lcd_Setvideosize(uint32_t width, uint32_t height, uint32_t imageBufWidth);
extern INT32 fibo_lcd_rstPin_setLevel(SAPP_IO_ID_T id,UINT8 level);
extern INT32 fibo_lcd_spi_config(uint8_t spilinetype, uint32_t spiclk);
extern INT32 fibo_lcd_spi_set_freq(uint32_t spiclk);
extern INT32 fibo_lcd_spi_set_mode(uint8_t spilinetype);
extern INT32 fibo_lcd_spi_write_cmd(uint16_t cmd);
extern INT32 fibo_lcd_spi_write_data(uint16_t data);
extern INT32 fibo_lcd_spi_read_data(uint16_t addr, uint8_t * pData, uint32_t len);
/*onenet*/
extern INT32 fibo_onenet_create(UINT32 total_size, UINT8 *config, UINT32 index, UINT32 cur_size);
extern INT32  fibo_onenet_delete(UINT32 ref);
extern INT32 fibo_onenet_open(UINT32 ref, UINT32 lifetime);
extern INT32  fibo_onenet_close(UINT32 ref);
extern INT32 fibo_onenet_addobj(UINT32 ref, UINT32 objid, UINT32 instanceconut, UINT8 *instancebitmap, UINT32 attrcount, UINT32 actoncount);
extern INT32  fibo_onenet_delobj(UINT32 ref, UINT32 objid);
extern INT32  fibo_onenet_notify(UINT32 ref, UINT32 msgid, INT32 objid, INT32 instid, INT32 resid, INT32 valuetye, INT32 length, UINT8 *value, INT32 index, INT32 ackid);
extern INT32  fibo_onenet_read_response(UINT32 ref, UINT32 msgid, UINT32 retcode, INT32 objid, INT32 instid, INT32 resid, INT32 valuetye, INT32 length, UINT8 *value, INT32 index);
extern INT32  fibo_onenet_write_response(UINT32 ref, UINT32 msgid, UINT32 retcode);
extern INT32  fibo_onenet_execute_response(UINT32 ref, UINT32 msgid, UINT32 retcode);
extern INT32  fibo_onenet_observe_response(UINT32 ref, UINT32 msgid, UINT32 retcode);
extern INT32  fibo_onenet_discover_response(UINT32 ref, UINT32 msgid, UINT32 retcode, UINT32 length, UINT8 *value);
extern INT32  fibo_onenet_parameter_response(UINT32 ref, UINT32 msgid, UINT32 retcode);
extern INT32  fibo_onenet_update(UINT32 ref, UINT32 lifetime, INT32 withobjectflag);

//camera

extern INT32 fibo_camera_init(void);
extern INT32 fibo_camera_deinit(void);
extern INT32 fibo_camera_GetSensorInfo(CAM_DEV_T * pstCamDevice);
extern INT32 fibo_camera_CaptureImage(UINT16 * * pFrameBuf);
extern INT32 fibo_camera_StartPreview(void);
extern INT32 fibo_camera_StopPreview(void);
extern INT32 fibo_camera_GetPreviewBuf(UINT16 * * pPreviewBuf);
extern INT32 fibo_camera_PrevNotify(UINT16 * pPreviewBuf);
extern void fibo_camera_led_open(bool enable);
extern bool fibo_camera_led_setLevel(uint32_t led_level);


//app ota
extern INT32 fibo_ota_handle(INT8 *data, UINT32 len);
extern INT32 fibo_ota_handle_no_reboot(INT8 *data, UINT32 len);
extern INT32 fibo_open_ota(UINT8 type, UINT8 *url,UINT8 *filename,UINT8 *username,UINT8 *password);
void fibo_ota_set_callback(fota_download_cb_t cb);
extern INT32 fibo_app_check(INT8 *data, UINT32 len);
extern INT32 fibo_app_dl(UINT8 type, UINT8 *url,UINT8 *filename,UINT8 *username,UINT8 *password);
extern INT32 fibo_app_update(void);
extern INT32 fibo_appfota_ota(UINT8 type, UINT8 *url,UINT8 *filename,UINT8 *username,UINT8 *password);
extern INT32 fibo_appfota_handle(INT8 *data, UINT32 len);
extern INT32 fibo_appfota_handle_no_reboot(INT8 *data, UINT32 len);



//firmware ota
extern INT32 fibo_firmware_handle(INT8 *data, UINT32 len);
extern INT32 fibo_firmware_handle_no_reboot(INT8 *data, UINT32 len);
extern INT32 fibo_firmware_ota(UINT8 type, UINT8 *url,UINT8 *filename,UINT8 *username,UINT8 *password);
extern INT32 fibo_firmware_dl(UINT8 type, UINT8 *url,UINT8 *filename,UINT8 *username,UINT8 *password);
extern INT32 fibo_firmware_update(void);
extern INT32 fibo_ota_errcode(void);

//appfota+firmware ota
extern INT32 fibo_appfw_ota(UINT8 type, UINT8 *url,UINT8 *filename,UINT8 *username,UINT8 *password);
extern INT32 fibo_appfw_handle(INT8 *data, UINT32 len);
extern INT32 fibo_appfw_handle_no_reboot(INT8 *data, UINT32 len);


//audio
extern INT32 fibo_audio_play(INT32 type,INT8 * filename);
extern INT32 fibo_audio_path_play(INT32 type,INT8 * filepath,auPlayerCallback_t cb_ctx);
extern INT32 fibo_eaudio_play(INT32 type, INT8 *filename);
extern INT32 fibo_audio_stop(void);
extern INT32 fibo_audio_pause(void);
extern INT32 fibo_audio_resume(void);
extern INT32 fibo_audio_list(void);
extern INT32 fibo_audio_mem_play(UINT8 format, UINT8 * buff, UINT32 size, auPlayerCallback_t cb_ctx);
extern INT32 fibo_tone_play(uint8_t tone_state, uint8_t tone_id, uint32_t tone_duration, uint8_t mix_factor);
extern INT32 fibo_set_audio_volume(uint8_t feature, uint8_t vol);
extern INT32 fibo_set_microphone_gain_value(uint8_t value);
extern INT32 fibo_get_microphone_gain_value(uint8_t *value);
extern INT32 fibo_set_acoustic_echo_canceler(uint8_t mode, uint8_t path, uint8_t ctrl, const char *param, uint16_t param_len);
extern INT32 fibo_get_acoustic_echo_canceler(uint8_t mode, uint8_t path, uint8_t ctrl, char *param);
extern int32_t fibo_audio_stream_play(uint8_t format, uint8_t *buff, uint32_t *Rp, uint32_t *Wp, uint32_t buffsize, int32_t timeout);
extern INT32 fibo_audio_list_play(const char fname[][128], int32_t num);

//inner flash
extern INT32 fibo_inner_flash_init(void);
extern INT32 fibo_inner_flash_info(UINT32 *pulId, UINT32 *pulCapacity);
extern INT32 fibo_inner_flash_read(UINT32 faddr, UINT8 *data,UINT32 size);
extern INT32 fibo_inner_flash_write(UINT32 faddr, UINT8 *data,UINT32 size);
extern INT32 fibo_inner_flash_erase(UINT32 faddr, UINT32 size);

//fcntl
extern INT32  fibo_sock_lwip_fcntl(int s, int cmd, int val);

//ping
extern int32_t fibo_mping(uint8_t ping_mode, char *ip, uint16_t total_num, uint16_t packet_len, uint8_t ping_ttl, uint32_t ping_tos, uint16_t ping_time_out);

//ext flash
extern void fibo_ext_flash_mode_set(uint8_t mode);
extern bool  fibo_ext_flash_poweron(uint8_t pinsel, uint8_t val, uint8_t div);
extern INT32 fibo_ext_flash_init(UINT8 Pinsel);
extern INT32 fibo_ext_flash_info(UINT32 * pulId, UINT32 * pulCapacity);
extern INT32 fibo_ext_flash_read(UINT32 faddr, UINT8 *data,UINT32 size);
extern INT32 fibo_ext_flash_write(UINT32 faddr, UINT8 *data,UINT32 size);
extern INT32 fibo_ext_flash_erase(UINT32 faddr, UINT32 size);

#ifdef CONFIG_FIBOCOM_NANDFLASH
//nand flash
extern void fibo_spi_nand_init(void);
extern void fibo_spi_nand_get_id(void);
extern int fibo_spi_nand_erase(uint32_t block);
extern int fibo_spi_nand_read(unsigned char *buf,unsigned int block,unsigned int stradr, unsigned int len);
extern int fibo_spi_nand_write(unsigned char *buf,unsigned int block,unsigned int stradr, unsigned int len);
#endif
//get IMEI
extern INT32 fibo_get_imei(UINT8 *imei, CFW_SIM_ID nSimID);
//custom API for ZHONGYIWULIAN
#ifdef CONFIG_MC615_CN_02_90_1200_ZHONGYIWULIAN
extern INT32 fibo_get_zy_imei(UINT8 *imei, CFW_SIM_ID nSimID);
extern INT32 fibo_set_zy_imei(UINT8 *imei, CFW_SIM_ID nSimID);
#endif

//send odm
extern INT32 fibo_send_odm(void *buf, size_t size);

//volume
extern INT32 fibo_set_volume(AUDIO_VOLUME_MODE_T mode, uint8_t level);
extern INT32 fibo_get_volume(AUDIO_VOLUME_MODE_T mode);
extern INT32 fibo_set_volume_external(AUDIO_VOLUME_MODE_T mode, uint8_t level);
extern INT32 fibo_get_volume_external(AUDIO_VOLUME_MODE_T mode);
extern bool  fibo_audio_volume_line_config(bool En);

extern INT32 fibo_UNI2GB(UINT16 unicode_char, UINT16* gb_char);
extern INT32 fibo_GB2UNI(UINT16 gb_char, UINT16* unicode_char);

//ringtone
extern INT32 fibo_MY_Ringtone(bool is_def_tone);
extern INT32 fibo_set_pwr_callback(key_callback pwr_cb,UINT16 long_press);
extern INT32 fibo_set_pwr_callback_ex(key_callback pwr_cb, UINT16 long_press, void * arg);
extern INT32 fibo_set_shutdown_mode(osiShutdownMode_t mode);

/* OpenCPU HTTP API */
extern oc_http_param_t *fibo_http_new();
extern void fibo_http_delete(oc_http_param_t *pstHttpParam);
extern INT32 fibo_http_get(oc_http_param_t *pstHttpParam, UINT8 *pUserDefHeader);
extern INT32 fibo_http_head(oc_http_param_t *pstHttpParam, UINT8 *pUserDefHeader);
extern INT32 fibo_http_post(oc_http_param_t *pstHttpParam, UINT8 *pPostData,UINT8 *pUserDefHeader);
extern INT32 fibo_http_response_header_foreach(oc_http_param_t *pstHttpParam, http_read_header_callback callback);
extern INT32 fibo_http_response_status_line(oc_http_param_t *pstHttpParam);
extern INT32 fibo_http_post_ex(oc_http_param_t *pstHttpParam, UINT8 *pPostData,UINT8 *pUserDefHeader,size_t length);
extern INT32 fibo_http_send_big_data(oc_http_param_t *pstHttpParam, UINT8 *pUserDefHeader, uint32_t buf_len);
extern INT32 fibo_http_post_data_file(oc_http_param_t *pstHttpParam, UINT8 *pUserDefHeader, uint32_t buf_len, char *pFileDir);

//sftp
INT32 fibo_sftp_open(char * remoteaddr,  const char *username,const char *password,int32_t uRemotePort);
INT32 fibo_sftp_close(void);
INT32 fibo_sftp_write(const char *loclpath,const char *sftppath);
INT32 fibo_sftp_read(const char *sftppath,const char *localfile);


//for Tianyu make
extern UINT8 fibo_read_key(void);
extern UINT8 fibo_get_pwrkeystatus(void);
extern UINT8 fibo_get_Usbisinsert(void);
extern void  fibo_SetLcdBright(UINT8 level);
extern bool  fibo_Rtc2Second(uint8_t * DateTimeBuf, uint32_t * OutSec);
extern bool  fibo_Second2Rtc(uint32_t SecondTime, uint8_t * DateTimeBuf);
extern INT32 fibo_GetBatVolResult(void);
extern UINT8 fibo_GetBatRemainlvl(void);
extern UINT8 fibo_GetBatChargPinStatus(void);
extern bool  fibo_SetBatChargSel(UINT8 level);
extern INT32 fibo_GetHardVer(void);
extern bool  fibo_pwrkey_init(void);
extern void *  fibo_app_start(uint32_t appimg_flash_offset,uint32_t appimg_flash_size,uint32_t appimg_ram_offset,uint32_t appimg_ram_size,void *parm);
extern UINT16 fibo_GetPowerOnCause(void);
extern bool fibo_get_audio_status(void);
extern INT32 fibo_GetChipID(INT8 *cfsnNUM);
extern bool fibo_TY_wakeup_init(UINT8 pin,UINT8 level);
extern bool fibo_TY_wakeup_enable(void);
extern bool fibo_TY_wakeup_disable(void);

/* for hui-er-feng & xin da lu */
typedef enum {
	KEY_RELEASE,
	KEY_PRESS,
	SHORT_TIME_PRESS,
	LONG_TIME_PRESS,
}keypad_status;

typedef struct {
	int32_t key_id;
	int8_t long_or_short_press;
	int8_t press_or_release;
}keypad_info_t;
extern bool fibo_pwrkeypad_config(UINT32 timeout, bool Ispwroff);
extern keypad_info_t fibo_get_pwrkeypad_status(void);
extern bool fibo_keypad_queue_init(UINT32 timeout);
extern bool fibo_keypad_queue_output(keypad_info_t * key);
extern bool fibo_bykeyid_get_status(UINT32 id);

//tts
bool fibo_tts_is_playing();
extern INT32 fibo_tts_stop();
extern INT32 fibo_tts_start(const UINT8 *pData, INT8 cEncode);
extern bool fibo_pwmOpen(void);
extern void fibo_pwmClose(void);
extern bool fibo_pwtConfig(uint32_t period_count,uint8_t prescaler,uint32_t duty);
extern bool fibo_pwtStartorStop(bool is_start);
extern bool fibo_chargeOpenorClose(bool is_open);
extern void fibo_SetPwlLevel(UINT8 index,UINT8 level);
extern INT32 fibo_tts_delay_play_init(UINT32 ms);

//device Number
extern INT32 fibo_set_get_deviceNUM(INT32 type,INT8 *deviceNUM,size_t length);
extern INT32 fibo_get_device_num(INT8 *deviceNUM,size_t length);


//get device sleep state
extern  bool fibo_GetDeviceSleepState(void);
extern INT32 fibo_set_prior_RAT(UINT8 nPreferRat,CFW_SIM_ID nSim);
extern UINT8 fibo_get_curr_prior_RAT(CFW_SIM_ID nSim);
extern INT32 fibo_get_chipid(uint32_t *chipid,uint32_t *metaid);

//public interphone interface
extern int fibo_custom_app_register(void *appHandleFun);
extern int fibo_receive_interphone_app_message(char *msg);
extern INT32 fibo_log_uart_output(hal_uart_port_t uart_port, hal_uart_config_t *uart_config,char *trace);
extern INT32 fibo_log_uart_output_close(hal_uart_port_t uart_port);

//Audio Streaming Recording
extern int32_t fibo_audio_recorder_stream_stop(void);
extern int32_t fibo_audio_recorder_stream_start_ex(uint8_t format, uint8_t *buf, uint32_t *Wp, uint32_t buffsize, auAmrnbMode_t amr_mode);
extern int32_t fibo_audio_recorder_stream_start(uint8_t format, uint8_t *buf, uint32_t *Wp, uint32_t buffsize);

//loopback
extern int32_t fibo_audio_loopback(audevOutput_t output, audevInput_t input, bool is_lp, uint8_t volume);

//yingzhihui custom enable external pa
extern INT32 fibo_external_PA_enable(uint16_t pin_id);
extern INT32 fibo_external_PA_enable_level(uint16_t pin_id);
extern INT32 fibo_external_PA_enable_fun(uint8_t mode,uint16_t pin_id);
extern INT32 fibo_extern_PA_disable_level(void);


extern INT32 fibo_set_initial_pdp(INT8 set_delete, uint8_t pdptype, char *apn, uint8_t nAuthProt,char *username,char *pwd,CFW_SIM_ID nSim);
extern void fibo_hmacsha1(int md_type, const UINT8 *api_secret, int key_len, const UINT8 *payload, int payload_len, UINT8 *encrypt_result, int len);

//TTS voice speed and pitch
extern INT32 fibo_tts_voice_speed(INT32 speed);
extern INT32 fibo_tts_voice_pitch(INT32 pitch);
extern INT32 fibo_get_tts_voice_speed(void);
extern INT32 fibo_get_tts_voice_pitch(void);
//usb
extern atDevice_t *FIBO_usbDevice_init(uint32_t name, usb_input_callback_t recv_cb);
extern int FIBO_usbDevice_send(atDevice_t *th, const void *data, size_t length);
extern int FIBO_usbDevice_State_report(atDevice_t *th, Report_UsbDev_Event  usedev_Event);
extern int fibo_atDevicePortClose();
extern int fibo_atDevicePortOpen();

extern INT32 fibo_getVbatStaticVol(void);
extern INT32 fibo_getVbatVolLevel(void);
extern bool fibo_setChargeConf(chargerinfo_t * chg_param_t);
extern INT32 fibo_getChargeCur(void);
extern INT8 *fibo_get_hw_verno(void);
extern INT8 *fibo_get_sw_verno(void);
extern INT32 fibo_set_app_version(char *ver);
extern INT32 fibo_get_version_info(char *outstr, int32_t outlen);


//for HengKe, Other customers can not use this function, otherwise it will cause fatal problems
extern INT32 fibo_set_acoustic_parameter_for_HengKe(void);
extern INT32 fibo_set_acoustic_parameter_for_TCL(void);
extern INT32 fibo_set_acoustic_parameter_for_SanNuo(void);
extern INT32 fibo_set_acoustic_parameter_for_AiTi(void);
extern INT32 fibo_set_acoustic_para(uint8_t type);
extern INT32 fibo_recovery_value(void);
//spi
extern INT32 fibo_spi_open(drvSpiConfig_t cfg, SPIHANDLE * spiFd);
extern INT32 fibo_spi_close(SPIHANDLE * spiFd);
extern INT32 fibo_spi_send(SPIHANDLE spiFd, SPI_IOC spiIoc, void * sendaddr, uint32_t size);
extern INT32 fibo_spi_recv(SPIHANDLE spiFd, SPI_IOC spiIoc, void * sendaddr, void * readaddr, uint32_t size);
extern INT32 fibo_spi_pinctrl(SPIHANDLE spiFd, drvSpiPinCtrl pinctrl, drvSpiPinState pinstate);

// classic BT
extern INT32 fibo_bt_onoff(uint8_t onoff);
extern INT32 fibo_bt_visible(uint8_t visible);
extern INT32 fibo_bt_set_read_name(uint8_t set_read,char *name,uint8_t name_type);
extern INT32 fibo_bt_set_read_addr(uint8_t set_read,char *address);
extern INT32 fibo_bt_scan_nearby(uint8_t mode,char *type);
extern INT32 fibo_bt_pair(uint8_t mode,char *address);
extern INT32 fibo_bt_pin(char *pin);
extern INT32 fibo_bt_list_paired_devices(FIBO_BT_DEVICE_T *dev_info);
extern INT32 fibo_bt_remove_paired_devices(uint8_t mode,char *address);
extern INT32 fibo_bt_connect(uint8_t mode, char *profile, char *address);
extern INT32 fibo_bt_send_data(char *profile,uint8_t type,char *data,uint32_t len);
extern INT32 fibo_bt_read_data(char *profile,char *data ,uint16_t *len);

//BLE
extern INT32 fibo_ble_mode(uint8_t set_read,uint8_t mode);
extern INT32 fibo_ble_get_ver(char *ver);
extern INT32 fibo_ble_set_read_addr(uint8_t set_read,uint8_t addr_type,char *addr);
extern INT32 fibo_ble_add_remove_whitelist(uint8_t set_read,uint8_t add_remove,uint8_t type,char *addr,fibo_ble_whilelist_t *ble_whilelist);
extern INT32 fibo_ble_set_read_name(uint8_t set_read,uint8_t *name,uint8_t name_type);

//BLE CLIENT
extern INT32 fibo_ble_connect(uint8_t connect,uint8_t addr_type,char *addr);
extern INT32 fibo_ble_get_connect_state(int *state,char *addr);
extern INT32 fibo_ble_update_connection(int Handle,int MaxInterval,int MinInterval,int Latency,int Timeout);
extern INT32 fibo_ble_set_scan_param(         int Type,int Interval,int Window,int Filter);
extern INT32 fibo_ble_scan_enable(int ScanEnable);
extern INT32 fibo_ble_client_int(void *parma);
extern INT32 fibo_ble_client_server_int(fibo_ble_btgatt_callback_t *parma);

extern INT32 fibo_ble_client_filter_manufacturer(UINT8 *data);
extern INT32 fibo_ble_client_filter_uuid(UINT8 *uuid);
extern INT32 fibo_ble_client_filter_name(UINT8 *name);
extern INT32 fibo_ble_client_connect_cancel(bdaddr_t *address);
extern INT32 fibo_ble_client_discover_all_primary_service(UINT16 module, UINT16 acl_handle);
extern INT32 fibo_ble_client_discover_primary_service_by_uuid(UINT16 module, UINT8 *uuid_l, UINT16 uuid_s, UINT16 acl_handle);
extern INT32 fibo_ble_client_discover_all_characteristic(UINT16 module, UINT16 start_handle, UINT16 end_handle, UINT16 acl_handle);
extern INT32 fibo_ble_client_read_char_value_by_handle(UINT16 module, UINT16 att_handle, UINT8 is_long_att, UINT16 acl_handle);
extern INT32 fibo_ble_client_read_char_value_by_uuid(UINT16 module, UINT8 *uuid_l, UINT16 uuid_s, UINT16 start_handle, UINT16 end_handle, UINT16 acl_handle);
extern INT32 fibo_ble_client_read_multiple_char_value(UINT16 module, UINT8* handle, UINT8 length, UINT16 acl_handle);
extern INT32 fibo_ble_client_write_char_value(UINT16 module, UINT16 att_handle, void *value, UINT16 length, UINT16 acl_handle);
extern INT32 fibo_ble_client_write_char_value_without_rsp(UINT16 module, UINT16 att_handle, void *value, UINT16 length, UINT16 acl_handle);
extern INT32 fibo_ble_client_write_char_value_signed(UINT16 module, UINT16 att_handle, void *value, UINT16 length, UINT16 acl_handle);
extern INT32 fibo_ble_client_write_char_value_reliable(UINT16 module, UINT16 att_handle, void *value, UINT16 length, UINT16 acl_handle);
extern INT32 fibo_ble_client_find_include_service(UINT16 module, UINT16 start_handle, UINT16 end_handle, UINT16 acl_handle);
extern INT32 fibo_ble_client_get_char_descriptor(UINT16 module, UINT16 start_handle, UINT16 end_handle, UINT16 acl_handle);
extern INT32 fibo_ble_client_read_char_descriptor (UINT16 module, UINT16 att_handle, UINT8 is_long_att, UINT16 acl_handle);
extern INT32 fibo_ble_client_write_char_descriptor(UINT16 module, UINT16 att_handle, void *value, UINT16 length, UINT16 acl_handle);

//BLE brodcast
extern INT32 fibo_ble_set_dev_param(int param_count,int AdvMin,int AdvMax,int AdvType,int OwnAddrType,int DirectAddrType,int AdvChannMap,int AdvFilter,char *DirectAddr);
extern INT32 fibo_ble_set_dev_data(int data_len,char *data);
extern INT32 fibo_ble_enable_dev(int onoff);
//BLE SERVER
extern INT32 fibo_ble_add_service_and_characteristic(const gatt_element_t *gatt_element, UINT32 size);
extern INT32 fibo_ble_notify(gatt_le_data_info_t *data,int type);
extern INT32 fibo_ble_set_scan_response_data(UINT8 *data,UINT8 length);

//ibeacon
extern INT32  fibo_ibeacon_enable(uint8_t AdvEnable);
extern INT32  fibo_set_ibeacon_param(int param_count,int AdvMin,int AdvMax,int AdvType,int OwnAddrType,int DirectAddrType,char *DirectAddr,int AdvChannMap,int AdvFilter);
extern INT32 fibo_ibeacon_adv(char *comapre_ID,char *UUID,char *minor,char *major);

//BT audio sink
extern INT32 fibo_bt_audio_int();
extern INT32 fibo_bt_audio_sink_set(int param);
extern INT32 fibo_bt_audio_sink_state_get(int *state);



//BASE64
extern INT32 fibo_base64_encode( unsigned char *dst, size_t dlen, size_t *olen,const unsigned char *src, size_t slen );
extern INT32 fibo_base64_decode( unsigned char *dst, size_t dlen, size_t *olen,const unsigned char *src, size_t slen );

extern INT32 fibo_ftp_open(char *url,char *username,char *passwd);
extern INT32 fibo_ftp_write(char *ftppath,char *localpath);
extern INT32 fibo_ftp_read(char *filename,char *downpath);
extern INT32 fibo_ftp_close();
extern INT32 fibo_set_mcal_dev(UINT8 ucDevValue,CFW_SIM_ID nSimID);
extern INT32 fibo_set_mcal(CFW_NW_NST_TX_PARA_CONFIG *nst_para_config, CFW_SIM_ID nSimID);
extern INT32 FIBO_usbDevice_Deinit(atDevice_t *th);
extern INT8 fibo_get_file_list(char *dir, void (*cb)(const char *filepath, size_t size, void *arg), void *args);
typedef struct
{
    CAM_DEV_T CamDev;
    lcdSpec_t lcddev;
    osiThread_t *camTask;
    uint8_t Decodestat;
    bool Openstat;
    bool MemoryState;
    bool gCamPowerOnFlag;
    bool issnapshot;
    uint32_t height;
    uint32_t width;
    uint32_t sweepsize;
    uint32_t times;
} camastae_t;

extern bool fibo_sweep_code(camastae_t *cam, uint16_t *pFrame, uint8_t *pDatabuf, uint8_t *pResult, uint32_t *pReslen, int *type);
extern INT32 fibo_hal_pmu_setlevel(UINT8 pmu_type,UINT8 level);
extern INT32 fibo_hal_adc_get_data(hal_adc_channel_t channel,UINT32 scale_value);
extern INT32 fibo_sim_SendApdu(CFW_SIM_ID nSim,uint8_t Channel, uint8_t *pdu,uint16_t pdu_len,osiEventCallback_t  sim_response_cb, void *cb_ctx);

#ifdef CONFIG_FIBOCOM_LG610
extern bool fibo_open_cam_led(int32_t level);
extern void fibo_close_cam_led();
#endif
extern INT32 fibo_audrec(uint8_t oper, uint8_t filenum, uint8_t format, uint8_t duration, char *user_name, auAmrnbMode_t amr_mode);
extern INT8 fibo_usbswitch(uint8_t en);
extern INT32 fibo_set_volume_ex(AUDIO_VOLUME_MODE_T mode, uint8_t level);

//uart-to-485
extern INT32 fibo_rs485_init(hal_uart_port_t uart_port, hal_uart_config_t * uart_config, uint16_t ctrl_pin, uart_input_callback_t recv_cb, void * arg);
extern INT32 fibo_rs485_deinit(hal_uart_port_t uart_port);
extern INT32 fibo_rs485_write(int uart_port, UINT8 * buff, UINT32 len, bool wait_finish);
extern INT32 fibo_rs485_read(int uart_port, UINT8 * buff, UINT32 len);
extern INT32 fibo_rs485_read_try(int uart_port, UINT8 * buff, UINT32 len, uint32_t timeout);
extern INT32 fibo_rs485_read_block(int uart_port, UINT8 * buff, UINT32 len, uint32_t timeout);
extern INT32 fibo_setRTC_timezone(int8_t timezone);
extern INT32 fibo_getRTC_timezone(void);
extern UINT8 fibo_getbootcause(void);
extern INT32 fibo_ischargefull(void);

//xml
extern XML_Parser fibo_xml_parser_create(const XML_Char *encodingName);
extern void fibo_xml_set_user_data(XML_Parser parser, void *p);
extern void fibo_xml_set_element_handle(XML_Parser parser,
                                    XML_StartElementHandler start,
                                    XML_EndElementHandler end);
extern void fibo_xml_set_character_data_handler(XML_Parser parser, XML_CharacterDataHandler handler);
extern enum XML_Status fibo_xml_parse(XML_Parser parser, const char *s, int len, int isFinal);
extern void fibo_xml_parser_free(XML_Parser parser);

extern INT32 fibo_find_sim(void);

//wifi scan
extern INT32 fibo_wifiscan_on();
extern INT32 fibo_wifiscan_off();
extern INT32 fibo_wifiscan(key_callback wifiscan_refponse_cb);
extern INT32 fibo_getCellInfo_ex(key_callback cellinfo_refponse_cb,CFW_SIM_ID nSim);
extern UINT64 fibo_getSysTick_ext(void);

extern INT32 fibo_get_AT_powercause(void);

//ccid
extern drvCCID_t *fibo_usb_ccid_open(uint32_t name);
extern void fibo_usb_ccid_set_line_change_callback(drvCCID_t *ccid, ccidLineCallback_t cb, void *ctx);
extern bool fibo_usb_ccid_online(drvCCID_t *ccid);
extern int fibo_usb_ccid_slot_check(drvCCID_t *ccid, uint8_t *slotmap);
extern int fibo_usb_ccid_slot_change(drvCCID_t *ccid, uint8_t index, bool insert);
extern int fibo_usb_ccid_read(drvCCID_t *ccid, struct ccid_bulk_out_header *buf, unsigned size);
extern int fibo_usb_ccid_write(drvCCID_t *ccid, const struct ccid_bulk_in_header *data, unsigned size);
extern void fibo_usb_ccid_close(drvCCID_t *ccid);

extern INT32 fibo_charger_turnon(void);
extern INT32 fibo_charger_turnoff(void);
extern INT32 fibo_ischarger_overvol(void);
extern INT32 fibo_set_usbmode(uint8_t usbmode);
extern INT32 fibo_get_usbmode(void);

typedef struct {
    uint8_t reg;
    uint8_t value;
}camera_reg_t;

extern bool fibo_camera_regs_init_cus(char *Sensor_name, camera_reg_t *cam_regs, int32_t num);
extern bool fibo_camera_read_reg_cus(char *Sensor_name, uint8_t addr, uint8_t *value);
extern bool fibo_camera_write_reg_cus(char *Sensor_name, uint8_t addr, uint8_t data);

typedef struct {
    uint8_t command;
    uint8_t length;
    uint8_t *CommandValue;
}lcd_reg_t;

extern void fibo_lcd_regs_init_cus(lcd_reg_t *regs, int32_t Length);
extern void fibo_lcd_write_reg_cus(uint8_t addr, uint8_t *CommandValue, int32_t Length);
extern void fibo_lcd_read_reg_cus(uint8_t addr, uint8_t *data, int32_t Length);
extern INT32 fibo_gpio_pull_high_resistance(SAPP_IO_ID_T id,bool is_pull_high);
extern bool fibo_phone_is_working(void);
extern INT32 fibo_audio_stream_stop(void);
//mbedtls tcos
extern uintptr_t fibo_tls_connect(GAPP_TLSConnectParams *pConnectParams, const char *host, int port);
extern void fibo_tls_disconnect(uintptr_t handle);
extern int fibo_tls_write(uintptr_t handle, unsigned char *msg, size_t totalLen, uint32_t timeout_ms, size_t *written_len);
extern int fibo_tls_read(uintptr_t handle, unsigned char *msg, size_t totalLen, uint32_t timeout_ms, size_t *read_len);
//sercureboot
extern bool fibo_get_security_flag(void);
extern void fibo_set_security_flag(void);
extern INT32 fibo_SpecialGpio_Init(void);

extern INT32 fibo_sink_OnOff(UINT32 sinknum, BOOL onOff);
extern INT32 fibo_sinkLevel_Set(UINT32 sinknum, UINT32 light_level);
extern bool fibo_keypad_msg_mod(bool  en);
extern void fibo_charger_set_notice_callbk(drvChargerNoticeCB_t cb);
extern uint32_t fibo_RFCAL_POC();
extern INT32 fibo_sock_ntp(char *addr, int port, CFW_SIM_ID nSimID, key_callback cb);
extern INT32 fibo_external_PA_close_delay(uint32_t tone_duration);
extern bool fibo_ischargering(void);
extern UINT32 fibo_timer_new_Irq(void (*fn)(void *arg),void * arg);
extern INT32 fibo_timer_start_us(UINT32 timerid, uint32_t us);

#define MAX_CODE_TYPE_LEN (8)
#define MAX_CODE_TYPE     (12)

typedef struct {
    char codeType[MAX_CODE_TYPE_LEN];
    uint8_t codeValue;
}barCodeEx_t;

extern bool fibo_set_bar_code(barCodeEx_t *bc, uint8_t num);
extern bool fibo_get_decode_author_status();
extern void fibo_camera_cleanbuf();
extern INT32 fibo_tone_play_standy_volume(uint8_t tone_state, uint8_t tone_id, uint32_t tone_duration);

extern INT8 fibo_show_sdcard_space(unsigned long long *total,unsigned long long *used,unsigned long long *rest);
extern bool fibo_format_sdc(int sdc_num);
extern bool fibo_mount_sdc(int sdc_num, const char *path);
extern void fibo_umount_sdc(const char *path);
// efuse
bool fibo_efuse_write(uint32_t index, uint32_t value);
bool fibo_efuse_read(uint32_t index, uint32_t *value);
extern bool fibo_pmu_ctrl(UINT32 id, BOOL enabled, BOOL lp_enabled);

extern INT32 fibo_lcd_spi_config(uint8_t spilinetype, uint32_t spiclk);
extern INT32 fibo_lcd_spi_set_freq(uint32_t spiclk);
extern INT32 fibo_lcd_spi_set_mode(uint8_t spilinetype);
extern INT32 fibo_lcd_spi_write_cmd(uint16_t cmd);

#if defined(CONFIG_FIBOCOM_LG610)
extern bool  fibo_enable_heartbeat(uint8_t cid,  uint8_t nsim, char mode, char potocal);
extern bool  fibo_set_heartbeat_param(char *serverip, uint16_t port,  uint16_t time, char *hexdata);
extern bool  fibo_set_heartbeat_reconnect_timer(uint16_t time1, uint16_t time2,uint16_t time3,uint16_t time4,uint16_t time5,uint16_t time6);
#endif
extern INT32 fibo_i2c_open(HAL_I2C_BPS_T bps,i2c_Handle *i2c_handle);
extern INT32 fibo_i2c_close(i2c_Handle *i2c_handle);
extern INT32 fibo_i2c_Write(i2c_Handle i2c_handle,drvI2cSlave_t idAddress,UINT8 *Buffer,UINT8 bufferlen);
extern INT32 fibo_i2c_Read(i2c_Handle i2c_handle,drvI2cSlave_t idAddress,UINT8 *Buffer,UINT8 bufferlen);
extern INT32 fibo_i2c_xfer(i2c_Handle i2c_handle,drvI2cSlave_t idAddress,UINT8 *send_Buffer,UINT8 send_bufferlen,UINT16 *recv_Buffer,UINT8 recv_bufferlen);

extern void fibo_dotMatrixLcdSetDisplayParam(uint8_t contrastRatio_coarse, uint8_t contrastRatio_fine, uint8_t bias);
extern void fibo_dotMatrixLcdSetLcmRate(uint8_t rate);
extern void fibo_dotMatrixLcdSendData(uint8_t column, uint8_t row, void *buff, uint32_t size);
extern void fibo_dotMatrixLcdSetAddrOnly(uint8_t column, uint8_t row);
extern void fibo_dotMatrixLcdSendBuffOnly(void *buff, uint32_t size);
extern void fibo_dotMatrixLcdSetFlushDirection(uint8_t direction);



extern INT32 fibo_getChargeStatus(void);
#if defined(CONFIG_FIBOCOM_SWEEP_CODE_LIB_HUANGH) && defined(CONFIG_FIBOCOM_LIANDI)
extern char* fibo_get_sweep_lib_ver();
extern bool fibo_set_scan_mode(uint32_t camProdut,uint32_t camSensorModel,uint32_t camSensorParameters);
#endif

extern INT32 fibo_file_getFileNum(const char *dir_path);
extern int32_t fibo_audio_recorder_poc_start(uint8_t format,uint32_t samplerate,uint8_t *audio_buff,uint32_t *Wp,uint32_t buffsize,auAmrnbMode_t amr_mode);
extern int32_t fibo_audio_recorder_poc_stop(void);


//gnss
extern int32_t fibo_gnss_sync_power_on(void);
extern int32_t fibo_gnss_async_power_on(void);
extern int32_t fibo_gnss_power_off(void);
extern int32_t fibo_gnss_gtagps(uint8_t injectMode, uint8_t param);
extern int32_t fibo_gnss_hot_start(void);
extern int32_t fibo_gnss_cold_start(void);
extern int32_t fibo_gnss_get_data(uint8_t* data, int len);

extern int32_t fibo_audio_play_poc_start(uint8_t format, uint32_t samplerate, uint8_t *audio_buff, uint32_t *Rp, uint32_t *Wp, uint32_t buffsize,int32_t timeout);
extern int32_t fibo_audio_play_poc_stop(void);
extern INT32 fibo_sim_SendApdu_ex(CFW_SIM_ID nSim,uint8_t Channel, uint8_t *pdu,uint16_t pdu_len,key_callback sim_res_cb);
// tp
extern int32_t fibo_tp_init(tp_cfg_t tp_cfg);
extern int32_t fibo_tp_load_calib(bool is_reCalib, tp_point_t calib_coords[], tp_calib_cb_t tp_calib_cb);
extern int32_t fibo_tp_open(tp_touch_cb_t tp_couch_cb);
extern int32_t fibo_tp_deinit(void);
//ble server close
extern INT32 fibo_ble_server_close(UINT16 handle);
extern INT32 fibo_ble_get_broadcast_data(scan_broadcast_cb scan_broadcast_cb_f);
// shudtowm mode
extern int fibo_shutdown_normal_info(uint8_t rwmode,uint8_t sysmode);

// mipi lcd APIs
extern int32_t fibo_mipi_lcd_rst_pin(uint16_t rst_pin);
extern int32_t fibo_mipi_lcd_init(void);
extern int32_t fibo_mipi_lcd_deinit(void);
extern int32_t fibo_mipi_lcd_send_buff(const lcdDisplay_t *lcdRec, uint16_t *buffer);
extern int32_t fibo_mipi_lcd_fill_rect16(const lcdDisplay_t *lcdRec, uint16_t color);
extern int32_t fibo_mipi_lcd_set_pixel16(uint16_t x, uint16_t y, uint16_t color);
extern int32_t fibo_mipi_lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
extern int32_t fibo_mipi_lcd_sleep(bool isSleep);
extern int32_t fibo_mipi_lcd_set_brush_direction(lcdDirect_t direct_type);
extern int32_t fibo_mipi_lcd_get_info(uint32_t *puldevid, uint32_t *pulwidth, uint32_t *pulheight);

// mipi lcd extended APIs
extern int32_t fibo_mipi_lcd_get_fb(uint16_t **framebuff, uint16_t *width, uint16_t *height);
extern int32_t fibo_mipi_lcd_fb_flush(void);
extern int32_t fibo_mipi_lcd_get_status(dpi_stat_t *stat);
extern int32_t fibo_mipi_lcd_sw_frame(uint8_t frame);

//sim init
extern INT32 fibo_sim_init(void);
//get mcc mnc
extern INT32 fibo_get_mcc_mnc(UINT8 *mcc,UINT8 *mnc);
extern osiEventGroupHandle_t osiEventGroupCreate();
extern osiEventBits_t osiEventGroupSetBits(osiEventGroupHandle_t eventGroup, const osiEventBits_t bitsToSet);
extern osiEventBits_t osiEventGroupWaitBits(osiEventGroupHandle_t eventGroup, const osiEventBits_t bitsToWaitFor, const osiBaseType_t clearOnExit, const osiBaseType_t waitForAllBits, osiTickType_t ticksToWait);
extern osiBaseType_t osiEventGroupSetBitsFromISR(osiEventGroupHandle_t eventGroup, const osiEventBits_t bitsToSet, osiBaseType_t *higherPriorityTaskWoken);
extern osiEventBits_t osiEventGroupClearBits(osiEventGroupHandle_t eventGroup, const osiEventBits_t bitsToClear);
extern osiEventBits_t osiEventGroupSync( osiEventGroupHandle_t xEventGroup, const osiEventBits_t uxBitsToSet, const osiEventBits_t uxBitsToWaitFor, osiTickType_t xTicksToWait );
extern void  osiEventGroupDelete( osiEventGroupHandle_t xEventGroup );
extern osiEventBits_t osiEventGroupGetBitsFromISR( osiEventGroupHandle_t xEventGroup );
extern char *osiTaskGetName(osiThread_t *thread);
extern uint32_t osiThreadStackUnused(osiThread_t *thread);

extern INT32 fibo_hal_uart_data_to_send(int uart_port);
extern INT8 fibo_sim_detect_switch(uint8_t nSlotStatus);

// safe file operation
extern INT32 fibo_sfile_init(const INT32 *path);
extern INT32 fibo_sfile_size(const INT32 *path);
extern INT32 fibo_sfile_read(const INT32 *path, UINT8 *buf, UINT32 count);
extern INT32 fibo_sfile_write(const INT32 *path, UINT8 *buf, UINT32 count);

extern INT32 fibo_external_PA_open_delay(uint16_t delay);

extern INT32 fibo_camera_CaptureImage_save_jpeg(char *filepath, int quality);
//sim2 sw
extern INT32 fibo_hal_sim2_sw(bool sw);

//dumpinfos 
extern INT32 fibo_read_dumpinfo(UINT32 * plcount, char * pDumpInfo);
extern INT32 fibo_clean_dumpinfo(void);
  typedef int32_t (*callBackFunc)(void *arg);
  typedef struct {
       callBackFunc idleCallBack; 
       callBackFunc dumpCallBack; 
  }CUSTOM_CALLBACK_TYPE;
extern void fibo_oc_dump_cb_init(void* cb_t); /* cb_t is CUSTOM_CALLBACK_TYPE type point */

#define CPU_USAGE_IDLE_TASK 0
extern INT32 fibo_get_cpu_usage(UINT32 taskid);
extern INT32 fibo_cpu_usage_init(UINT32 interval);

//VSim
extern INT8 fibo_setVSimHandler(void *getSimType, void *vsimReset, void *vsimProcessApdu);
extern INT32 fibo_audio_list_play_v2(const char fname[][128], int32_t file_num);
extern INT32 fibo_audio_list_play_stop_v2(void);
extern INT32 fibo_audio_list_play_pause_v2(void);
extern INT32 fibo_audio_list_play_resume_v2(void);
extern INT32 fibo_audio_stream_stop_nowait(void);
extern INT32 fibo_get_app_flash_address(void);

/**
 * @brief general spi lcd APIs
*/
#include "drv_genspi_lcd.h"

extern int32_t fibo_genspi_lcd_init(genspi_lcd_cfg_t cfg, SPIHANDLE *spiFd);
extern int32_t fibo_genspi_lcd_deinit(void);
extern int32_t fibo_genspi_lcd_get_id(uint32_t *id);
extern int32_t fibo_genspi_lcd_send_reg_tab(lcd_reg_t *regs_tab, int32_t num);
extern int32_t fibo_genspi_lcd_get_info(uint32_t *width, uint32_t *height);
extern int32_t fibo_genspi_lcd_fill_rect16(const lcdDisplay_t *lcdRec, uint16_t color);
extern int32_t fibo_genspi_lcd_send_buff(const lcdDisplay_t *lcdRec, uint16_t *buffer);
extern int32_t fibo_genspi_lcd_set_pixel16(uint16_t x, uint16_t y, uint16_t color);
extern int32_t fibo_genspi_lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
extern int32_t fibo_genspi_lcd_set_direction(lcdDirect_t direct_type);
extern int32_t fibo_genspi_lcd_sleep(bool isSleep);
extern int32_t fibo_genspi_lcd_write_cmd(uint8_t cmd);
extern int32_t fibo_genspi_lcd_write_data(uint8_t data);
extern int32_t fibo_genspi_lcd_read_data(uint8_t cmd, uint32_t size, uint8_t *pBuf);

#endif
/**
 * function type of callback
 */
typedef void (*osiCallback_t)(void *ctx);
extern INT32 fibo_audio_recorder_start(osiThread_t *rec_thread,osiCallback_t cb,INT32 fd,uint8_t rec_type,uint8_t rec_format,auAmrnbMode_t amr_mode);
extern INT32 fibo_audio_recorder_stop(void);

extern INT32 fibo_appfw_handle_by_path_ex(INT8 *path);

extern void fibo_set_audio_delay_flag(bool aud_delay_status);
extern bool fibo_get_audio_delay_flag(void);

