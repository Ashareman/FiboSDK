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

// #include <socket.h>
// #include <sys/types.h>
// #include <sys/time.h>
// #include <netdb.h>
// #include <stdio.h>
// #include <unistd.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <stdbool.h>

#include <stdlib.h>
#include <string.h>

#define FIBO_DEMO_R_FAILED -1
#define FIBO_DEMO_R_SUCCESS 0

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
    int sock;
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

static int connect_timeout(fibo_demo_connection_t *client, struct sockaddr *addr, int addr_len, int timeout)
{
    int ret = FIBO_DEMO_R_SUCCESS;
    struct timeval tm = {0};
    fd_set wset;
    FD_ZERO(&wset);
    FD_SET(client->sock, &wset);

    tm.tv_sec = timeout / 1000;
    tm.tv_usec = (timeout % 1000) * 1000;

    ret = connect(client->sock, addr, addr_len);
    if (ret != FIBO_DEMO_R_SUCCESS || errno == EINPROGRESS)
    {
        if (errno != EINPROGRESS)
        {
            TCP_DEMO_LOG_INFO("connect failed:%s", strerror(errno));
            ret = FIBO_DEMO_R_FAILED;
        }
        else
        {
            ret = select(client->sock + 1, NULL, &wset, NULL, timeout > 0 ? &tm : NULL);
            if (ret < 0)
            {
                TCP_DEMO_LOG_INFO("select failed:%s", strerror(errno));
                ret = FIBO_DEMO_R_FAILED;
            }
            else if (ret == 0)
            {
                TCP_DEMO_LOG_INFO("connect time out");
                ret = FIBO_DEMO_R_FAILED;
            }
            else if (ret > 0)
            {
                if (!FD_ISSET(client->sock, &wset))
                {
                    TCP_DEMO_LOG_INFO("connect error when select:%s", strerror(errno));
                    ret = FIBO_DEMO_R_FAILED;
                }
                else
                {
                    ret = FIBO_DEMO_R_SUCCESS;
                }
            }
        }
    }
    else
    {
        TCP_DEMO_LOG_INFO("tcp connect directly");
    }

    return ret;
}

/**
* @brief 发送数据，如果发送失败，则需要关闭socket
* 
* @param c 
* @param data 
* @param len 
* @return int 
 */
int fibo_demo_send(fibo_demo_connection_t *c, const void *data, int len)
{
    int ret = FIBO_DEMO_R_SUCCESS;
    uint32_t sent = 0;
    struct timeval tm = {0};
    fd_set wset;

    const uint8_t *send_data = (const uint8_t *)data;
    while (sent < len)
    {
        ret = (int)write(c->sock, &send_data[sent], len - sent);
        if (ret < 0)
        {
            if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
            {
                TCP_DEMO_LOG_INFO("wait send buffer ava");
                FD_ZERO(&wset);
                FD_SET(c->sock, &wset);
                tm.tv_sec = 1;
                tm.tv_usec = 0;
                select(c->sock + 1, NULL, &wset, NULL, &tm);
            }
            else
            {
                TCP_DEMO_LOG_INFO("send data failed:%s send=%d len=%u", strerror(errno), (int)(len - sent), len);
                ret = FIBO_DEMO_R_FAILED;
                break;
            }
        }
        else
        {
            sent += (uint32_t)ret;
            TCP_DEMO_LOG_INFO("try send:send=%d", sent);
        }
    }

    return ret;
}

/**
* @brief 创建一个TCP连接
* 
* @param client 连接的句柄
* @param hostname 连接的域名或者IP地址
* @param port 连接端口
* @param family 
*        AF_INET 表示IPV4
*        AF_INET6 表示IPV6
*        AF_UNSPEC 表示不指定，有系统决定。
*        如果没有特殊要求的话，可以直接填写AF_UNSPEC
* @param proto 
*        SOCK_STREAM 表示TCP
*        SOCK_DGRAM  表示UDP 
* @param timeout 连接的超时时间，单位毫秒
* @return int 
 */
int fibo_demo_connect(fibo_demo_connection_t *client, const char *hostname, uint16_t port, int family, int proto, int timeout)
{
    int ret = FIBO_DEMO_R_SUCCESS;
    char service[6] = {0};
    int non_block = 1;
    struct addrinfo *peer_addr = NULL;
    struct addrinfo hints = {AI_PASSIVE, family, proto, proto == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP, 0, NULL, NULL, NULL};

    client->sock = -1;

    //如果是域名，这里有DNS解析的过程
    snprintf(service, sizeof(service), "%u", port);
    ret = getaddrinfo(hostname, service, &hints, &peer_addr);
    CHECK(ret != FIBO_DEMO_R_SUCCESS, FIBO_DEMO_R_FAILED, "get addr info failed: (%s)", hostname);

    for (struct addrinfo *cur = peer_addr; cur != NULL; cur = cur->ai_next)
    {
        client->sock = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
        if (client->sock < 0)
        {
            TCP_DEMO_LOG_INFO("create socket failed:%s", strerror(errno));
            ret = FIBO_DEMO_R_FAILED;
            continue;
        }

        //设置为非阻塞
        ret = ioctl(client->sock, FIONBIO, &non_block);
        CHECK(ret != 0, FIBO_DEMO_R_FAILED, "set non block fail", ret);

        TCP_DEMO_LOG_INFO("connect with timeout timeout=%d", timeout);
        ret = connect_timeout(client, (struct sockaddr *)cur->ai_addr, cur->ai_addrlen, timeout);

        if (ret == FIBO_DEMO_R_SUCCESS)
        {
            TCP_DEMO_LOG_INFO("racoon connect ok");
            break;
        }
        close(client->sock);
        client->sock = -1;
        TCP_DEMO_LOG_INFO("try tcp connect failed:%s", strerror(errno));
        ret = FIBO_DEMO_R_FAILED;
    }

    CHECK(ret != FIBO_DEMO_R_SUCCESS, FIBO_DEMO_R_FAILED, "connect host(%s) failed: %s", hostname, strerror(errno));
    client->status = FIBO_STATUS_CONNECT_OK;
    freeaddrinfo(peer_addr);

    //这里可以发送事件或者信号量，告诉其他任务连接已经建立成功
    //send event

    return ret;

error:
    if (peer_addr != NULL)
    {
        freeaddrinfo(peer_addr);
    }
    if (client->sock > 0)
    {
        close(client->sock);
    }

    client->status = FIBO_STATUS_CONNECT_FAIL;
    //这里可以发送事件或者信号量，告诉其他任务连接建立失败
    //send event

    return ret;
}

