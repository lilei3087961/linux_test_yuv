#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include "utils.h"

#define WIDTH		320
#define	HIGHT		240
#define FILE_VIDEO  "/dev/video0"
#define JPG "./out/image%d"
typedef struct{
    void *start;
	int length;
}BUFTYPE;
BUFTYPE *usr_buf;

static unsigned int n_buffer = 0;  
struct timeval time;
unsigned char* mjpeg_buff;
unsigned char* yuyv_buff;
unsigned char* yuv420_buff;

void init_mjpeg_encode(void)
{
	mjpeg_buff = (unsigned char*)malloc(WIDTH * HIGHT * 2);
	if(mjpeg_buff ==  NULL)
	{
		perror("mjpeg_encode malloc err\n");
	};

	yuyv_buff = (unsigned char*)malloc(WIDTH * HIGHT * 2);
	if(yuyv_buff ==  NULL)
	{
		perror("mjpeg_encode malloc err\n");
	};

	yuv420_buff = (unsigned char*)malloc(WIDTH * HIGHT * 1.5);
	if(yuv420_buff ==  NULL)
	{
		perror("mjpeg_encode malloc yuv420_buff err\n");
	};

	memset(yuv420_buff, 0, WIDTH * HIGHT * 1.5);
	
}
int process_image(void *addr, int length)
{
	FILE *fp;
	static int num = 0;
	char image_name[20];
	
	sprintf(image_name, JPG, num++);
	if((fp = fopen(image_name, "w")) == NULL)
	{
		perror("Fail to fopen JPG");
		exit(EXIT_FAILURE);
	}
	//fwrite(addr, WIDTH*HIGHT*2 , 1, fp);
        fwrite(addr, length , 1, fp);
	usleep(500);
	fclose(fp);
	return 0;
}
int parse_jpeg2yuv(FILE *pFile,int len)
{
	int k = WIDTH;
	int j = HIGHT;
	printf("parse_jpeg2yuv 111 pFile:%p len:%d\n",pFile,len);
	//mjpeg_buff = (unsigned char *)pFile;
	size_t result = fread(mjpeg_buff,1,len,pFile); 
	printf("parse_jpeg2yuv 222 mjpeg_buff:%p len:%d result:%d \n",mjpeg_buff,strlen(mjpeg_buff),result);
	jpeg_decode(&yuyv_buff, mjpeg_buff, &k, &j);
	//process_image(yuyv_buff, WIDTH * HIGHT*2);
        //YUV422ToI420(yuyv_buff,yuv420_buff,WIDTH,HIGHT);
        YUV422ToNv21(yuyv_buff,yuv420_buff,WIDTH,HIGHT);
        //yuv422toyuv420(yuv420_buff,yuyv_buff,WIDTH,HIGHT);
        printf("parse_jpeg2yuv 333 YUV422ToNv21 yuv420_buff:%p  \n",yuv420_buff);
        process_image(yuv420_buff, WIDTH * HIGHT*1.5);
}
int main()
{
	printf("testYuv.c main() 000 \n");
	init_mjpeg_encode();
	char strjpegPath[80]={"1.jpg"};
	//char strjpegPath[80]={"/sdcard/DCIM/1.jpg"};
	printf("testYuv.c main() 111 \n");
	FILE *pFile = fopen(strjpegPath,"rb");
	printf("testYuv.c main() 222 pFile:%p \n",pFile);
	fseek(pFile,0,SEEK_END); /* 定位到文件末尾 */  
	int len= ftell(pFile);
	fseek(pFile,0L,SEEK_SET); /* 定位到文件开头 */  
	printf("testYuv.c main() len:%d \n",len);
	parse_jpeg2yuv(pFile,len);
	fclose(pFile);
}
