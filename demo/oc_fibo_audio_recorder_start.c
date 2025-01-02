#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Y', 'A', 'P')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "osi_api.h"

extern void test_printf(void);
osiThread_t *gengt;
int fdSize = 0;
int fd;
/**
 * Read from pipe, and write to file   。函数指针传参后真正调用的就是这个函数
 */
static void prvRecordPipeRead(void *param)
{
    osiPipe_t *d_pipe = (osiPipe_t *)param;

    if (d_pipe == NULL)
        return;

    char buf[640];
    for (;;)
    {
        //为什么是osiPipeRead，那是因为录音的数据会存放在这个pipe里面；
        int bytes = osiPipeRead(d_pipe, buf, 640);
        OSI_LOGI(0, "gengt bytes = %d",bytes);
        if (bytes <= 0)
            break;
        //这一步时间录制的音频数据放在文件系统里面
        fibo_file_write(fd, buf, bytes);
        
    }
}


//这是客户在openCPU中创建的线程用于接收录音的音频数据
static void gengtTaskEntry(void *param)
{
    OSI_LOGI(0, "gengt thread enter, param 0x%x", param);
    //srand(100);
    //这一步就是为了获取当前线程ID，执行到这里，肯定是当前线程，不用再刻意的记线程ID
    osiThread_t  *thread_id = osiThreadCurrent();  
    for (;;)
    {
        #if  1
        osiEvent_t event = {};

        //等待是否有录音的消息发送到当前线程(阻塞等待)
        osiEventWait(thread_id, &event);
        //OSI_LOGI(0, "gengt %d");
        #endif
        #if   1
        //为什么是/FFS/文件夹下-->  是为了方便调用AT+AUDPLAY=1,"hello1.amr" 去播放音频，方便快速验证
        fdSize = fibo_file_getSize("/FFS/hello1.wav");
        OSI_LOGI(0, "gengt fdSize=%d", fdSize);
       
        //我是为了录制足够多的音频之后，去验证fibo_audio_recorder_stop（）；
        if(fdSize > 1024*200)
        {
            OSI_LOGI(0, "gengt62 ");
            fibo_audio_recorder_stop();
            fibo_file_close(fd);
        }
        #endif
    }
}

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    //srand(100);
    char fnameamr1[128] = "/FFS/hello1.wav";

    fd = fibo_file_open(fnameamr1, FS_O_RDWR|FS_O_CREAT|FS_O_TRUNC);//打开文件
    OSI_PRINTFI("audio gengt Step 1 success");
    for (int n = 0; n < 10; n++)
    {
        OSI_LOGI(0, "hello world %d", n);
        fibo_taskSleep(100);
    }
    //fibo_audio_recorder_start(&gengt,prvRecordPipeRead,1,1,0);
    OSI_LOGI(0, "gengt  11111 ");
    //fibo_taskSleep(50*1000);    这里不关注，我是为了延时时间足够长，去拨打电话；
    OSI_LOGI(0, "gengt 22222");
    //这里调用了接口
    fibo_audio_recorder_start(gengt,prvRecordPipeRead,fd,1,2,0);
    fibo_thread_delete();
}
void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    //申请一个指定的线程去处理录音的音频数据
    gengt = osiThreadCreate("gengt", gengtTaskEntry, NULL,OSI_PRIORITY_NORMAL, 1024*10,10);
    fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    
    return 0;
}
void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}

#if   0

[Project]: all project
[OA/JIRA]: IRP-20220900158  【花样年】【MC615-CN-02-80】 增加上下行混音通话录音
[Influence]: all project
[Description]: 录音，并将录音数据发送给客户的openCpu中申请的指定线程，读取音频数据
[Common issue]:需求
备注：
1>INT32 fibo_audio_recorder_start(osiThread_t *rec_thread,osiCallback_t cb,
        INT32 fd,uint8_t rec_type,uint8_t rec_format,auAmrnbMode_t amr_mode)
函数作用:录音，并将录音数据发送给客户的openCpu中申请的指定线程，读取音频数据
参数1 osiThread_t *rec_thread：客户在openCPU中自己创的线程
参数2 osiCallback_t cb ：客户在openCPU中实现的回调函数
参数3：INT32 fd:客户如果录制的是wav，写入到文件系统中，需要传入文件描述符用于stop添加wav头,其他类型传0即可；
参数4 uint8_t rec_type,：录音方式  1：本地录音  2：通话状态下开启录音
参数5 uint8_t rec_format, ：录音类型：
      AUSTREAM_FORMAT_PCM, 	 ---> 1     PCM
      AUSTREAM_FORMAT_WAVPCM,    ---> 2     WAV
      AUSTREAM_FORMAT_MP3,    	 ---> 3     MP3当前基线不支持录音MP3格式
      AUSTREAM_FORMAT_AMRNB,  	 ---> 4     AMR-NB
参数6 auAmrnbMode_t amr_mode
AU_AMRNB_MODE_475,     // 4.75 kbps    ----> 0
AU_AMRNB_MODE_515,     // 5.15 kbps    ----> 1
AU_AMRNB_MODE_590,     // 5.90 kbps    ----> 2
AU_AMRNB_MODE_670,     // 6.70 kbps    ----> 2
AU_AMRNB_MODE_740,     // 7.40 kbps    ----> 3
AU_AMRNB_MODE_795,     // 7.95 kbps    ----> 4
AU_AMRNB_MODE_1020,    // 10.2 kbps    ----> 5
AU_AMRNB_MODE_1220,    // 12.2 kbps    ----> 6
2>int32_t fibo_audio_recorder_stop(void)
函数作用：停止录音,配合fibo_audio_recorder_start使用



#endif

