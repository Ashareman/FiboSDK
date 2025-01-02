#if 1




#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Y', 'A', 'P')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
/*¼��������*/
typedef struct 
{
	auStreamFormat_t format;    //¼��������
	uint8_t  *audioRecBuf;      //¼����buf�ռ�
	uint32_t audioRecBufWp;     //¼����Ҫ���ݵ�ָ��
	int32_t  audioRecFd;        //��¼������д���ļ���
	int32_t  audioRecBufSize;   //¼����buf�ռ�Ĵ�С
	int32_t  audiorec_pos;      //��¼��ǰ��ָ��ƫ�Ƶĵ�ַ
	int32_t  audiorec_leftSize;
}RecordContext_t;

static RecordContext_t grecordctx = {};



typedef struct 
{
	auStreamFormat_t format;    //���ŵ�����
	uint8_t *audioPlayBuf;   
	uint32_t audioPlayWp;   
	uint32_t audioPlayRp; 
	int32_t  audioPlayFd;   
	int32_t  audioplayBufSize;
	int32_t  audioplay_pos;

	uint8_t  *audioTempPlayDatabuf;
	uint32_t  audioTempPlayDataSize;
}PlayContext_t;

static PlayContext_t gplayctx = {};



extern void test_printf(void);

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}


#define TEST_AUDIO_PLAY_BUF_SIZE   		1024*30
#define OFFSET_AUDIO_PLAY_BUF_SIZE      1000

#define TEST_AUDIO_REC_BUF_SIZE   		1024*8
#define OFFSET_AUDIO_REC_BUF_SIZE       500
#define TEST_AUDIO_FILE_MAX_LEN         1024*150


static int 	TestPlayOffset = 0; 






