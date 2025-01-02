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

extern void test_printf(void);

static void prvThreadEntry(void *param);

BOOL run = false;
BOOL quit = true;
int connect_res_result = 0;
static UINT32 g_lock = 0;
static UINT32 g_lock_wait_devicesecret = 0;
static bool g_fibo_aliyun_dynamic_register_flag = false;

#define HMAC_SHA1_LEN 20
#define HMAC_SHA256_LEN 32

#define IOTX_ALIYUN_PRODUCT_KEY_MAXLEN     (20)
#define IOTX_ALIYUN_DEVICE_NAME_MAXLEN     (32)
#define IOTX_ALIYUN_DEVICE_SECRET_MAXLEN   (64)
#define IOTX_ALIYUN_PRODUCT_SECRET_MAXLEN  (64)

#define IOTX_ALIUN_MQTT_CLIENTID_MAXLEN    (256)
#define IOTX_ALIYUN_MQTT_USERNAME_MAXLEN   (256)
#define IOTX_ALIYUN_MQTT_PASSWORD_MAXLEN   (256)
#define IOTX_ALIYUN_MQTT_HOSTNAME_MAXLEN (256)

#define IOTX_ALIYUN_SIGN_SOURCE_MAXLEN  (256)

#define MODE_TLS_DIRECT             "2"
#define IOTX_AOS_VERSION                "3.0.0"

#define TIMESTAMP_VALUE             "2524608000000"
#define SECURE_MODE                 MODE_TLS_DIRECT
#define SIGN_FMT_LEN  50  /*  "clientId%sdeviceName%sproductKey%stimestamp%s";*/

char g_fibo_aliyun_device_secret[64] = {0};

typedef struct _fibo_aliyun_device_info_st
{
	char product_key[IOTX_ALIYUN_PRODUCT_KEY_MAXLEN+1];
	char device_name[IOTX_ALIYUN_DEVICE_NAME_MAXLEN+1];
	char device_secret[IOTX_ALIYUN_DEVICE_SECRET_MAXLEN+1];
	char product_secret[IOTX_ALIYUN_PRODUCT_SECRET_MAXLEN+1];
}fibo_aliyun_device_info_st;

typedef struct _fibo_aliyun_mqtt_info_st
{
	char client_id[IOTX_ALIUN_MQTT_CLIENTID_MAXLEN+1];
	char user_name[IOTX_ALIYUN_MQTT_USERNAME_MAXLEN+1];
	char password[IOTX_ALIYUN_MQTT_PASSWORD_MAXLEN+1];
}fibo_aliyun_mqtt_info;

typedef struct {
    char hostname[IOTX_ALIYUN_MQTT_HOSTNAME_MAXLEN+1];
    uint16_t port;
    char clientid[IOTX_ALIUN_MQTT_CLIENTID_MAXLEN+1];
    char username[IOTX_ALIYUN_MQTT_USERNAME_MAXLEN+1];
    char password[IOTX_ALIYUN_MQTT_PASSWORD_MAXLEN+1];
} iotx_sign_mqtt_t;

static fibo_aliyun_device_info_st g_fibo_aliyun_device_info = {0};

const char *clientid_kv[][2] = {
    {
        "timestamp", TIMESTAMP_VALUE
    },
    {
        "_v", "aos-r-"IOTX_AOS_VERSION
    },
    {
        "securemode", SECURE_MODE
    },
    {
        "signmethod", "hmacsha256"
    },
    {
        "lan", "C"
    },
#ifdef MQTT_AUTO_SUBSCRIBE
    {
        "_ss", "1"
    },
#endif
#if 0
    {
        "v", IOTX_ALINK_VERSION
    },
#else
    {
        "gw", "0"
    },
    {
        "ext", "0"
    },
#endif
};

void mbedtls_md_init( mbedtls_md_context_t *ctx )
{
    memset( ctx, 0, sizeof( mbedtls_md_context_t ) );
}

int32_t infra_json_value(const char *input, uint32_t input_len, const char *key, uint32_t key_len, char **value, uint32_t *value_len)
{
    uint32_t idx = 0;

    for (idx = 0;idx < input_len;idx++) {
        if (idx + key_len >= input_len) {
            return -1;
        }
        if (memcmp(&input[idx], key, key_len) == 0) {
            idx += key_len;
            /* shortest ":x, or ":x} or ":x] */
            if ((idx + 4 >= input_len) ||
                (input[idx+1] != ':')) {
                return -1;
            }
            idx+=2;
            *value = (char *)&input[idx];
            for (;idx < input_len;idx++) {
                if ((input[idx] == ',') ||
                    (input[idx] == '}') ||
                    (input[idx] == ']')) {
                    *value_len = idx - (*value - input);
                    return 0;
                }
            }
        }
    }

    return -1;
}


