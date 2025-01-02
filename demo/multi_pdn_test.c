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


#define in_range(c, lo, up)  ((u8_t)c >= lo && (u8_t)c <= up)
#define isprint(c)           in_range(c, 0x20, 0x7f)
#define isdigit(c)           in_range(c, '0', '9')
#define isxdigit(c)          (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#define islower(c)           in_range(c, 'a', 'z')
#define isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
#define xchar(i)             ((i) < 10 ? '0' + (i) : 'A' + (i) - 10)

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}
unsigned long htonl(unsigned long n)
{
    return ((n & 0xff) << 24) | ((n & 0xff00) << 8) | ((n & 0xff0000) >> 8) | ((n & 0xff000000) >> 24);
}

unsigned short htons(unsigned short n)
{
    return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}


/**
 * Check whether "cp" is a valid ascii representation
 * of an IPv6 address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 *
 * @param cp IPv6 address in ascii representation (e.g. "FF01::1")
 * @param addr pointer to which to save the ip address in network order
 * @return 1 if cp could be converted to addr, 0 on failure
 */
int
ip6addr_aton(const char *cp, ip6_addr_t *addr)
{
  u32_t addr_index, zero_blocks, current_block_index, current_block_value;
  const char *s;

  /* Count the number of colons, to count the number of blocks in a "::" sequence
     zero_blocks may be 1 even if there are no :: sequences */
  zero_blocks = 8;
  for (s = cp; *s != 0; s++) {
    if (*s == ':') {
      zero_blocks--;
    } else if (!isxdigit(*s)) {
      break;
    }
  }

  /* parse each block */
  addr_index = 0;
  current_block_index = 0;
  current_block_value = 0;
  for (s = cp; *s != 0; s++) {
    if (*s == ':') {
      if (addr) {
        if (current_block_index & 0x1) {
          addr->addr[addr_index++] |= current_block_value;
        }
        else {
          addr->addr[addr_index] = current_block_value << 16;
        }
      }
      current_block_index++;
      current_block_value = 0;
      if (current_block_index > 7) {
        /* address too long! */
        return 0;
      }
      if (s[1] == ':') {
        if (s[2] == ':') {
          /* invalid format: three successive colons */
          return 0;
        }
        s++;
        /* "::" found, set zeros */
        while (zero_blocks > 0) {
          zero_blocks--;
          if (current_block_index & 0x1) {
            addr_index++;
          } else {
            if (addr) {
              addr->addr[addr_index] = 0;
            }
          }
          current_block_index++;
          if (current_block_index > 7) {
            /* address too long! */
            return 0;
          }
        }
      }
    } else if (isxdigit(*s)) {
      /* add current digit */
      current_block_value = (current_block_value << 4) +
          (isdigit(*s) ? (u32_t)(*s - '0') :
          (u32_t)(10 + (islower(*s) ? *s - 'a' : *s - 'A')));
    } else {
      /* unexpected digit, space? CRLF? */
      break;
    }
  }

  if (addr) {
    if (current_block_index & 0x1) {
      addr->addr[addr_index++] |= current_block_value;
    }
    else {
      addr->addr[addr_index] = current_block_value << 16;
    }
  }

  /* convert to network byte order. */
  if (addr) {
    for (addr_index = 0; addr_index < 4; addr_index++) {
      addr->addr[addr_index] = lwip_htonl(addr->addr[addr_index]);
    }
  }

  if (current_block_index != 7) {
    return 0;
  }

  return 1;
}

/**
 * Same as ipaddr_ntoa, but reentrant since a user-supplied buffer is used.
 *
 * @param addr ip6 address in network order to convert
 * @param buf target buffer where the string is stored
 * @param buflen length of buf
 * @return either pointer to buf which now holds the ASCII
 *         representation of addr or NULL if buf was too small
 */