static void prvThreadEntry(void *param)
{
	/*
	*����¼���Ĳ���
	*/
	RecordContext_t *prec= &grecordctx;
	//char FileNameRec[32] = "FFS/audio_rec1.mp3";
	char FileNameRec[32] = "FFS/audio_rec1.pcm";
	//char FileNameRec[32] = "FFS/8K16m0.pcm"
	int32_t  fileRecSize = 0;
	
	
	PlayContext_t *pplay = &gplayctx;
	//char FileNamePlay[128] = "/FFS/audioPlay1.mp3";
	//char FileNamePlay[128] = "/FFS/aiti.pcm";
	char FileNamePlay[128] = "FFS/8K16m0.pcm";
	int  play_ret = 0;
	uint32_t play_read = 0;
	int32_t  filePlaySize = 0;

	
	
	
	prec->format = 1;
	prec->audioRecBufSize= TEST_AUDIO_REC_BUF_SIZE;   //1024*8
	prec->audioRecBuf = NULL;
	prec->audioRecBufWp= 0;
	prec->audioRecFd= 0;
	prec->audiorec_pos = 0;
	prec->audiorec_leftSize =0;
	

	//int audioRecRp_pos = 0;     
	//int audioRecMaxLen = 4096*15;//¼�Ƶ���Ƶ�ļ��Ĵ�С
	//int audioRecLeftSize = 0;
	

   /*
	*���ڲ��ŵĺ���
	*/
	pplay->format = 1;
	pplay->audioplayBufSize = TEST_AUDIO_PLAY_BUF_SIZE;  //1024*8
	pplay->audioPlayRp = 0;
	pplay->audioPlayWp = 0;
	pplay->audioPlayFd = 0;
	pplay->audioPlayBuf = NULL;
    pplay->audioTempPlayDatabuf = NULL;
	pplay->audioTempPlayDataSize = OFFSET_AUDIO_PLAY_BUF_SIZE; //500
	pplay->audioplay_pos = 0;
    
	
	


	/*************¼����ص�**************/
	//static uint8_t *audioRecTestbuff = NULL;

	//����¼���Ŀռ�
 	prec->audioRecBuf= (uint8_t *)malloc(prec->audioRecBufSize);
	if(prec->audioRecBuf == NULL)
	{
		return;
	}
	//����¼���ļ�
	prec->audioRecFd = vfs_open(FileNameRec, O_RDWR | O_CREAT | O_TRUNC);	
	if (prec->audioRecFd < 0)	
	{		
		OSI_LOGI(0, "gengt114 create file failed");
	}

	//�򿪲��ŵ��ļ�
	pplay->audioPlayFd = vfs_open(FileNamePlay, FS_O_RDONLY);
	if(pplay->audioPlayFd < 0)
	{
		OSI_PRINTFI("gengt pplay->audioPlayFd error1");
	}
	else
	{
    	OSI_PRINTFI("gengt pplay->audioPlayFd success1");
	}

    //��ȡ���������ļ����ļ�����   
    filePlaySize= vfs_file_size(FileNamePlay);       
    OSI_PRINTFI("gengt pplay filesize = %d ",filePlaySize);

	//���벥�ŵĿռ�1
	pplay->audioPlayBuf = (uint8_t *)malloc(pplay->audioplayBufSize); 
	if (pplay->audioPlayBuf == NULL)
	{
		OSI_PRINTFI("gengt89 pplay->audioPlayBuf  malloc error");	
	}
	//�����ŵĿռ��buffer���
	memset(pplay->audioPlayBuf, 0,pplay->audioplayBufSize);  
	 //��ȡ�ļ��ĵ�ǰƫ�������ο���ǰλ��
	play_ret= vfs_lseek(pplay->audioPlayFd,TestPlayOffset,FS_SEEK_SET);  
	if(play_ret < 0)
	{	
		OSI_PRINTFI("gengt pplay vfs_lseek error");	
	} 
    //������ʱbuf�Ŀռ�
	pplay->audioTempPlayDatabuf = (uint8_t *)malloc(pplay->audioTempPlayDataSize); 
	if( pplay->audioTempPlayDatabuf == NULL)
	{
		OSI_PRINTFI("gengt89 pplay->audioTempPlayDatabuf  malloc error");	
	}
    memset(pplay->audioTempPlayDatabuf,0,pplay->audioTempPlayDataSize);	

     
	#if 1
	//read����������ֽڣ���SHELL_OFFSET_BUF_SIZE == 500Byte ��Audio_data������
	play_read = fibo_file_read(pplay->audioPlayFd, pplay->audioTempPlayDatabuf, pplay->audioTempPlayDataSize); 
	memcpy(&pplay->audioPlayBuf[pplay->audioPlayWp], pplay->audioTempPlayDatabuf, play_read);
	
	OSI_PRINTFI("gengt187 play_read = %d ",play_read); 
	pplay->audioPlayWp = play_read;
	#endif
	play_ret = fibo_audio_poc_play_record_start(AUSTREAM_FORMAT_PCM,AUSTREAM_FORMAT_PCM,8000,1,pplay->audioPlayBuf,&pplay->audioPlayRp,&pplay->audioPlayWp,pplay->audioplayBufSize,prec->audioRecBuf,&prec->audioRecBufWp,prec->audioRecBufSize);
	if(play_ret)
	{
		OSI_PRINTFI("gengt193 fibo_audio_poc_play_record success"); 
	}
	else
	{
		return;
	}
	
	for(;;)
	{
		while(1)
		{
			OSI_PRINTFI("gengt204 fibo_file_read = %d  / Rp = %d  / Wp = %d",play_read,pplay->audioPlayRp,pplay->audioPlayWp);
			
			//�����жϵ�ǰ��Pipeָ���Ƿ��ȵȣ� WP���ɿͻ����ݵ�
			if ( pplay->audioPlayRp == pplay->audioPlayWp)
			{	
				//�ӵ�ǰλ�ÿ�ʼ������
				play_ret = fibo_file_seek(pplay->audioPlayFd,TestPlayOffset,FS_SEEK_SET);  
				if(play_ret  < 0)
				{	
					OSI_PRINTFI("gengt399 Step 3 fileseek failed");	
				}
				//�ļ��ĳ��� - ��ǰ���ļ���λ��  
				if(filePlaySize- TestPlayOffset >= OFFSET_AUDIO_PLAY_BUF_SIZE)
				{
					play_read= fibo_file_read(pplay->audioPlayFd,pplay->audioTempPlayDatabuf, OFFSET_AUDIO_PLAY_BUF_SIZE);
				}
				else
				{
					play_read = fibo_file_read(pplay->audioPlayFd,pplay->audioTempPlayDatabuf,filePlaySize- TestPlayOffset);
				}
				
				OSI_PRINTFI("gengt225 fibo_file_read = %d  / Rp = %d  / Wp = %d",play_read,pplay->audioPlayRp,pplay->audioPlayWp);	
				TestPlayOffset += play_read;   
				//��ӡ��ǰ�ļ���ƫ��λ��
				OSI_PRINTFI("gengt228 TestPlayOffset = %d",TestPlayOffset);	
				
				//memset(g_pcm_buff, 0, sizeof(g_pcm_buff));
				if(pplay->audioplayBufSize - pplay->audioPlayWp > play_read)
				{
					
					memcpy(&pplay->audioPlayBuf[pplay->audioPlayWp], pplay->audioTempPlayDatabuf, play_read);
					pplay->audioPlayWp += play_read;
					OSI_PRINTFI("gengt236 pplay->audioPlayWp = %d",pplay->audioPlayWp);	
				}
				else
				{
					pplay->audioplay_pos = pplay->audioplayBufSize - pplay->audioPlayWp;
					if( 0 == pplay->audioplay_pos )
					{
					 	memcpy(&pplay->audioPlayBuf[pplay->audioPlayWp], pplay->audioTempPlayDatabuf, play_read);
						
						//Wp = Read;
						pplay->audioPlayWp = 0;
					}
					else
					{
					 	memcpy(pplay->audioPlayBuf[pplay->audioPlayWp], pplay->audioTempPlayDatabuf, pplay->audioplay_pos );
						pplay->audioPlayWp = 0;
					 	memcpy(pplay->audioPlayBuf[0], pplay->audioTempPlayDatabuf[pplay->audioplay_pos ],play_read - pplay->audioplay_pos );
						pplay->audioPlayWp= play_read - pplay->audioplay_pos ;
					}
					OSI_PRINTFI("gengt255 pplay->audioPlayWp = %d",pplay->audioPlayWp);	
				}
				
				OSI_PRINTFI("gengt258 play_read = %d pplay->audioPlayRp = %d pplay->audioPlayWp = %d",play_read,pplay->audioPlayRp,pplay->audioPlayWp);	
			}

			OSI_PRINTFI("gengt261  prec->audiorec_pos = %d prec->audioRecBufWp = %d",prec->audiorec_pos,prec->audioRecBufWp); 

			if(prec->audiorec_pos != prec->audioRecBufWp)	
		    {	
				fileRecSize = vfs_file_size(FileNameRec);		
				OSI_LOGI(0, "gengt264 fileRecSize  = %d max_len = %d",fileRecSize,TEST_AUDIO_FILE_MAX_LEN);	
				if (fileRecSize >= TEST_AUDIO_FILE_MAX_LEN)		  
				{				
					fibo_audio_poc_play_record_stop();	
					free(prec->audioRecBuf);				
					prec->audioRecBuf = NULL;			
					prec->audioRecBufWp= 0;
					vfs_close(prec->audioRecFd);	
					break;			
				}			
				OSI_LOGI(0, "gengt274 audio recorder start to save pcm");
				//˵��������д�������Pipe����
				if (prec->audiorec_pos < prec->audioRecBufWp)			
				{				
					OSI_LOGI(0, "gengt278 audio recorder read position = %d < write=%d",prec->audiorec_pos,prec->audioRecBufWp);		
					vfs_write(prec->audioRecFd, &(prec->audioRecBuf[prec->audiorec_pos]), (prec->audioRecBufWp - prec->audiorec_pos));	
					prec->audiorec_pos = prec->audioRecBufWp;			
				}	//˵��������д����pipe���棬���ҿ�ʼ��ͷд��		
				else   			
				{				
					OSI_LOGI(0, "gengt284 audio recorder read position = %d > write=%d",prec->audiorec_pos,prec->audioRecBufWp);			
					prec->audiorec_leftSize = prec->audioRecBufSize - prec->audiorec_pos;				
					OSI_LOGI(0, "gengt286 audio recorder leftspace %d", prec->audiorec_leftSize);			
					vfs_write(prec->audioRecFd, &(prec->audioRecBuf[prec->audiorec_pos]), prec->audiorec_leftSize);			
					prec->audiorec_pos = 0;				
					vfs_write(prec->audioRecFd, &(prec->audioRecBuf[prec->audiorec_pos]), (prec->audioRecBufWp- prec->audiorec_pos));  
					prec->audiorec_pos += prec->audioRecBufWp;	

				}	
			}
			#if 0
			else		
			{		
			 	 OSI_LOGI(0, "gengt There is no rec data from mic,wait moment.");
				 //osiThreadSleep(2);		
				 continue;		
			 }
			#endif
				fibo_taskSleep(10);
			}	
	
	}
	
	OSI_PRINTFI("gengt Step 5 success");
    fibo_thread_delete();
}


void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
#endif