void fibo_hmacsha256(int md_type, const UINT8 *api_secret, int key_len, const UINT8 *payload, int payload_len, UINT8 *encrypt_result, int len)
{
	int i, j= 0;
	mbedtls_md_context_t ctx;
	unsigned char hmac_hash1[HMAC_SHA1_LEN] = {'\0'};
	unsigned char hmac_hash256[HMAC_SHA256_LEN] = {'\0'};
	
	mbedtls_md_init(&ctx);
	mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type) , 1); //use hmac
	mbedtls_md_hmac_starts(&ctx, api_secret, key_len);
	mbedtls_md_hmac_update(&ctx, payload, payload_len);

	//convert bytes to hex string
	if(md_type == MBEDTLS_MD_SHA1)
	{
	   mbedtls_md_hmac_finish(&ctx, hmac_hash1);
		for(i=0; i < HMAC_SHA1_LEN; i++)
		{
		  OSI_PRINTFI("%02x", hmac_hash1[i]);
		  sprintf((char *)encrypt_result + (i + j), "%02x", hmac_hash1[i]);
		  j++;
		}

	}
	else if(MBEDTLS_MD_SHA256)
	{
	    mbedtls_md_hmac_finish(&ctx, hmac_hash256);
		for(i=0; i < HMAC_SHA256_LEN; i++)
		{
		  OSI_PRINTFI("%02x", hmac_hash256[i]);
		  sprintf((char *)encrypt_result + (i + j), "%02x", hmac_hash256[i]);
		  j++;
		}

	}
	mbedtls_md_free(&ctx); 
}

bool fibo_iotx_generate_sign_string(const char *device_id, const char *device_name, const char *product_key,
                               const char *device_secret, char *sign_string)
{
    char signsource[IOTX_ALIYUN_SIGN_SOURCE_MAXLEN] = {0};
    uint16_t signsource_len = 0;
	int hex_len = 0;

    signsource_len = SIGN_FMT_LEN + strlen(device_id) + strlen(device_name) + strlen(product_key) + strlen(
                                 TIMESTAMP_VALUE);
    if (signsource_len >= IOTX_ALIYUN_SIGN_SOURCE_MAXLEN) {
        return false;
    }

    memset(signsource, 0, IOTX_ALIYUN_SIGN_SOURCE_MAXLEN);
    memcpy(signsource, "clientId", strlen("clientId"));
    memcpy(signsource + strlen(signsource), device_id, strlen(device_id));
    memcpy(signsource + strlen(signsource), "deviceName", strlen("deviceName"));
    memcpy(signsource + strlen(signsource), device_name, strlen(device_name));
    memcpy(signsource + strlen(signsource), "productKey", strlen("productKey"));
    memcpy(signsource + strlen(signsource), product_key, strlen(product_key));
    memcpy(signsource + strlen(signsource), "timestamp", strlen("timestamp"));
    memcpy(signsource + strlen(signsource), TIMESTAMP_VALUE, strlen(TIMESTAMP_VALUE));

	fibo_hmacsha256(MBEDTLS_MD_SHA256, (UINT8 *)device_secret, strlen(device_secret), (UINT8 *)signsource, strlen(signsource),
		(UINT8 *)sign_string, hex_len);

    return true;
}

static bool fibo_aliyun_dynreg_sign_password(fibo_aliyun_device_info_st* device_info, iotx_sign_mqtt_t *signout, char *rand)
{
    char signsource[IOTX_ALIYUN_SIGN_SOURCE_MAXLEN] = {0};
    uint16_t signsource_len = 0;
    const char sign_fmt[] = "deviceName%sproductKey%srandom%s";
	int hex_len = 0;

	/* password */
    signsource_len = strlen(sign_fmt) + strlen(device_info->device_name) + 1 + strlen(device_info->product_key) + strlen(
                                 rand) + 1;
    if (signsource_len >= IOTX_ALIYUN_SIGN_SOURCE_MAXLEN) {
        return false;
    }
    memset(signsource, 0, signsource_len);
    memcpy(signsource, "deviceName", strlen("deviceName"));
    memcpy(signsource + strlen(signsource), device_info->device_name, strlen(device_info->device_name));
    memcpy(signsource + strlen(signsource), "productKey", strlen("productKey"));
    memcpy(signsource + strlen(signsource), device_info->product_key, strlen(device_info->product_key));
    memcpy(signsource + strlen(signsource), "random", strlen("random"));
    memcpy(signsource + strlen(signsource), rand, strlen(rand));

	fibo_hmacsha256(MBEDTLS_MD_SHA256, (UINT8 *) device_info->product_secret, strlen(device_info->product_secret), (UINT8 *)signsource, 
		strlen(signsource), (UINT8 *)(signout->password), hex_len);
	OSI_PRINTFI("[%s] dynamic register, password:%s", __func__, signout->password);

    return true;
}

