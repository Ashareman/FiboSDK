/*****************************************************************
*   Copyright (C), 2021, Shenzhen Fibocom Wireless Inc
*                  All rights reserved
*
*   FileName    : fibocom_tcp_demo.c
*   Author      : Charles Shi
*   Created     : 2021-06-28
*   Description : tcp api demo
*****************************************************************/

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Y', 'A', 'P')


#include "fibo_opencpu.h"


#undef errno
#define errno fibo_get_socket_error()

typedef enum fibo_connection_status_s
{
    FIBO_STATUS_IDLE = 0,
    FIBO_STATUS_CONNECTTING,
    FIBO_STATUS_CONNECT_OK,
    FIBO_STATUS_CONNECT_FAIL,
    FIBO_STATUS_CONNECT_DISCONNECTTD
} fibo_connection_status_t;

typedef struct fibo_demo_connection_s
{
    int socket_id;
    fibo_connection_status_t status;
} fibo_demo_connection_t;

#define TCP_DEMO_LOG_INFO(format, ...)                                                               \
    do                                                                                               \
    {                                                                                                \
        OSI_PRINTFI("[tcp_demo][info][%s:%d]-" format " \n", __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0);

#define CHECK(condition, err_code, format, ...)       \
    do                                                \
    {                                                 \
        if (condition)                                \
        {                                             \
            TCP_DEMO_LOG_INFO(format, ##__VA_ARGS__); \
            ret = err_code;                           \
            goto error;                               \
        }                                             \
    } while (0);

unsigned long myhtonl(unsigned long n)
{
    return ((n & 0xff) << 24) | ((n & 0xff00) << 8) | ((n & 0xff0000) >> 8) | ((n & 0xff000000) >> 24);
}

unsigned short myhtons(unsigned short n)
{
    return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

int ip4addr_aton(const char *cp, ip4_addr_t *addr)
{
    u32_t val;
    u8_t base;
    char c;
    u32_t parts[4];
    u32_t *pp = parts;

    c = *cp;
    for (;;)
    {
        /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, 1-9=decimal.
     */
        if (!isdigit(c))
        {
            return 0;
        }
        val = 0;
        base = 10;
        if (c == '0')
        {
            c = *++cp;
            if (c == 'x' || c == 'X')
            {
                base = 16;
                c = *++cp;
            }
            else
            {
                base = 8;
            }
        }
        for (;;)
        {
            if (isdigit(c))
            {
                val = (val * base) + (u32_t)(c - '0');
                c = *++cp;
            }
            else if (base == 16 && isxdigit(c))
            {
                val = (val << 4) | (u32_t)(c + 10 - (islower(c) ? 'a' : 'A'));
                c = *++cp;
            }
            else
            {
                break;
            }
        }
        if (c == '.')
        {
            /*
       * Internet format:
       *  a.b.c.d
       *  a.b.c   (with c treated as 16 bits)
       *  a.b (with b treated as 24 bits)
       */
            if (pp >= parts + 3)
            {
                return 0;
            }
            *pp++ = val;
            c = *++cp;
        }
        else
        {
            break;
        }
    }
    /*
   * Check for trailing characters.
   */
    if (c != '\0' && !isspace(c))
    {
        return 0;
    }
    /*
   * Concoct the address according to
   * the number of parts specified.
   */
    switch (pp - parts + 1)
    {

    case 0:
        return 0; /* initial nondigit */

    case 1: /* a -- 32 bits */
        break;

    case 2: /* a.b -- 8.24 bits */
        if (val > 0xffffffUL)
        {
            return 0;
        }
        if (parts[0] > 0xff)
        {
            return 0;
        }
        val |= parts[0] << 24;
        break;

    case 3: /* a.b.c -- 8.8.16 bits */
        if (val > 0xffff)
        {
            return 0;
        }
        if ((parts[0] > 0xff) || (parts[1] > 0xff))
        {
            return 0;
        }
        val |= (parts[0] << 24) | (parts[1] << 16);
        break;

    case 4: /* a.b.c.d -- 8.8.8.8 bits */
        if (val > 0xff)
        {
            return 0;
        }
        if ((parts[0] > 0xff) || (parts[1] > 0xff) || (parts[2] > 0xff))
        {
            return 0;
        }
        val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
        break;
    default:
        break;
    }
    if (addr)
    {
        ip4_addr_set_u32(addr, val);
    }
    return 1;
}

fibo_demo_connection_t g_client = {-1, FIBO_STATUS_IDLE};

static void fibo_tcp_demo_close(void)
{
    if (0 < g_client.socket_id)
    {
        fibo_sock_close(g_client.socket_id);
        g_client.socket_id = -1;
    }

    g_client.status = FIBO_STATUS_CONNECT_DISCONNECTTD;
}

static INT32 fibo_tcp_demo_connect(void)
{
    INT32 ret = -1;
    INT32 socketid = -1;
    int flags = -1;
    GAPP_TCPIP_ADDR_T addr;
    fibo_demo_connection_t *p_client = &g_client;

    do
    {
        memset(&addr, 0, sizeof(addr));

        //创建socket
        socketid = fibo_sock_create(GAPP_IPPROTO_TCP);
        if (0 > socketid)
        {
            TCP_DEMO_LOG_INFO("createc socket failed!");
            break;
        }

        p_client->socket_id = socketid;

        //设置为非阻塞
        flags = fibo_sock_lwip_fcntl(socketid, F_GETFL, 0);
        if (0 == (flags | 0x4000))
        {
            flags |= O_NONBLOCK;
            ret = fibo_sock_lwip_fcntl(socketid, F_SETFL, flags);
            if (0 > ret)
            {
                TCP_DEMO_LOG_INFO("set socket NONBLOCK failed!");
                fibo_tcp_demo_close();
                break;
            }
        }

        //连接服务器
        p_client->status = FIBO_STATUS_CONNECTTING;
        TCP_DEMO_LOG_INFO("connecting sevrer...");

        addr.sin_port = myhtons(3410);
        ip4addr_aton("47.110.234.36", &addr.sin_addr.u_addr.ip4);
        addr.sin_addr.u_addr.ip4.addr = myhtonl(addr.sin_addr.u_addr.ip4.addr);
        ret = fibo_sock_connect(socketid, &addr);
        if ((0 > ret) && (EINPROGRESS != errno))
        {
            g_client.status = FIBO_STATUS_CONNECT_FAIL;
            TCP_DEMO_LOG_INFO("connect sevrer failed!");
            fibo_tcp_demo_close();
            break;
        }
        else
        {
            g_client.status = FIBO_STATUS_CONNECT_OK;
            TCP_DEMO_LOG_INFO("connect sevrer success!");
        }
    }while (0);

    return ret;
}

static void fibo_tcp_demo_recv_data_handle(char *p_buf, INT32 buf_len)
{
    TCP_DEMO_LOG_INFO("recv data: %s, buf_len: %d", p_buf, buf_len);
}

static void fibo_tcp_demo_recv(void)
{
    INT32 len = 0;
    char recv_buf[1024] = {0};
    fibo_demo_connection_t *p_client = &g_client;

    while (1)
    {
        memset(recv_buf, 0, sizeof(recv_buf));
        while ((0 > (len =fibo_sock_recv(p_client->socket_id, (UINT8 *)recv_buf, sizeof(recv_buf)))
                && (errno == EINTR)));

        if (0 < len)
        {
            fibo_tcp_demo_recv_data_handle(recv_buf, len);
        }
        else if (0 > len)
        {
            if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
            {
                TCP_DEMO_LOG_INFO("recv finish.");
                break;
            }
            else
            {
                TCP_DEMO_LOG_INFO("read failed, close tcp connection:%s", strerror(errno));
                fibo_tcp_demo_close();
            }

            break;
        }
        else
        {
            TCP_DEMO_LOG_INFO("socket has been disconnected!");
            fibo_tcp_demo_close();
            break;
        }
    }

}

static void fibo_tcp_demo_loop(void)
{
    INT32 ret = -1;
    fd_set rfds, wfds;
    struct timeval tv = {0};
    fibo_demo_connection_t *p_client = &g_client;

    while ((FIBO_STATUS_CONNECTTING ==p_client->status) || (FIBO_STATUS_CONNECT_OK ==p_client->status))
    {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);

        if (FIBO_STATUS_CONNECT_OK ==p_client->status)
        {
            FD_SET(p_client->socket_id, &rfds);
        }

        FD_SET(p_client->socket_id, &wfds);

        while ((0 > fibo_sock_lwip_select(p_client->socket_id + 1, &rfds, &wfds, NULL, NULL))
               && (errno == EINTR));

        //读取数据
        if (FD_ISSET(p_client->socket_id, &rfds))
        {
            fibo_tcp_demo_recv();
        }

        //发送数据
        if (FD_ISSET(p_client->socket_id, &wfds))
        {
            if (FIBO_STATUS_CONNECTTING == p_client->status)
            {
                int err;
                INT32 length = sizeof(err);
                ret = fibo_sock_getOpt(p_client->socket_id, SOL_SOCKET, SO_ERROR, (char *)&err, &length);
                if ((0 == ret) && (ECONNREFUSED != err) && (ETIMEDOUT != err))
                {
                    p_client->status = FIBO_STATUS_CONNECT_OK;
                    TCP_DEMO_LOG_INFO("connect sevrer success!");
                }
                else
                {
                    p_client->status = FIBO_STATUS_CONNECT_FAIL;
                    TCP_DEMO_LOG_INFO("connect sevrer failed!");
                    fibo_tcp_demo_close();
                }
            }

            if (FIBO_STATUS_CONNECT_OK == p_client->status)
            {
                int len = -1;
                char buf[128] = {"Hello! Fibocom"};
                while (((len = fibo_sock_send(p_client->socket_id, (UINT8 *)buf, strlen(buf))) == -1)
                        && (errno == EINTR));

                if ((-1 == len) && (errno == EAGAIN || errno == EWOULDBLOCK))
                {
                    TCP_DEMO_LOG_INFO("send again.");
                    continue;
                }

                if (0 >= len)
                {
                    TCP_DEMO_LOG_INFO("send data failed!");
                    fibo_tcp_demo_close();
                }
            }
        }

        fibo_taskSleep(10);
    }
}

/*
* 拨号上网
*/
static INT32 fibo_tcp_demo_dun(void)
{
    INT32 ret = -1;
    reg_info_t reg_info;

    memset(&reg_info, 0, sizeof(reg_info_t));

    while (1)
    {
        fibo_getRegInfo(&reg_info, 0);
        fibo_taskSleep(1000);

        if (1 == reg_info.nStatus)
        {
            ret = fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, NULL);
            fibo_taskSleep(1000);

            break;
        }
    }

    TCP_DEMO_LOG_INFO("fibo modem dial-up %s!", (0 <= ret) ? "success" : "failed");
    return ret;
}

void main_task(void *arg)
{
    INT32 ret = -1;

    ret = fibo_tcp_demo_dun();
    CHECK((0 > ret), ret, "tcp dun failed: %d", ret);

    ret = fibo_tcp_demo_connect();
    CHECK((0 > ret), ret, "connect server failed: %d", ret);

    //自建轮询
    fibo_tcp_demo_loop();

error:
    fibo_thread_delete();
}

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    fibo_thread_create(main_task, "fibo_tcp_demo", 1024 * 4, NULL, OSI_PRIORITY_NORMAL);

    return NULL;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
