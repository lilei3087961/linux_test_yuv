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
#define WIDTH_BIG       1920
#define HIGHT_BIG       1080   
#define FILE_VIDEO  "/dev/video0"
#define JPG "./out/image_%s"
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
unsigned char* yuv420_buff_big;
unsigned char* nv21_test_buff;
unsigned char* nv21_test_buff_big;

void init_mjpeg_encode(void)
{
        nv21_test_buff = (unsigned char*)malloc(WIDTH * HIGHT * 1.5);
        if(nv21_test_buff ==  NULL) {
                perror("nv21_test_buff malloc err\n");
        }else{
        	memset(nv21_test_buff, 0, WIDTH * HIGHT * 1.5);
	}
        
       nv21_test_buff_big = (unsigned char*)malloc(WIDTH_BIG * HIGHT_BIG * 1.5);
        if(nv21_test_buff_big ==  NULL) {
                perror("nv21_test_buff_big malloc err\n");
        }else{
                memset(nv21_test_buff_big, 0, WIDTH_BIG * HIGHT_BIG * 1.5);
        }

	mjpeg_buff = (unsigned char*)malloc(WIDTH * HIGHT * 2);
	if(mjpeg_buff ==  NULL)	{
		perror("mjpeg_encode malloc err\n");
	}else{
		memset(mjpeg_buff, 0, WIDTH * HIGHT * 2);
	}
       
	yuyv_buff = (unsigned char*)malloc(WIDTH * HIGHT * 2);
	if(yuyv_buff ==  NULL){
		perror("mjpeg_encode malloc err\n");
	}else{
		memset(yuyv_buff, 0, WIDTH * HIGHT * 2);
	}

	yuv420_buff = (unsigned char*)malloc(WIDTH * HIGHT * 1.5);
	if(yuv420_buff ==  NULL){
		perror("mjpeg_encode malloc yuv420_buff err\n");
	}else{
		memset(yuv420_buff, 0, WIDTH * HIGHT * 1.5);
	}

        yuv420_buff_big = (unsigned char*)malloc(WIDTH_BIG * HIGHT_BIG * 1.5);
        if(yuv420_buff_big ==  NULL){
                perror("mjpeg_encode malloc yuv420_buff_big err\n");
        }else{
		memset(yuv420_buff_big, 0, WIDTH_BIG * HIGHT_BIG * 1.5);
	}
}
int dump_yuv(void *addr, int length,char* name)
{
	FILE *fp;
	static int num = 0;
	char image_name[20];
	
	sprintf(image_name, JPG, name);
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
//两个yuv图片合成
int merage_two_yuv_for_nv21(unsigned char *big_buff,int width_big,int height_big,unsigned char * small_buff,int width_small,int height_small)
{
    printf("merage_two_yuv_for_nv21 big_buff:%p,width_big:%d height_big:%d small_buff:%p width_small:%d height_small:%d \n",big_buff,width_big,height_big,small_buff,width_small,height_small);
    if(width_big<width_small || height_big < height_small)
    {
        printf("merage_two_yuv_for_nv21 error width_big<width_small || height_big < height_small \n");
        return -1;
    }
   //merge small y and vu to large
    int small_vu_distance = width_small*height_small;
    int large_vu_distance = width_big*height_big;
    //for test
/*    unsigned char * test_buff_y = nv21_test_buff;
    unsigned char * test_buff_vu = nv21_test_buff+small_vu_distance; 
    unsigned char * test_buff_big_y = nv21_test_buff_big;
    unsigned char * test_buff_big_vu = nv21_test_buff_big +large_vu_distance; */

    int y_center_offset = ((height_big - height_small)/2)*width_big + (width_big-width_small)/2;
    int vu_center_offset = ((height_big - height_small)/4)*width_big + (width_big-width_small)/2;
    unsigned char * big_buff_y =big_buff + y_center_offset;
    unsigned char * big_buff_vu = big_buff+large_vu_distance+vu_center_offset;
    unsigned char * small_buff_y = small_buff; 
    unsigned char * small_buff_vu = small_buff + small_vu_distance;
    //memcpy(test_buff_big_y,big_buff,width_big*height_big*3/2); //for test
      printf("##merage_two_yuv_for_nv21  large_vu_distance:%d,small_vu_distance:%d \n",large_vu_distance,small_vu_distance);
    for(int i=0;i<height_small;i++)
    {  
	//printf("merage_two_yuv_for_nv21 big_buff i:%d",i);
        //for test
/*        memcpy(test_buff_y,small_buff_y,width_small); //nv21_test_buff
        memcpy(test_buff_vu,small_buff_vu,width_small/4);
        test_buff_y += width_small;
        test_buff_vu += width_small/2;    */

        memcpy(big_buff_y,small_buff_y,width_small);
        if(i%2 == 0){
	 memcpy(big_buff_vu,small_buff_vu,width_small);
         big_buff_vu += width_big; 
         // memset(big_buff_vu,0,width_small/2); 
        }
        big_buff_y += width_big;
        small_buff_y += width_small;
        small_buff_vu += width_small/2;  
     }
    dump_yuv(nv21_test_buff,width_small*height_small*3/2,"nv21_small");
    dump_yuv(nv21_test_buff_big,width_big*height_big*3/2,"nv21_big");
    dump_yuv(big_buff,width_big*height_big*3/2,"nv21_merage");
}
int parse_jpeg2yuv(FILE *pFile,int len,unsigned char * camera_buff,int lenbig)
{
	int k = WIDTH;
	int j = HIGHT;
	printf("parse_jpeg2yuv 111 pFile:%p len:%d camera_buff:%p lenbig:%d\n",pFile,len,camera_buff,lenbig);
	size_t result = fread(mjpeg_buff,1,len,pFile); 
	printf("parse_jpeg2yuv 222 mjpeg_buff:%p len:%d result:%d \n",mjpeg_buff,strlen(mjpeg_buff),result);
	jpeg_decode(&yuyv_buff, mjpeg_buff, &k, &j);
        YUV422ToNv21(yuyv_buff,yuv420_buff,WIDTH,HIGHT);
        printf("parse_jpeg2yuv 333 YUV422ToNv21 yuv420_buff:%p  \n",yuv420_buff);
        merage_two_yuv_for_nv21(camera_buff,WIDTH_BIG,HIGHT_BIG,yuv420_buff,WIDTH,HIGHT); 
}
int merage_jpeg2yuv_for_nv21(char *jpegPath,int width_small,int height_small,unsigned char * camera_buff,int width_big,int height_big)
{
        int k = width_small;
        int j = height_small;
        //buff1为jpeg分配内存
        mjpeg_buff = (unsigned char*)malloc(width_small * height_small * 2);
        if(mjpeg_buff ==  NULL) {
                perror("merage_jpeg2yuv_for_nv21() step1 mjpeg_buff malloc err\n");
        }else{
                memset(mjpeg_buff, 0, width_small * height_small * 2);
        }
        //buff2为jpeg转码后的yuv422数据分配内存
        yuyv_buff = (unsigned char*)malloc(width_small * height_small * 2);
	if(yuyv_buff ==  NULL){
		perror("merage_jpeg2yuv_for_nv21() step2 yuyv_buff malloc err\n");
	}else{
		memset(yuyv_buff, 0, WIDTH * HIGHT * 2);
	}
        //为yuv422转换为nv21数据分配内存
        yuv420_buff = (unsigned char*)malloc(width_small * height_small * 1.5);
	if(yuv420_buff ==  NULL){
		perror("merage_jpeg2yuv_for_nv21() step3 yuv420_buff malloc yuv420_buff err\n");
	}else{
		memset(yuv420_buff, 0, WIDTH * HIGHT * 1.5);
	}

        FILE *pfile = fopen(jpegPath,"rb");
        printf("merage_jpeg2yuv_for_nv21() 111 pfile:%p \n",pfile);
        fseek(pfile,0,SEEK_END); /* 定位到文件末尾 */
        int len= ftell(pfile);
        fseek(pfile,0,SEEK_SET); /* 定位到文件开头 */
        size_t result = fread(mjpeg_buff,1,len,pfile); //读取mjpeg_buff
        jpeg_decode(&yuyv_buff, mjpeg_buff, &k, &j);   //将jpeg转码成yuyv422
        YUV422ToNv21(yuyv_buff,yuv420_buff,width_small,height_small); //将yuyv422转码成nv21  
        fclose(pfile); 
        printf("merage_jpeg2yuv_for_nv21() 222 mjpeg_buff:%p len:%d  \n",mjpeg_buff,len);
        merage_two_yuv_for_nv21(camera_buff,width_big,height_big,yuv420_buff,width_small,height_small); 
}
int get_camera_buff()
{
	yuv420_buff_big = (unsigned char*)malloc(WIDTH_BIG * HIGHT_BIG * 1.5);
        if(yuv420_buff_big ==  NULL){
                perror("get_camera_buff() malloc yuv420_buff_big err\n");
        }else{
		memset(yuv420_buff_big, 0, WIDTH_BIG * HIGHT_BIG * 1.5);
	}       
        //////step2
        char stryuvpath[80]={"camera.yuv"};
        FILE *pyuvfile = fopen(stryuvpath,"rb"); 
        fseek(pyuvfile,0,SEEK_END); /* 定位到文件末尾 */
        int lenyuv= ftell(pyuvfile);
        fseek(pyuvfile,0l,SEEK_SET); /* 定位到文件开头 */ 
        size_t result = fread(yuv420_buff_big,1,lenyuv,pyuvfile); 
        fclose(pyuvfile);
        printf("get_camera_buff() lenyuv:%d result:%d\n",lenyuv,result);
        
}
         
int main()
{
	printf("testyuv.c main() 000 \n");
        get_camera_buff();
        merage_jpeg2yuv_for_nv21("1.jpg",320,240,yuv420_buff_big,WIDTH_BIG,HIGHT_BIG);

/*	init_mjpeg_encode();
	char strjpegpath[80]={"1.jpg"};
	FILE *pfile = fopen(strjpegpath,"rb");
	printf("testyuv.c main() 222 pfile:%p \n",pfile);
	fseek(pfile,0,SEEK_END);  //定位到文件末尾   
	int len= ftell(pfile);
	fseek(pfile,0,SEEK_SET); //定位到文件开头 
	printf("testyuv.c main() len:%d \n",len);
       //////step2
        char stryuvpath[80]={"camera.yuv"};
        FILE *pyuvfile = fopen(stryuvpath,"rb"); 
        fseek(pyuvfile,0,SEEK_END); //定位到文件末尾
        int lenyuv= ftell(pyuvfile);
        fseek(pyuvfile,0,SEEK_SET); //定位到文件开头
        size_t result = fread(yuv420_buff_big,1,lenyuv,pyuvfile); 
        printf("testyuv.c main() lenyuv:%d result:%d\n",lenyuv,result);
        
	parse_jpeg2yuv(pfile,len,yuv420_buff_big,lenyuv);
	fclose(pfile); */
}