static bool fibo_aliyun_dynreg_sign_clientid(fibo_aliyun_device_info_st* device_info, iotx_sign_mqtt_t *signout, char *rand)
{
    const char *clientid1 = "|authType=register,random=";
    const char *clientid2 = ",signmethod=hmacsha256,securemode=2|";
    uint32_t clientid_len = 0;

    clientid_len = strlen(device_info->product_key) + 1 + strlen(device_info->device_name) +
                   strlen(clientid1) + strlen(rand) + strlen(clientid2) + 1;
    if (clientid_len >= IOTX_ALIUN_MQTT_CLIENTID_MAXLEN) {
        return false;
    }
    memset(signout->clientid, 0, clientid_len);
    memcpy(signout->clientid, device_info->product_key, strlen(device_info->product_key));
    memcpy(signout->clientid + strlen(signout->clientid), ".", strlen("."));
    memcpy(signout->clientid + strlen(signout->clientid), device_info->device_name, strlen(device_info->device_name));
    memcpy(signout->clientid + strlen(signout->clientid), clientid1, strlen(clientid1));
    memcpy(signout->clientid + strlen(signout->clientid), rand, strlen(rand));
    memcpy(signout->clientid + strlen(signout->clientid), clientid2, strlen(clientid2));
	OSI_PRINTFI("[%s]%d clientid:%s", __func__, __LINE__, signout->clientid);
    return true;
}
void infra_hex2str(uint8_t *input, uint16_t input_len, char *output)
{
    char *zEncode = "0123456789ABCDEF";
    int i = 0, j = 0;

    for (i = 0; i < input_len; i++) {
        output[j++] = zEncode[(input[i] >> 4) & 0xf];
        output[j++] = zEncode[(input[i]) & 0xf];
    }
}

bool fibo_aliyun_dynamic_register(char* hostname, fibo_aliyun_device_info_st* device_info)
{
    uint32_t length = 0, rand_num = 0;
    char rand_str[9] = {0};
    iotx_sign_mqtt_t signout;
	INT32 ret = 0;
    memset(&signout, 0, sizeof(iotx_sign_mqtt_t));

	OSI_PRINTFI("[%s] enter!", __func__);
	if (hostname == NULL || device_info == NULL)
	{
		return false;
	}

	if ((strlen(device_info->product_key) == 0) || (strlen(device_info->device_name) == 0))
	{
		return false;
	}

    /* setup hostname */
    memcpy(signout.hostname, hostname, strlen(hostname));

    /* setup port */
    signout.port = 1883;

    /* setup username */
    length = strlen(device_info->device_name) + strlen(device_info->product_key) + 2;
    if (length >= IOTX_ALIYUN_MQTT_USERNAME_MAXLEN) {
        return false;
    }
	OSI_PRINTFI("[%s]%d, product_key:%s, product_secret:%s, device_name:%s", 
		__func__, __LINE__, device_info->product_key, device_info->product_secret, device_info->device_name);
    memset(signout.username, 0, IOTX_ALIYUN_MQTT_USERNAME_MAXLEN);
    memcpy(signout.username, device_info->device_name, strlen(device_info->device_name));
    memcpy(signout.username + strlen(signout.username), "&", strlen("&"));
    memcpy(signout.username + strlen(signout.username), device_info->product_key, strlen(device_info->product_key));

	OSI_PRINTFI("[%s]%d username:%s", __func__, __LINE__, signout.username);
	rand_num = rand();
    infra_hex2str((unsigned char *)&rand_num, 4, rand_str);
	/* setup password */
	fibo_aliyun_dynreg_sign_password(device_info, &signout, rand_str);
	/* setup clientid */
	fibo_aliyun_dynreg_sign_clientid(device_info, &signout, rand_str);
	/* connect to aliyun server */
	fibo_mqtt_set(signout.username, signout.password);
    fibi_mqtt_set_tls_ver(4);
	do {
		ret = fibo_mqtt_connect(signout.clientid, "iot-as-mqtt.cn-shanghai.aliyuncs.com", 443, 1, 40, 2);
		fibo_taskSleep(30000);
		OSI_PRINTFI("[%s]%d", __func__, __LINE__);
	} while (ret < 0);
	
	return true;
}