char *
ip6addr_ntoa_r(const ip6_addr_t *addr, char *buf, int buflen)
{
  u32_t current_block_index, current_block_value, next_block_value;
  s32_t i;
  u8_t zero_flag, empty_block_flag;

  i = 0;
  empty_block_flag = 0; /* used to indicate a zero chain for "::' */

  for (current_block_index = 0; current_block_index < 8; current_block_index++) {
    /* get the current 16-bit block */
    current_block_value = lwip_htonl(addr->addr[current_block_index >> 1]);
    if ((current_block_index & 0x1) == 0) {
      current_block_value = current_block_value >> 16;
    }
    current_block_value &= 0xffff;

    /* Check for empty block. */
    if (current_block_value == 0) {
      if (current_block_index == 7 && empty_block_flag == 1) {
        /* special case, we must render a ':' for the last block. */
        buf[i++] = ':';
        if (i >= buflen) {
          return NULL;
        }
        break;
      }
      if (empty_block_flag == 0) {
        /* generate empty block "::", but only if more than one contiguous zero block,
         * according to current formatting suggestions RFC 5952. */
        next_block_value = lwip_htonl(addr->addr[(current_block_index + 1) >> 1]);
        if ((current_block_index & 0x1) == 0x01) {
            next_block_value = next_block_value >> 16;
        }
        next_block_value &= 0xffff;
        if (next_block_value == 0) {
          empty_block_flag = 1;
          buf[i++] = ':';
          if (i >= buflen) {
            return NULL;
          }
          continue; /* move on to next block. */
        }
      } else if (empty_block_flag == 1) {
        /* move on to next block. */
        continue;
      }
    } else if (empty_block_flag == 1) {
      /* Set this flag value so we don't produce multiple empty blocks. */
      empty_block_flag = 2;
    }

    if (current_block_index > 0) {
      buf[i++] = ':';
      if (i >= buflen) {
        return NULL;
      }
    }

    if ((current_block_value & 0xf000) == 0) {
      zero_flag = 1;
    } else {
      buf[i++] = xchar(((current_block_value & 0xf000) >> 12));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    if (((current_block_value & 0xf00) == 0) && (zero_flag)) {
      /* do nothing */
    } else {
      buf[i++] = xchar(((current_block_value & 0xf00) >> 8));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    if (((current_block_value & 0xf0) == 0) && (zero_flag)) {
      /* do nothing */
    }
    else {
      buf[i++] = xchar(((current_block_value & 0xf0) >> 4));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    buf[i++] = xchar((current_block_value & 0xf));
    if (i >= buflen) {
      return NULL;
    }
  }

  buf[i] = 0;

  return buf;
}

/**
 * Convert numeric IPv6 address into ASCII representation.
 * returns ptr to static buffer; not reentrant!
 *
 * @param addr ip6 address in network order to convert
 * @return pointer to a global static (!) buffer that holds the ASCII
 *         representation of addr
 */
char *
ip6addr_ntoa(const ip6_addr_t *addr)
{
  static char str[40];
  return ip6addr_ntoa_r(addr, str, 40);
}

UINT32 g_pdp_active_sem;
bool   test_pdp_active_flag;

UINT32 g_pdp_deactive_sem;
bool   test_pdp_deactive_flag;

UINT32 g_pdp_asyn_active_sem;
bool   test_pdp_asyn_active_flag;

UINT32 g_pdp_asyn_deactive_sem;
bool   test_pdp_asyn_deactive_flag;

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
}
static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);

    fibo_set_dualsim(1);
    fibo_taskSleep(30*1000);
	CFW_SIM_ID simid = fibo_get_dualsim();

    OSI_PRINTFI("[%s-%d] simid = %d", __FUNCTION__, __LINE__,simid);
	int ret = -1;
	ret = fibo_multi_pdn_cfg(FIBO_E_LBS, 3, 0);
	OSI_LOGI(0, "ret = %d", ret);
	uint8_t ip_type;
	struct sockaddr_in ip_addr, ip_addr1;
	struct sockaddr_in6 ip6_addr, ip6_addr1;
#if 1
	//while(1)
	{
		ip_type = 0;
		memset(&ip_addr, 0, sizeof(ip_addr));
        memset(&ip_addr1, 0, sizeof(ip_addr1));
		memset(&ip6_addr, 0, sizeof(ip6_addr));
		fibo_taskSleep(10*1000);
		{
			pdp_profile_t pdp_profile;
			char *pdp_type = "IP";
			char *apn = "CMNET";
			memset(&pdp_profile, 0, sizeof(pdp_profile));

			pdp_profile.cid = 2;

			memcpy(pdp_profile.nPdpType, pdp_type, strlen((char *)pdp_type));
			memcpy(pdp_profile.apn, apn, strlen((char *)apn));
			ret = fibo_asyn_PDPActive(1, &pdp_profile, simid);

			g_pdp_asyn_active_sem = fibo_sem_new(0);
			test_pdp_asyn_active_flag =1;
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
			fibo_sem_wait(g_pdp_asyn_active_sem);
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
			fibo_sem_free(g_pdp_asyn_active_sem);
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
			fibo_taskSleep(50);
			ret = fibo_get_ip_info_by_cid(2, &ip_type, &ip_addr, &ip6_addr);
			if(0 == ret)
			{
				OSI_LOGI(0, "ip_type = %d", ip_type);
			}
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
   		}
		{
			pdp_profile_t pdp_profile;
			char *pdp_type = "IP";
			char *apn = "INTERNET";
			memset(&pdp_profile, 0, sizeof(pdp_profile));

			pdp_profile.cid = 3;

			memcpy(pdp_profile.nPdpType, pdp_type, strlen((char *)pdp_type));
			memcpy(pdp_profile.apn, apn, strlen((char *)apn));
			ret = fibo_asyn_PDPActive(1, &pdp_profile, simid);

			g_pdp_asyn_active_sem = fibo_sem_new(0);
			test_pdp_asyn_active_flag =1;
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
			fibo_sem_wait(g_pdp_asyn_active_sem);
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
			fibo_sem_free(g_pdp_asyn_active_sem);
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
			fibo_taskSleep(50);
			ret = fibo_get_ip_info_by_cid(3, &ip_type, &ip_addr1, &ip6_addr);
			if(0 == ret)
			{
				OSI_LOGI(0, "ip_type = %d", ip_type);
			}
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
	}
	
	GAPP_TCPIP_ADDR_T addr;
    INT8 socketid1, socketid2;
    int rcvnum = 0;
    char rcvbuf[128];
    int opt = 1;
    GAPP_TCPIP_ADDR_T bind_addr, bind_addr1;
	char *sendbuf = "opencpu test udp ok";

    memset(rcvbuf, 0, sizeof(rcvbuf));
    memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
    OSI_PRINTFI("[%s-%d]sys_sock_create start :\n", __FUNCTION__, __LINE__);
    //socketid1 = fibo_sock_create_ex(AF_INET,SOCK_STREAM,IPPROTO_IP);
	//OSI_PRINTFI("[%s-%d]sys_sock_create  retcode = %d\n", __FUNCTION__, __LINE__, socketid1);
#if 1
    socketid1 = fibo_sock_create(GAPP_IPPROTO_TCP);
    OSI_PRINTFI("[%s-%d]sys_sock_create  ret = %d\n", __FUNCTION__, __LINE__, socketid1);

    bind_addr.sin_addr.u_addr.ip4.addr = ip_addr.sin_addr.s_addr;
    ret = fibo_sock_bind(socketid1, &bind_addr);
    OSI_PRINTFI("[%s-%d] fibo_sock_bind  ret = %d\n", __FUNCTION__, __LINE__, ret);

    addr.sin_port = htons(3410);
    ret = fibo_sock_setOpt(socketid1, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(int));
    OSI_PRINTFI("[%s-%d]setopt  ret = %d\n", __FUNCTION__, __LINE__, ret);

    ip4addr_aton("47.110.234.36", &addr.sin_addr.u_addr.ip4);
    ret = fibo_sock_connect(socketid1, &addr);
    OSI_PRINTFI("[%s-%d] sys_sock_connect addr.sin_addr.addr = %u\n", __FUNCTION__, __LINE__, addr.sin_addr.u_addr.ip4.addr);
    OSI_PRINTFI("[%s-%d] sys_sock_create  retcode = %d\n", __FUNCTION__, __LINE__, ret);
#endif
#if 0
    socketid2 = fibo_sock_create(GAPP_IPPROTO_UDP);
    OSI_PRINTFI("[%s-%d] socketid = %d", __FUNCTION__, __LINE__, socketid2);

    bind_addr1.sin_addr.u_addr.ip4.addr = ip_addr1.sin_addr.s_addr;
    ret = fibo_sock_bind(socketid2, &bind_addr1);
    OSI_PRINTFI("[%s-%d] fibo_sock_bind  ret = %d\n", __FUNCTION__, __LINE__, ret);

    addr.sin_port = htons(5105);
    ip4addr_aton("47.110.234.36", &addr.sin_addr.u_addr.ip4);
    //addr.sin_addr.u_addr.ip4.addr = htonl(addr.sin_addr.u_addr.ip4.addr);
    ret = fibo_sock_connect(socketid2, &addr);
    OSI_PRINTFI("[%s-%d] sys_sock_connect addr.sin_addr.addr = %u\n", __FUNCTION__, __LINE__, addr.sin_addr.u_addr.ip4.addr);
#endif
    while(1)
	{
#if 1
		ret = fibo_sock_send(socketid1, (UINT8 *)"demo opencpu test tcp ok", strlen("demo opencpu test tcp ok"));
		OSI_PRINTFI("[%s-%d] sys_sock_send  ret = %d\n", __FUNCTION__, __LINE__, ret);
		fibo_taskSleep((UINT32)1000);
		rcvnum = fibo_sock_recv(socketid1, (UINT8 *)rcvbuf, 64);
		OSI_PRINTFI("[%s-%d] sys_sock_recv  rcvnum = %d, rcvbuf = %s\n", __FUNCTION__, __LINE__, rcvnum, rcvbuf);
		fibo_taskSleep((UINT32)10*1000);
#endif

        fibo_lbs_get_gisinfo(6);
        
		fibo_taskSleep((UINT32)5*1000);
        
#if 0
        ret = fibo_sock_send(socketid2, (UINT8 *)sendbuf, strlen(sendbuf));
        OSI_PRINTFI("[%s-%d] sys_sock_send ret = %d", __FUNCTION__, __LINE__, ret);
        memset(rcvbuf, 0, sizeof(rcvbuf));
        rcvnum = fibo_sock_recv(socketid2, (UINT8 *)rcvbuf, 64);
        OSI_PRINTFI("[%s-%d] fibo_sock_recv rcvnum = %d, rcvbuf = %s\n", __FUNCTION__, __LINE__, rcvnum, rcvbuf);
        fibo_taskSleep((UINT32)5*1000);
#endif
	}
#endif

#if 0
	{
    	memset(&ip6_addr, 0, sizeof(ip6_addr));
        memset(&ip6_addr1, 0, sizeof(ip6_addr1));
	    {
    		pdp_profile_t pdp_profile;
    		char *pdp_type = "IPV4V6";
    		char *apn = "CMNET";
    		memset(&pdp_profile, 0, sizeof(pdp_profile));
            ip_type = 0;

    		pdp_profile.cid = 2;

    		memcpy(pdp_profile.nPdpType, pdp_type, strlen((char *)pdp_type));
    		memcpy(pdp_profile.apn, apn, strlen((char *)apn));
    		ret = fibo_asyn_PDPActive(1, &pdp_profile, simid);

    		g_pdp_asyn_active_sem = fibo_sem_new(0);
    		test_pdp_asyn_active_flag =1;
    		OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
    		fibo_sem_wait(g_pdp_asyn_active_sem);
    		OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
    		fibo_sem_free(g_pdp_asyn_active_sem);
    		OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);

    	}
    	{
    		pdp_profile_t pdp_profile;
    		char *pdp_type = "IPV4V6";
    		char *apn = "INTERNET";
    		memset(&pdp_profile, 0, sizeof(pdp_profile));
            ip_type = 0;

    		pdp_profile.cid = 3;

    		memcpy(pdp_profile.nPdpType, pdp_type, strlen((char *)pdp_type));
    		memcpy(pdp_profile.apn, apn, strlen((char *)apn));
    		ret = fibo_asyn_PDPActive(1, &pdp_profile, simid);

    		g_pdp_asyn_active_sem = fibo_sem_new(0);
    		test_pdp_asyn_active_flag =1;
    		OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
    		fibo_sem_wait(g_pdp_asyn_active_sem);
    		OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
    		fibo_sem_free(g_pdp_asyn_active_sem);
    		OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);

    	}
    }

    fibo_taskSleep(2*1000);
    ret = fibo_get_ip_info_by_cid(2, &ip_type, &ip_addr, &ip6_addr);
    if(0 == ret)
    {
        OSI_PRINTFI("ip_type = %d, %s", ip_type, ip6addr_ntoa((ip6_addr_t *)&ip6_addr.sin6_addr));
    }
    OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
    fibo_taskSleep(2*1000);
    ret = fibo_get_ip_info_by_cid(3, &ip_type, &ip_addr1, &ip6_addr1);
    if(0 == ret)
    {
        OSI_PRINTFI("ip_type = %d, %s", ip_type, ip6addr_ntoa((ip6_addr_t *)&ip6_addr1.sin6_addr));
    }
    OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);

    GAPP_TCPIP_ADDR_T bind_addr, bind_addr1;
    INT8 socketid, socketid1;
    char rcvbuf[128];
    int rcvnum = 0;
    INT32 retcode;

    {
		GAPP_TCPIP_ADDR_T addr;		
		memset(rcvbuf, 0, sizeof(rcvbuf));
		memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
		socketid = fibo_sock_create_ex(AF_INET6,SOCK_STREAM,IPPROTO_IP);
        OSI_PRINTFI("[%s-%d]sys_sock_create  socketid = %d\n", __FUNCTION__, __LINE__, socketid);

        bind_addr.sin_addr.type = AF_INET6;
        memcpy(&bind_addr.sin_addr.u_addr.ip6.addr[0], &ip6_addr.sin6_addr.un.u32_addr[0], sizeof(ip6_addr.sin6_addr.un.u32_addr));
        ret = fibo_sock_bind(socketid, &bind_addr);
        OSI_PRINTFI("[%s-%d] fibo_sock_bind  ret = %d\n", __FUNCTION__, __LINE__, ret);

        addr.sin_addr.type = AF_INET6;
        addr.sin_port = htons(3120);
        ip6addr_aton("2400:3200:1600::202", &addr.sin_addr.u_addr.ip6);
        retcode = fibo_sock_connect(socketid, &addr);
        OSI_PRINTFI("[%s-%d] sys_sock_connect  retcode = %d\n", __FUNCTION__, __LINE__, retcode);
	}

	{
		GAPP_TCPIP_ADDR_T addr;
		memset(rcvbuf, 0, sizeof(rcvbuf));
		memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
		socketid1 = fibo_sock_create_ex(AF_INET6,SOCK_DGRAM,IPPROTO_IP);
        OSI_PRINTFI("[%s-%d]sys_sock_create  retcode = %d\n", __FUNCTION__, __LINE__, socketid1);

        bind_addr1.sin_addr.type = AF_INET6;
        memcpy(&bind_addr1.sin_addr.u_addr.ip6.addr[0], &ip6_addr1.sin6_addr.un.u32_addr[0], sizeof(ip6_addr1.sin6_addr.un.u32_addr));
        ret = fibo_sock_bind(socketid1, &bind_addr1);
        OSI_PRINTFI("[%s-%d] fibo_sock_bind  ret = %d\n", __FUNCTION__, __LINE__, ret);

        addr.sin_port = htons(5105);
        ip6addr_aton("2400:3200:1600::202", &addr.sin_addr.u_addr.ip6);
		addr.sin_addr.type = AF_INET6;
        retcode = fibo_sock_connect(socketid1, &addr);
        OSI_PRINTFI("[%s-%d] sys_sock_connect  retcode = %d\n", __FUNCTION__, __LINE__, retcode);

	}
    while(1)
	{
        ret = fibo_sock_send(socketid, (UINT8 *)"demo opencpu test TCP ok", strlen("demo opencpu test TCP ok"));
        OSI_PRINTFI("[%s-%d] sys_sock_send  retcode = %d\n", __FUNCTION__, __LINE__, ret);			
        fibo_taskSleep((UINT32)1000);
        rcvnum = fibo_sock_recv(socketid, (UINT8 *)rcvbuf, 64);			
		OSI_PRINTFI("[%s-%d] rcvnum = %d, rcvbuf = %s", __FUNCTION__, __LINE__, rcvnum, rcvbuf);

		fibo_taskSleep((UINT32)10*1000);

        ret = fibo_sock_send(socketid1, (UINT8 *)"demo opencpu test udp ok", strlen("demo opencpu test UDP ok"));
        OSI_PRINTFI("[%s-%d] sys_sock_send  retcode = %d\n", __FUNCTION__, __LINE__, ret);			
        fibo_taskSleep((UINT32)1000);
        rcvnum = fibo_sock_recv(socketid1, (UINT8 *)rcvbuf, 64);			
		OSI_PRINTFI("[%s-%d] rcvnum = %d, rcvbuf = %s", __FUNCTION__, __LINE__, rcvnum, rcvbuf);

        fibo_taskSleep((UINT32)5*1000);
	}
#endif
    fibo_thread_delete();
}

static void at_res_callback(UINT8 *buf, UINT16 len)
{
    OSI_PRINTFI("FIBO <--%s", buf);
}

static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = sig_res_callback,
    .at_resp = at_res_callback};

void * appimg_enter(void *param)
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
