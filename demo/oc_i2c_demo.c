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
		  
		  static HAL_I2C_BPS_T bsp1_1 = {1,1}; 
		  UINT8 data[6];
		  UINT8 data1[2] ={0x21,0x30};
		  i2c_Handle i2c_handle=0;
		  int result = 0;
		  drvI2cSlave_t drv_i2c= {0x44,0,0,true,false};
		  UINT16 data2[1] ={0xe000};

	 static void prvThreadEntry(void *param)
	 {
		 OSI_LOGI(0, "application thread enter, param 0x%x", param);
		 fibo_gpio_mode_set(41,0);
		 fibo_gpio_mode_set(42,0);
		 OSI_LOGI(0, "application thread enter, param 0x%x", param);
				 result = fibo_i2c_open(bsp1_1,&i2c_handle);
			 OSI_LOGI(0,"i2c handle is %p",i2c_handle);
			 OSI_LOGI(0, "test_i2c open result %d",result);
			 fibo_taskSleep(1000);
			 result = fibo_i2c_Write(i2c_handle,drv_i2c,data1,2);
			 OSI_PRINTFI("test_i2c i2c_send_data result = %d",result);
			 OSI_PRINTFI("test_i2c i2c_send_data data = %d,%d",data1[0],data1[1]);
			 while(1)
				  {
				  fibo_taskSleep(1000);
				 result = fibo_i2c_xfer(i2c_handle,drv_i2c,data,6,data2,1);
				  OSI_PRINTFI("test_i2c i2c_get_data data = %d,%d",data[0],data[1]);  
				  }
		 fibo_thread_delete();
	 }
	 
	 void * appimg_enter(void *param)
	 {
		 OSI_LOGI(0, "application image enter, param 0x%x", param);
	 
	 
		 fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
		 return 0;
	 }
	 
	 void appimg_exit(void)
	 {
		 OSI_LOGI(0, "application image exit");
	 }