void fibo_aliyun_device_secret_parse(char* topic, char* message, uint32_t len, char* deviceSecret)
{
	char *device_secret = NULL;
    uint32_t device_secret_len = 0;
	int32_t res = 0;
	const char *asterisk = "**********************";

    if (memcmp(topic, "/ext/register", strlen("/ext/register"))) {
        return;
    }

    /* parse secret */
    res = infra_json_value((char *)message, len, "deviceSecret", strlen("deviceSecret"),
                           &device_secret, &device_secret_len);
    if (res == 0) {
        memcpy(deviceSecret, device_secret + 1, device_secret_len - 2);
        memcpy(device_secret + 1 + 5, asterisk, strlen(asterisk));
    }
	OSI_PRINTFI("[%s] device_secret:%s, device_secret:%s", __func__, deviceSecret, device_secret);
	return;
}

/* calc username, password and clientId */
static bool fibo_aliyun_get_mqtt_info(fibo_aliyun_device_info_st* device_info, fibo_aliyun_mqtt_info* mqtt_info)
{
	char clientId[IOTX_ALIUN_MQTT_CLIENTID_MAXLEN+1];
	char userName[IOTX_ALIYUN_MQTT_USERNAME_MAXLEN+1];
	char passWord[IOTX_ALIYUN_MQTT_PASSWORD_MAXLEN+1];
	char device_id[IOTX_ALIYUN_PRODUCT_KEY_MAXLEN +  IOTX_ALIYUN_DEVICE_NAME_MAXLEN + 1] = {0};
	bool result = false;
	uint32_t length = 0;
	
	if (device_info == NULL || mqtt_info == NULL)
	{
		OSI_PRINTFI("[%s]%d input param NULL", __func__, __LINE__);
		return false;
	}

	/* no device_name or no product_key */
	if ((strlen(device_info->device_name) == 0) || (strlen(device_info->product_key) == 0))
	{
		OSI_PRINTFI("[%s]%d device_name or product_key NULL", __func__, __LINE__);
		return false;
	}

	/* no device_secret and no product_secret */
	if ((strlen(device_info->device_secret) == 0) && (strlen(device_info->product_secret) == 0))
	{
		OSI_PRINTFI("[%s]%d device_secret and product_secret NULL", __func__, __LINE__);
		return false;
	}	
	OSI_PRINTFI("[%s]%d product_key:%s, device_name:%s, product_secret:%s", __func__, __LINE__, 
			device_info->product_key, device_info->device_name, device_info->product_secret);
	
	if (strlen(device_info->device_secret) == 0) // dynamic register, get device_secret
	{
		g_fibo_aliyun_dynamic_register_flag = true;
		memset(g_fibo_aliyun_device_secret, 0x0, sizeof(g_fibo_aliyun_device_secret));
		fibo_aliyun_dynamic_register("iot-as-mqtt.cn-shanghai.aliyuncs.com", device_info);
		fibo_sem_try_wait(g_lock_wait_devicesecret, 120000);	
		memcpy(device_info->device_secret, g_fibo_aliyun_device_secret, strlen(g_fibo_aliyun_device_secret));
		OSI_PRINTFI("[%s] device_secret:%s", __func__, g_fibo_aliyun_device_secret);
		if (strlen(device_info->device_secret) == 0)
		{
			OSI_PRINTFI("[%s] dynamic register failed!", __func__);
			g_fibo_aliyun_dynamic_register_flag = false;
			return false;
		}
	}

	memcpy(device_id, device_info->product_key, strlen(device_info->product_key));
    memcpy(device_id + strlen(device_id), ".", strlen("."));
    memcpy(device_id + strlen(device_id), device_info->device_name, strlen(device_info->device_name));
	/* clientID */
	memset(clientId, 0, sizeof(clientId));
    memcpy(clientId, device_id, strlen(device_id));
    memcpy(clientId + strlen(clientId), "|", 1);
    for (int i = 0; i < (sizeof(clientid_kv) / (sizeof(clientid_kv[0]))); i++) {
        if ((strlen(clientId) + strlen(clientid_kv[i][0]) + strlen(clientid_kv[i][1]) + 2) >=
            IOTX_ALIUN_MQTT_CLIENTID_MAXLEN) {
            return false;
        }

        memcpy(clientId + strlen(clientId), clientid_kv[i][0], strlen(clientid_kv[i][0]));
        memcpy(clientId + strlen(clientId), "=", 1);
        memcpy(clientId + strlen(clientId), clientid_kv[i][1], strlen(clientid_kv[i][1]));
        memcpy(clientId + strlen(clientId), ",", 1);
    }
	OSI_PRINTFI("[%s]%d", __func__, __LINE__);
    memcpy(clientId + strlen(clientId) - 1, "|", 1);

	/* usename */
	length = strlen(device_info->device_name) + strlen(device_info->product_key) + 2;
	if (length >= IOTX_ALIYUN_MQTT_USERNAME_MAXLEN) {
		return false;
	}
	memset(userName, 0, IOTX_ALIYUN_MQTT_USERNAME_MAXLEN);
	memcpy(userName, device_info->device_name, strlen(device_info->device_name));
	memcpy(userName + strlen(userName), "&", strlen("&"));
	memcpy(userName + strlen(userName), device_info->product_key, strlen(device_info->product_key));

	/* password */
	result = fibo_iotx_generate_sign_string(device_id,device_info->device_name,device_info->product_key,
				device_info->device_secret,passWord);
	if (result == false)
	{
		OSI_PRINTFI("[%s]%d get password failed", __func__, __LINE__);
	}

	memset(mqtt_info, 0x0, sizeof(fibo_aliyun_mqtt_info));
	memcpy(mqtt_info->client_id, clientId, strlen(clientId));
	memcpy(mqtt_info->user_name, userName, strlen(userName));
	memcpy(mqtt_info->password, passWord, strlen(passWord));

	return true;
}