static int init_connect_client(fibo_demo_connection_t *c)
{
    memset(c, 0, sizeof(*c));
    c->status = FIBO_STATUS_IDLE;

    return FIBO_DEMO_R_SUCCESS;
}

static void recv_task(void *param)
{
    int ret = FIBO_DEMO_R_SUCCESS;
    int iret = FIBO_DEMO_R_SUCCESS;
    fibo_demo_connection_t *c = (fibo_demo_connection_t *)param;

    //接收线程tick事件。可以用来周期性做其他事情。
    int loop_period = 1000;

    struct timeval tv;
    fd_set read_fds;
    int fd = c->sock;

    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);

    tv.tv_sec = loop_period / 1000;
    tv.tv_usec = (loop_period % 1000) * 1000;
    TCP_DEMO_LOG_INFO("read enter...");

    char recv_buf[1024];

    while (c->status == FIBO_STATUS_CONNECT_OK && ret == FIBO_DEMO_R_SUCCESS)
    {
        iret = select(fd + 1, &read_fds, NULL, NULL, &tv);
        if (iret > 0)
        {
            iret = recv(fd, recv_buf, sizeof(recv_buf), 0);
            if (iret > 0)
            {
                TCP_DEMO_LOG_INFO("socket have read (%u) bytes", (unsigned int)iret);
                //处理数据,回应数据
            }
            else if (iret == 0)
            {
                //TCP已经正常关闭，收到了FIN包或者发送了FIN包
                ret = FIBO_DEMO_R_FAILED;
                TCP_DEMO_LOG_INFO("socket have closed noraml");
            }
            else if ((EAGAIN == errno) || (EWOULDBLOCK == errno))
            {
                //缓冲区数据接收完毕，继续等待数据
                TCP_DEMO_LOG_INFO("recv finish fd=%d", fd);
            }
            else
            {
                //出错，关闭tcp连接。
                ret = FIBO_DEMO_R_FAILED;
                TCP_DEMO_LOG_INFO("read failed, close tcp connection:%s", strerror(errno));
            }
        }
        else if (iret == 0)
        {
            TCP_DEMO_LOG_INFO("wait for network data...");
        }
        else
        {
            TCP_DEMO_LOG_INFO("select failed:%s", strerror(errno));
            ret = FIBO_DEMO_R_FAILED;
        }
    }

    c->status = FIBO_STATUS_CONNECT_DISCONNECTTD;
    close(fd);
    TCP_DEMO_LOG_INFO("tcp connect close");

    fibo_thread_delete();
}

void main_task(void *arg)
{
    int test = 1;
    TCP_DEMO_LOG_INFO("TCP wait network");
    reg_info_t reg_info;
    while (test)
    {
        fibo_getRegInfo(&reg_info, 0);
        fibo_taskSleep(1000);
        TCP_DEMO_LOG_INFO("[%s-%d]", __FUNCTION__, __LINE__);
        if (reg_info.nStatus == 1)
        {
            test = 0;
            fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, NULL);
            fibo_taskSleep(1000);
            TCP_DEMO_LOG_INFO("[%s-%d]", __FUNCTION__, __LINE__);
        }
    }

    fibo_demo_connection_t g_client = {0};

    init_connect_client(&g_client);

    //创建一个TCP连接，超时时间为5s
    fibo_demo_connection_t *c = &g_client;
    c->status = FIBO_STATUS_CONNECTTING;
    int ret = fibo_demo_connect(c, "www.gov.cn", 80, AF_INET, SOCK_STREAM, 5000);
    CHECK(ret != FIBO_DEMO_R_SUCCESS, ret, "connect fail:%d", ret);

    //发送数据
    fibo_demo_send(c, "GET /\r\n\r\n", sizeof("GET /\r\n\r\n") - 1);

    //读取数据
    fibo_thread_create(recv_task, "tcp-recv", 1024 * 16, c, OSI_PRIORITY_NORMAL);

error:
    fibo_thread_delete();
}

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    fibo_thread_create(main_task, "tcp-connect", 1024 * 4, NULL, OSI_PRIORITY_NORMAL);

    return NULL;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