static void create_mqtt_connect()
{
    run = false;
    connect_res_result = 0;
    OSI_PRINTFI("mqttapi thread create start");
    while (!quit)
    {
        fibo_taskSleep(1000);
    }
    OSI_PRINTFI("mqttapi thread create");
    fibo_thread_create(prvThreadEntry, "mqtt-thread", 1024 * 16, NULL, OSI_PRIORITY_NORMAL);
}

void user_signal_process(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    OSI_PRINTFI("mqttapi RECV SIGNAL:%d", sig);
    char *topic;
    int8_t qos;
    char *message;
    uint32_t len;
    int ret = 1;
    switch (sig)
    {
    case GAPP_SIG_CONNECT_RSP:
        connect_res_result = ret = va_arg(arg, int);
        OSI_PRINTFI("mqttapi connect :%s", ret ? "ok" : "fail");
		OSI_PRINTFI("connect_res_result:%d", connect_res_result);
        fibo_sem_signal(g_lock);
        break;
    case GAPP_SIG_CLOSE_RSP:
        ret = va_arg(arg, int);
        OSI_PRINTFI("mqttapi close :%s", ret ? "ok" : "fail");
		if (g_fibo_aliyun_dynamic_register_flag == false)
		{
			create_mqtt_connect();
		}
		else
		{
			OSI_PRINTFI("dynamic register mqtt closed!");
			g_fibo_aliyun_dynamic_register_flag = false;
		}
        break;
    case GAPP_SIG_SUB_RSP:
        ret = va_arg(arg, int);
        OSI_PRINTFI("mqttapi sub :%s", ret ? "ok" : "fail");
        fibo_sem_signal(g_lock);
        break;
    case GAPP_SIG_UNSUB_RSP:
        ret = va_arg(arg, int);
        OSI_PRINTFI("mqttapi unsub :%s", ret ? "ok" : "fail");
        fibo_sem_signal(g_lock);
        break;
    case GAPP_SIG_PUB_RSP:
        ret = va_arg(arg, int);
        OSI_PRINTFI("mqttapi pub :%s", ret ? "ok" : "fail");
        fibo_sem_signal(g_lock);
        break;
    case GAPP_SIG_INCOMING_DATA_RSP:
        //gapp_dispatch(GAPP_SIG_INCOMING_DATA_RSP, pub_msg->topic, pub_msg->qos, pub_msg->message, pub_msg->recv_len);
        topic = va_arg(arg, char *);
        qos = va_arg(arg, int);
        message = va_arg(arg, char *);
        len = va_arg(arg, uint32_t);
        OSI_PRINTFI("mqttapi recv message :topic:%s, qos=%d message=%s len=%d", topic, qos, message, len);
		char device_secret[64] = {0};
		if (memcmp(topic, "/ext/register", strlen("/ext/register")) == 0)
		{
			fibo_aliyun_device_secret_parse(topic, message, len, device_secret);
			memcpy(g_fibo_aliyun_device_secret, device_secret, strlen(device_secret));
			fibo_sem_signal(g_lock_wait_devicesecret);
		}
        break;
    default:
        break;
    }
}

static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = user_signal_process};


static void prvThreadEntry(void *param)
{
	int ret = 0;
	int test = 1;
	bool result = false;
	OSI_LOGI(0, "application thread enter, param 0x%x", param);
    //srand(100);
	fibo_aliyun_device_info_st* device_info = &g_fibo_aliyun_device_info;
	fibo_aliyun_mqtt_info mqtt_info = {0};
    for (int n = 0; n < 10; n++)
    {
        OSI_LOGI(0, "hello world %d", n);
        fibo_taskSleep(1000);
    }

	memset(device_info, 0x0, sizeof(fibo_aliyun_device_info_st));
	memcpy(device_info->product_key, "a1klgAXldch", strlen("a1klgAXldch"));
	memcpy(device_info->device_name, "867567040951111", strlen("867567040951111"));
	//memcpy(device_info.device_secret, "045b90593307f0ad7d4f2d62d63628d6", strlen("045b90593307f0ad7d4f2d62d63628d6"));
	memcpy(device_info->product_secret, "HPoiKreay5vmZEHJ", strlen("HPoiKreay5vmZEHJ"));

	OSI_PRINTFI("[%s]%d product_key:%s, device_name:%s, product_secret:%s", __func__, __LINE__, 
		device_info->product_key, device_info->device_name, device_info->product_secret);

	OSI_PRINTFI("mqttapi wait network");
    reg_info_t reg_info;
    quit = false;
    run = true;

    if (g_lock == 0)
    {
        g_lock = fibo_sem_new(0);
    }

	if (g_lock_wait_devicesecret == 0)
	{
		g_lock_wait_devicesecret = fibo_sem_new(0);
	}
	
    while (test)
    {
        fibo_getRegInfo(&reg_info, 0);
        fibo_taskSleep(1000);
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        if (reg_info.nStatus == 1)
        {
            test = 0;
            fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, NULL);
            fibo_taskSleep(1000);
            OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        }
    }
	OSI_PRINTFI("[%s]%d", __func__, __LINE__);
	result = fibo_aliyun_get_mqtt_info(device_info, &mqtt_info);
	if (result == false)
	{
		OSI_PRINTFI("[%s]%d get mqttinfo failed!", __func__, __LINE__);
		fibo_thread_delete();	
		return;
	}
    fibo_taskSleep(1 * 1000);
    OSI_PRINTFI("mqtt [%s-%d]", __FUNCTION__, __LINE__);
    fibo_mqtt_set(mqtt_info.user_name, mqtt_info.password);
    fibi_mqtt_set_tls_ver(4);

    do
    {
        ret = fibo_mqtt_connect(mqtt_info.client_id, "iot-as-mqtt.cn-shanghai.aliyuncs.com", 443, 1, 40, 2);
        fibo_taskSleep(2000);
    } while (ret < 0);

    OSI_PRINTFI("mqttapi connect finish");

    fibo_sem_wait(g_lock);
    OSI_PRINTFI("mqttapi start sub topic");
    if (connect_res_result)
    {
        ret = fibo_mqtt_sub("/a1klgAXldch/867567040951111/user/get", 1);
        if (ret < 0)
        {
            OSI_PRINTFI("mqttapi sub failed");
        }
    }
    else
    {
        run = false;
        OSI_PRINTFI("mqttapi connect res fail");
    }

    while (run)
    {
        OSI_PRINTFI("mqttapi running");
        fibo_taskSleep(2 * 1000);
    }
    quit = true;
    OSI_PRINTFI("mqttapi thread exit");
    create_mqtt_connect();
	
    char *pt = (char *)fibo_malloc(512);
    if (pt != NULL)
    {
        OSI_LOGI(0, "malloc address %u", (unsigned int)pt);
        fibo_free(pt);
    }

	test_printf();
    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    create_mqtt_connect();
    return &user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}

