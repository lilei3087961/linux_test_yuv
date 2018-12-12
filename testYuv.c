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
#define WIDTH_BIG       640//1920
#define HIGHT_BIG       480//1080
#define FILE_VIDEO  "/dev/video0"
#define JPG "./out/image_%s"
int gccFlag = 0;
#if defined __GNUC__ && __GNUC__ >= 4
#  define CV_EXPORTS __attribute__ ((visibility ("default")))
#else
//printf("lilei 222 \n");
//gccFlag = 2;
#endif
typedef CV_EXPORTS struct{
    void *start;
	int length;
}BUFTYPE;
BUFTYPE *usr_buf;

static unsigned int n_buffer = 0;  
struct timeval time;
unsigned char* mjpeg_buff;
unsigned char* yuyv_buff;
unsigned char* yuv420_buff;
unsigned char* yuv420_buff_for_yv12;
unsigned char* yuv420_buff_for_nv21;
unsigned char* yuv420_buff_big;
unsigned char* nv21_test_buff;
unsigned char* nv21_test_buff_big;

int dump_yuv(void *addr, int length,char* name)
{
	FILE *fp;
	static int num = 0;
	//char image_name[20];
	
	//sprintf(image_name, JPG, name);
	printf("lilei getPropertyMultiScan() ##dump_yuv 111 \n");
	if((fp = fopen(name, "w")) == NULL)
	{
		perror("dump_yuv getPropertyMultiScan() Fail to fopen file");
		exit(EXIT_FAILURE);
	}
	//fwrite(addr, WIDTH*HIGHT*2 , 1, fp);
        fwrite(addr, length , 1, fp);
	usleep(500);
	fclose(fp);
	return 0;
}
//两个yuv图片合成
int merge_two_yuv_for_nv21(unsigned char *big_buff,int width_big,int height_big,unsigned char * small_buff,int width_small,int height_small)
{
    printf("merge_two_yuv_for_nv21 big_buff:%p,width_big:%d height_big:%d small_buff:%p width_small:%d height_small:%d \n",big_buff,width_big,height_big,small_buff,width_small,height_small);
    if(width_big<width_small || height_big < height_small)
    {
        printf("merge_two_yuv_for_nv21 error width_big<width_small || height_big < height_small \n");
        return -1;
    }
    //for test
    //nv21_test_buff = (unsigned char*)malloc(width_small * height_small * 2);
    //YUV420spRotateNegative90(nv21_test_buff,small_buff,width_small,height_small);
    //small_buff = nv21_test_buff;
//    dump_yuv(small_buff,width_small*height_small*3/2,"out/image_yv12_small_rotate_90.yuv");
//    return;

    int small_vu_distance = width_small*height_small;
    int large_vu_distance = width_big*height_big;
    int offset_horizontal = +10; //基于小图居中横向偏移量
    int offset_vertical = +10;   //基于小图居中纵向偏移量
    int y_offset = width_big*offset_vertical+offset_horizontal;//基于小图居中后y偏移量
    int vu_offset = width_big*offset_vertical/2 +offset_horizontal;//基于小图居中后vu偏移量

    int y_center_offset = ((height_big - height_small)/2)*width_big + (width_big-width_small)/2;
    int vu_center_offset = ((height_big - height_small)/4)*width_big + (width_big-width_small)/2;
    unsigned char * big_buff_y =big_buff + y_center_offset+y_offset;
    unsigned char * big_buff_vu = big_buff+large_vu_distance+vu_center_offset+vu_offset;
    unsigned char * small_buff_y = small_buff; 
    unsigned char * small_buff_vu = small_buff + small_vu_distance;
    printf("##merge_two_yuv_for_nv21  large_vu_distance:%d,small_vu_distance:%d \n",large_vu_distance,small_vu_distance);
    for(int i=0;i<height_small;i++)
    {  
        memcpy(big_buff_y,small_buff_y,width_small);
        if(i%2 == 0){
		memcpy(big_buff_vu,small_buff_vu,width_small);
		big_buff_vu += width_big; 
        }
        big_buff_y += width_big;
        small_buff_y += width_small;
        small_buff_vu += width_small/2;  //或者在每两行y偏移一个width_small
     }
    //dump_yuv(nv21_test_buff,width_small*height_small*3/2,"nv21_small");
    //dump_yuv(nv21_test_buff_big,width_big*height_big*3/2,"nv21_big");
    dump_yuv(big_buff,width_big*height_big*3/2,"out/image_nv21_merge_+10_+10.yuv");
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
		memset(yuyv_buff, 0, width_small * height_small * 2);
	}
        //为yuv422转换为nv21数据分配内存
        yuv420_buff = (unsigned char*)malloc(width_small * height_small * 1.5);
	if(yuv420_buff ==  NULL){
		perror("merage_jpeg2yuv_for_nv21() step3 yuv420_buff malloc yuv420_buff err\n");
	}else{
		memset(yuv420_buff, 0, width_small * height_small * 1.5);
	}

        FILE *pfile = fopen(jpegPath,"rb");
        printf("merage_jpeg2yuv_for_nv21() 111 pfile:%p \n",pfile);
        fseek(pfile,0,SEEK_END); /* 定位到文件末尾 */
        int len= ftell(pfile);
        fseek(pfile,0,SEEK_SET); /* 定位到文件开头 */
        size_t result = fread(mjpeg_buff,1,len,pfile); //读取mjpeg_buff
        if (result != len)
        {
        	printf("warning!! read mjpeg_buff:%zu \n",result);
        }
        jpeg_decode(&yuyv_buff, mjpeg_buff, &k, &j);   //将jpeg转码成yuyv422
        dump_yuv(yuyv_buff,width_small*height_small*2,"out/yuv422_small.yuv");
        return;
        YUV422ToNv21(yuyv_buff,yuv420_buff,width_small,height_small); //将yuyv422转码成nv21
        fclose(pfile); 
        printf("merage_jpeg2yuv_for_nv21() 222 mjpeg_buff:%p len:%d  \n",mjpeg_buff,len);
        merge_two_yuv_for_nv21(camera_buff,width_big,height_big,yuv420_buff,width_small,height_small);
}
//两个yv12图片合成
void merge_two_yuv_for_yv12(unsigned char *big_buff,int width_big,int height_big,unsigned char * small_buff,int width_small,int height_small)
{
    printf("lilei getPropertyMultiScan() merge_two_yuv_for_yv12 big_buff:%p,width_big:%d height_big:%d small_buff:%p width_small:%d height_small:%d \n",big_buff,width_big,height_big,small_buff,width_small,height_small);
    if(width_big<width_small || height_big < height_small)
    {
        printf("lilei getPropertyMultiScan() merge_two_yuv_for_yv12 error width_big<width_small || height_big < height_small \n");
        return;
    }
    int small_vu_distance = width_small*height_small;
    int large_vu_distance = width_big*height_big;
    int offset_horizontal = -10; //基于小图居中横向偏移量
    int offset_vertical = -10;   //基于小图居中纵向偏移量
    int y_offset = width_big*offset_vertical+offset_horizontal;//基于小图居中后y偏移量
    int v_offset = (width_big/2)*offset_vertical/2 +offset_horizontal/2; //基于小图居中后v偏移量
    int u_offset = (width_big/2)*offset_vertical/2 +offset_horizontal/2; //基于小图居中后u偏移量

    int y_center_offset = ((height_big - height_small)/2)*width_big + (width_big-width_small)/2;
    int v_center_offset = ((height_big - height_small)/4)*width_big/2 + (width_big-width_small)/4;
    int u_center_offset = ((height_big - height_small)/4)*width_big/2 + (width_big-width_small)/4;
    unsigned char * big_buff_y =big_buff + y_center_offset+y_offset;
    unsigned char * big_buff_v = big_buff+large_vu_distance + v_center_offset+v_offset;
    unsigned char * big_buff_u = big_buff+large_vu_distance*5/4 + u_center_offset+u_offset;
    unsigned char * small_buff_y = small_buff;
    unsigned char * small_buff_v = small_buff + small_vu_distance;
    unsigned char * small_buff_u = small_buff + small_vu_distance*5/4;
    printf("lilei getPropertyMultiScan() ##merge_two_yuv_for_yv12  large_vu_distance:%d,small_vu_distance:%d \n",large_vu_distance,small_vu_distance);
    for(int i=0;i<height_small;i++)
    {
        memcpy(big_buff_y,small_buff_y,width_small);
        if(i%2 == 0){
			memcpy(big_buff_v,small_buff_v,width_small/2);
			memcpy(big_buff_u,small_buff_u,width_small/2);
			big_buff_v += width_big/2;
			big_buff_u += width_big/2;
			small_buff_v += width_small/2;
			small_buff_u += width_small/2;
        }
        big_buff_y += width_big;
        small_buff_y += width_small;
     }

     //dump_yuv(nv21_test_buff,width_small*height_small*3/2,"/sdcard/DCIM/image_nv21_small.yuv");
     //dump_yuv(nv21_test_buff_big,width_big*height_big*3/2,"/sdcard/DCIM/image_nv21_big.yuv");
     dump_yuv(big_buff,width_big*height_big*3/2,"out/image_yv12_merge_-10_-10.yuv");
}
//将jpeg合并到yv12图像中
void merge_jpeg2yuv_for_yv12(const char *jpegPath,int width_small,int height_small,unsigned char * camera_buff,int width_big,int height_big)
{
        int k = width_small;
        int j = height_small;
        //buff1为jpeg分配内存
        mjpeg_buff = (unsigned char*)malloc(width_small * height_small * 2);
        if(mjpeg_buff ==  NULL) {
                printf("lilei getPropertyMultiScan() merge_jpeg2yuv_for_yv12() step1 mjpeg_buff malloc err\n");
        }else{
                memset(mjpeg_buff, 0, width_small * height_small * 2);
        }
        //buff2为jpeg转码后的yuv422数据分配内存
        yuyv_buff = (unsigned char*)malloc(width_small * height_small * 2);
	if(yuyv_buff ==  NULL){
		printf("lilei getPropertyMultiScan() merge_jpeg2yuv_for_yv12() step2 yuyv_buff malloc err\n");
	}else{
		memset(yuyv_buff, 0, width_small * height_small * 2);
	}
        //为yuv422转换为nv21数据分配内存
        yuv420_buff = (unsigned char*)malloc(width_small * height_small * 1.5);
	if(yuv420_buff ==  NULL){
		printf("lilei getPropertyMultiScan() merge_jpeg2yuv_for_yv12() step3 yuv420_buff malloc yuv420_buff err\n");
	}else{
		memset(yuv420_buff, 0, width_small * height_small * 1.5);
	}

        FILE *pfile = fopen(jpegPath,"rb");
        printf("lilei getPropertyMultiScan() merge_jpeg2yuv_for_yv12() 111 pfile:%p \n",pfile);
        fseek(pfile,0,SEEK_END); /* 定位到文件末尾 */
        int len= ftell(pfile);
        fseek(pfile,0,SEEK_SET); /* 定位到文件开头 */
        size_t result = fread(mjpeg_buff,1,len,pfile); //读取mjpeg_buff
        jpeg_decode(&yuyv_buff, mjpeg_buff, &k, &j);   //将jpeg转码成yuyv422
        dump_yuv(yuyv_buff,width_big*height_big*2,"out/dump_yuyv.yuv");
        //YUV422ToYv12(yuyv_buff,yuv420_buff,width_small,height_small); //将yuyv422转码成yv12
        fclose(pfile);
        printf("lilei getPropertyMultiScan() merge_jpeg2yuv_for_yv12() 222 mjpeg_buff:%p len:%d  \n",mjpeg_buff,len);
        //merge_two_yuv_for_yv12(camera_buff,width_big,height_big,yuv420_buff,width_small,height_small);
        free(mjpeg_buff);
        free(yuv420_buff);
        free(yuyv_buff);

}
//将nv21合并到yv12图像中
void merge_yuv2yuv_for_yv12(const char *jpegPath,int width_small,int height_small,unsigned char * camera_buff,int width_big,int height_big)
{
        int k = width_small;
        int j = height_small;

        //为yuv422转换为nv21数据分配内存
        yuv420_buff = (unsigned char*)malloc(width_small * height_small * 1.5);
		if(yuv420_buff ==  NULL){
			printf("lilei getPropertyMultiScan() merge_yuv2yuv_for_yv12() step3 yuv420_buff malloc yuv420_buff err\n");
		}else{
			memset(yuv420_buff, 0, width_small * height_small * 1.5);
		}
		//为nv21转换为yv12数据分配内存
	    yuv420_buff_for_yv12 = (unsigned char*)malloc(width_small * height_small * 1.5);
		if(yuv420_buff_for_yv12 ==  NULL){
			printf("lilei getPropertyMultiScan() gome_merge_yuv2yuv_for_yv12() step3 malloc yuv420_buff_for_yv12 err\n");
		}else{
			memset(yuv420_buff_for_yv12, 0, width_small * height_small * 1.5);
		}

        FILE *pfile = fopen(jpegPath,"rb");
        printf("lilei getPropertyMultiScan() merge_yuv2yuv_for_yv12() 111 pfile:%p \n",pfile);
        fseek(pfile,0,SEEK_END); /* 定位到文件末尾 */
        int len= ftell(pfile);
        fseek(pfile,0,SEEK_SET); /* 定位到文件开头 */
        size_t result = fread(yuv420_buff,1,len,pfile); //读取mjpeg_buff

        dump_yuv(yuv420_buff,width_small*height_small*3/2,"out/qrd_image_nv21.yuv");
        Nv21ToYv12(yuv420_buff,yuv420_buff_for_yv12,width_small,height_small); //将nv21转码成yv12
        dump_yuv(yuv420_buff_for_yv12,width_small*height_small*3/2,"out/qrd_image_nv21_to_yv12.yuv");
        fclose(pfile);
        printf("lilei getPropertyMultiScan() merge_yuv2yuv_for_yv12() 222 mjpeg_buff:%p len:%d  \n",yuv420_buff,len);
        merge_two_yuv_for_yv12(camera_buff,width_big,height_big,yuv420_buff_for_yv12,width_small,height_small);
        free(yuv420_buff);
        free(yuv420_buff_for_yv12);

}
//将yuv420合并到nv21图像中
void merge_yuv2yuv_for_nv21(const char *jpegPath,int width_small,int height_small,unsigned char * camera_buff,int width_big,int height_big)
{
        int k = width_small;
        int j = height_small;

        //为yuv422转换为nv21数据分配内存
        yuv420_buff = (unsigned char*)malloc(width_small * height_small * 1.5);
	if(yuv420_buff ==  NULL){
		printf("lilei getPropertyMultiScan() merge_yuv2yuv_for_nv21() step3 yuv420_buff malloc yuv420_buff err\n");
	}else{
		memset(yuv420_buff, 0, width_small * height_small * 1.5);
	}

        FILE *pfile = fopen(jpegPath,"rb");
        printf("lilei getPropertyMultiScan() merge_yuv2yuv_for_nv21() 111 pfile:%p \n",pfile);
        fseek(pfile,0,SEEK_END); /* 定位到文件末尾 */
        int len= ftell(pfile);
        fseek(pfile,0,SEEK_SET); /* 定位到文件开头 */
        size_t result = fread(yuv420_buff,1,len,pfile); //读取mjpeg_buff

        dump_yuv(yuv420_buff,width_small*height_small*3/2,"out/qrd_image_yuv420.yuv");
        //YUV422ToYv12(yuyv_buff,yuv420_buff,width_small,height_small); //将yuyv422转码成yv12
        fclose(pfile);
        printf("lilei getPropertyMultiScan() merge_yuv2yuv_for_nv21() 222 mjpeg_buff:%p len:%d  \n",yuv420_buff,len);
        merge_two_yuv_for_nv21(camera_buff,width_big,height_big,yuv420_buff,width_small,height_small);
        free(yuv420_buff);

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
        char stryuvpath[80]={"camera_taobao_yv12_640x480.yuv"};
        FILE *pyuvfile = fopen(stryuvpath,"rb"); 
        fseek(pyuvfile,0,SEEK_END); /* 定位到文件末尾 */
        int lenyuv= ftell(pyuvfile);
        fseek(pyuvfile,0l,SEEK_SET); /* 定位到文件开头 */ 
        size_t result = fread(yuv420_buff_big,1,lenyuv,pyuvfile); 
        fclose(pyuvfile);
        printf("get_camera_buff() lenyuv:%d result:%d\n",lenyuv,result);
        
}
int init_buffer(){
	yuv420_buff_for_nv21 = (unsigned char*)malloc(WIDTH_BIG * HIGHT_BIG * 1.5);
	if(yuv420_buff_for_nv21 == NULL){
		perror("init_bufffer() 111 malloc yuv420_buff_for_nv21 err\n");
	}else{
		memset(yuv420_buff_for_nv21,0,WIDTH_BIG * HIGHT_BIG * 1.5);
	}
	yuv420_buff_for_yv12 = (unsigned char*)malloc(WIDTH_BIG * HIGHT_BIG * 1.5);
	if(yuv420_buff_for_yv12 == NULL){
			perror("init_bufffer() 222 malloc yuv420_buff_for_yv12 err\n");
		}else{
			memset(yuv420_buff_for_yv12,0,WIDTH_BIG * HIGHT_BIG * 1.5);
	}
	//char stryuvpath[80] = {"camera_nv21.yuv"};
	//char stryuvpath[80] = {"camera_yv12.yuv"};
	char stryuvpath[80] = {"camera_taobao_yv12_640x480.yuv"};
	FILE * fileYuv = fopen(stryuvpath,"rb");
	fseek(fileYuv,0,SEEK_END); /* 定位到文件末尾 */
	int lenyuv = ftell(fileYuv);
	fseek(fileYuv,0l,SEEK_SET); /* 定位到文件开头 */
	//size_t result = fread(yuv420_buff_for_nv21,1,lenyuv,fileYuv);
	size_t result = fread(yuv420_buff_for_yv12,1,lenyuv,fileYuv);
	fclose(fileYuv);
	printf("get_camera_buff() lenyuv:%d result:%d\n",lenyuv,result);
}
int test(){
        if(1){
          printf("lilei test gcc %d \n",__GNUC__);     
          return 0;
        }
	char MODEL[] ="GOME 2018M29A";
	char currentModel[] = "GOME 2018M29A2";
	int x = 0;
	int y = 0;
	getQrcodeOffset(&x,&y);
	printf("x:%d,y:%d",x,y);
	if(0 == strcmp(currentModel,MODEL)){
		printf("currentModel:%s, same as:%s",currentModel,MODEL);
	}else{
		printf("currentModel:%s, not same as:%s",currentModel,MODEL);
	}
	printf("testyuv.c test()  \n");
	init_buffer();
//	Nv21ToYv12(yuv420_buff_for_nv21,yuv420_buff_for_yv12,WIDTH_BIG,HIGHT_BIG);
//	dump_yuv(yuv420_buff_for_yv12,WIDTH_BIG*HIGHT_BIG*3/2,"out/Nv21ToYv12.yuv");
	Yv12ToNv21(yuv420_buff_for_yv12,yuv420_buff_for_nv21,WIDTH_BIG,HIGHT_BIG);
	dump_yuv(yuv420_buff_for_nv21,WIDTH_BIG*HIGHT_BIG*3/2,"out/camera_taobao_nv21_640x480.yuv");
}
int getQrcodeOffset(int *x,int *y){
	*x= 100;
	*y= 200;
}
void dumpToFile(unsigned char* buff,int bufSize,int width,int height,char strDumpFilePath[80],char subfix[10])
{
   FILE * pFile;
   //char strDumpFilePath[80]={"/sdcard/DCIM/dump_bgr_"};

   char strSize[20]={""};
   char strWidth[9]={""};
   char strHeight[9]={""};
   sprintf(strWidth,"%d",width);
   sprintf(strHeight,"%d",height);
   strcat(strSize,strWidth);
   strcat(strSize,"X");
   strcat(strSize,strHeight);
   strcat(strSize,"_"); 
 
   strcat(strDumpFilePath,strSize);

   clock_t t = clock();
   char strNum[10];
   sprintf(strNum, "%ld", t);
   strcat(strDumpFilePath,strNum);
   strcat(strDumpFilePath,".bgr");
   //strcat(strDumpFilePath,subfix);
   if((pFile = fopen(strDumpFilePath, "wb"))==NULL){
     printf("lilei dumpToFile error cant open the file: strDumpFilePath:%s \n",strDumpFilePath);
   }else{
     fwrite(buff,bufSize,1,pFile);
     printf("~~~lilei dumpToFile:%s bufSize:%d strNum:%s,strSize:%s,strWidth:%s end \n",strDumpFilePath,bufSize,strNum,strSize,strWidth);
   }
}
void testModifyBgr(){
    printf("lilei testModifyBgr() 000 \n");
   int width =  1280;
   int height = 720;
   unsigned char* bgr888_buff = NULL;
   //unsigned char* yv12_buff = NULL;
   char strBgrPath[80]={"dump_bgr_1280_720.bgr"};
   FILE *pbgrfile;
    printf("lilei testModifyBgr() 111 \n");
   if((pbgrfile = fopen(strBgrPath,"rb")) == NULL){
     printf("lilei testModifyBgr() open /sdcard/DCIM/dump_bgr_1280_720.bgr error !! \n");
     return;
   } 
   //从bgr文件读取bgr到变量 bgr888_buff   
   bgr888_buff =  (unsigned char*)malloc(width * height * 3);
   fseek(pbgrfile,0,SEEK_END);  
   int lenbgrFile= ftell(pbgrfile);
   fseek(pbgrfile,0,SEEK_SET); /* 定位到文件开头 */
   size_t result = fread(bgr888_buff,1,lenbgrFile,pbgrfile);
   fclose(pbgrfile);
   printf("lilei testModifyBgr() 222 lenbgrFile:%d \n",lenbgrFile);

   int modifyLen = 3*width*10;
   unsigned char* modifyData = (unsigned char*)malloc(modifyLen*sizeof(unsigned char));
   memset(modifyData,0,modifyLen);
   for(int i=0;i<modifyLen;){ //替换10行bgr数据为蓝色
      modifyData[i] = 0xff;
      //modifyData = modifyData+3;
      i=i+3;
   } 
    printf("lilei testModifyBgr() 333 \n");
    //memcpy(bgr888_buff,modifyData,modifyLen);
    printf("lilei testModifyBgr() 444 modifyLen:%d \n",modifyLen);
   //dump_yuv(modifyData,modifyLen,"out/dump_bgr_1280_720_modifyed.bgr");
   char strDumpFilePath[80]={"out/dump_bgr_"};
   char strSubfixp[] ={".bgr"};
   dumpToFile(modifyData,modifyLen,width,10,strDumpFilePath,strSubfixp);
}
/*截取src字符串中,从下标为start开始到end-1(end前面)的字符串保存在dest中(下标从0开始)*/
void substring(char *dest,char *src,int start,int end)
{
    int i=start;
    if(start>strlen(src))return;
    if(end>strlen(src))
        end=strlen(src);
    while(i<end)
    {
        dest[i-start]=src[i];
        i++;
    }
    dest[i-start]='\0';
    return;
}
/*返回str1中最后一次出现str2的位置(下标),不存在返回-1*/
int lastIndexOf(char *str1,char *str2)
{
    char *p=str1;
    int i=0,len=strlen(str2);
    p=strstr(str1,str2);
    if(p==NULL)return -1;
    while(p!=NULL)
    {
        for(;str1!=p;str1++)i++;
        p=p+len;
        p=strstr(p,str2);
    }
    return i;
}

int testSpliteString(){//从路径中解析出宽高
char str[] = {"/sdcard/DCIM/qrcode_300x300.yuv"};
int indexbegin = lastIndexOf(str,"_");
int indexend = lastIndexOf(str,".");
int length = indexend - indexbegin;
char subStr[length-1];
substring(subStr,str,indexbegin+1,indexend);
printf( "result indexbegin:%d,indexend:%d,length:%d,subStr:%s \n", indexbegin,indexend,length,subStr);
char delims[] = "x";
char *result = NULL;
result = strtok( subStr, delims );
if(result != NULL){
	printf( "width is:%s \n", result );
	result = strtok( NULL, delims );
}
if(result != NULL){
	printf( "height is:%s \n", result );
}
	//char str[] = "now # is the time for all # good men to come to the # aid of their country";
//	   char delims[] = "/";
//	   char * subStr = ".yuv"; //需要的子串
//	   char *result = NULL;
//	   result = strtok( str, delims );
//	   while( result != NULL ) {
//	       //printf( "result is \"%s\"\n", result );
//		   printf( "result is:%s \n", result );
//		   if(strstr(result,subStr)){
//			   printf( "result is:%s contains:%s \n", result, subStr);
//
//		   }
//	       result = strtok( NULL, delims );
//	   }

}
//新的yuv转bgr方法
static int Table_fv1[256] = { -180, -179, -177, -176, -174, -173, -172, -170, -169, -167, -166, -165, -163, -162, -160, -159, -158, -156, -155, -153, -152, -151, -149, -148, -146, -145, -144, -142, -141, -139, -138, -137,  -135, -134, -132, -131, -130, -128, -127, -125, -124, -123, -121, -120, -118, -117, -115, -114, -113, -111, -110, -108, -107, -106, -104, -103, -101, -100, -99, -97, -96, -94, -93, -92, -90,  -89, -87, -86, -85, -83, -82, -80, -79, -78, -76, -75, -73, -72, -71, -69, -68, -66, -65, -64,-62, -61, -59, -58, -57, -55, -54, -52, -51, -50, -48, -47, -45, -44, -43, -41, -40, -38, -37,  -36, -34, -33, -31, -30, -29, -27, -26, -24, -23, -22, -20, -19, -17, -16, -15, -13, -12, -10, -9, -8, -6, -5, -3, -2, 0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14, 15, 16, 18, 19, 21, 22, 23, 25, 26, 28, 29, 30, 32, 33, 35, 36, 37, 39, 40, 42, 43, 44, 46, 47, 49, 50, 51, 53, 54, 56, 57, 58, 60, 61, 63, 64, 65, 67, 68, 70, 71, 72, 74, 75, 77, 78, 79, 81, 82, 84, 85, 86, 88, 89, 91, 92, 93, 95, 96, 98, 99, 100, 102, 103, 105, 106, 107, 109, 110, 112, 113, 114, 116, 117, 119, 120, 122, 123, 124, 126, 127, 129, 130, 131, 133, 134, 136, 137, 138, 140, 141, 143, 144, 145, 147, 148,  150, 151, 152, 154, 155, 157, 158, 159, 161, 162, 164, 165, 166, 168, 169, 171, 172, 173, 175, 176, 178 };
static int Table_fv2[256] = { -92, -91, -91, -90, -89, -88, -88, -87, -86, -86, -85, -84, -83, -83, -82, -81, -81, -80, -79, -78, -78, -77, -76, -76, -75, -74, -73, -73, -72, -71, -71, -70, -69, -68, -68, -67, -66, -66, -65, -64, -63, -63, -62, -61, -61, -60, -59, -58, -58, -57, -56, -56, -55, -54, -53, -53, -52, -51, -51, -50, -49, -48, -48, -47, -46, -46, -45, -44, -43, -43, -42, -41, -41, -40, -39, -38, -38, -37, -36, -36, -35, -34, -33, -33, -32, -31, -31, -30, -29, -28, -28, -27, -26, -26, -25, -24, -23, -23, -22, -21, -21, -20, -19, -18, -18, -17, -16, -16, -15, -14, -13, -13, -12, -11, -11, -10, -9, -8, -8, -7, -6, -6, -5, -4, -3, -3, -2, -1, 0, 0, 1, 2, 2, 3, 4, 5, 5, 6, 7, 7, 8, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 17, 18, 19, 20, 20, 21, 22, 22, 23, 24, 25, 25, 26, 27, 27, 28, 29, 30, 30, 31, 32, 32, 33, 34, 35, 35, 36, 37, 37, 38, 39, 40, 40, 41, 42, 42, 43, 44, 45, 45, 46, 47, 47, 48, 49, 50, 50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 57, 58, 59, 60, 60, 61, 62, 62, 63, 64, 65, 65, 66, 67, 67, 68, 69, 70, 70, 71, 72, 72, 73, 74, 75, 75, 76, 77, 77, 78, 79, 80, 80, 81, 82, 82, 83, 84, 85, 85, 86, 87, 87, 88, 89, 90, 90 };
static int Table_fu1[256] = { -44, -44, -44, -43, -43, -43, -42, -42, -42, -41, -41, -41, -40, -40, -40, -39, -39, -39, -38, -38, -38, -37, -37, -37, -36, -36, -36, -35, -35, -35, -34, -34, -33, -33, -33, -32, -32, -32, -31, -31, -31, -30, -30, -30, -29, -29, -29, -28, -28, -28, -27, -27, -27, -26, -26, -26, -25, -25, -25, -24, -24, -24, -23, -23, -22, -22, -22, -21, -21, -21, -20, -20, -20, -19, -19, -19, -18, -18, -18, -17, -17, -17, -16, -16, -16, -15, -15, -15, -14, -14, -14, -13, -13, -13, -12, -12, -11, -11, -11, -10, -10, -10, -9, -9, -9, -8, -8, -8, -7, -7, -7, -6, -6, -6, -5, -5, -5, -4, -4, -4, -3, -3, -3, -2, -2, -2, -1, -1, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 29, 29, 29, 30, 30, 30, 31, 31, 31, 32, 32, 33, 33, 33, 34, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 40, 40, 40, 41, 41, 41, 42, 42, 42, 43, 43 };
static int Table_fu2[256] = { -227, -226, -224, -222, -220, -219, -217, -215, -213, -212, -210, -208, -206, -204, -203, -201, -199, -197, -196, -194, -192, -190, -188, -187, -185, -183, -181, -180, -178, -176, -174, -173, -171, -169, -167, -165, -164, -162, -160, -158, -157, -155, -153, -151, -149, -148, -146, -144, -142, -141, -139, -137, -135, -134, -132, -130, -128, -126, -125, -123, -121, -119, -118, -116, -114, -112, -110, -109, -107, -105, -103, -102, -100, -98, -96, -94, -93, -91, -89, -87, -86, -84, -82, -80, -79, -77, -75, -73, -71, -70, -68, -66, -64, -63, -61, -59, -57, -55, -54, -52, -50, -48, -47, -45, -43, -41, -40, -38, -36, -34, -32, -31, -29, -27, -25, -24, -22, -20, -18, -16, -15, -13, -11, -9, -8, -6, -4, -2, 0, 1, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19, 21, 23, 24, 26, 28, 30, 31, 33, 35, 37, 39, 40, 42, 44, 46, 47, 49, 51, 53, 54, 56, 58, 60, 62, 63, 65, 67, 69, 70, 72, 74, 76, 78, 79, 81, 83, 85, 86, 88, 90, 92, 93, 95, 97, 99, 101, 102, 104, 106, 108, 109, 111, 113, 115, 117, 118, 120, 122, 124, 125, 127, 129, 131, 133, 134, 136, 138, 140, 141, 143, 145, 147, 148, 150, 152, 154, 156, 157, 159, 161, 163, 164, 166, 168, 170, 172, 173, 175, 177, 179, 180, 182, 184, 186, 187, 189, 191, 193, 195, 196, 198, 200, 202, 203, 205, 207, 209, 211, 212, 214, 216, 218, 219, 221, 223, 225 };

int YV12ToBGR24_Table(unsigned char* pYUV,unsigned char* pBGR24,int width,int height)
{
    if (width < 1 || height < 1 || pYUV == NULL || pBGR24 == NULL)
        return 0;
    const long len = width * height;
    unsigned char* yData = pYUV;
    unsigned char* vData = &yData[len];
    unsigned char* uData = &vData[len >> 2];
    printf("lilei YV12ToBGR24_Table begin");
    int bgr[3];
    int yIdx,uIdx,vIdx,idx;
    int rdif,invgdif,bdif;
    for (int i = 0;i < height;i++){
        for (int j = 0;j < width;j++){
            yIdx = i * width + j;
            vIdx = (i/2) * (width/2) + (j/2);
            uIdx = vIdx;
            
            rdif = Table_fv1[vData[vIdx]];
            invgdif = Table_fu1[uData[uIdx]] + Table_fv2[vData[vIdx]];
            bdif = Table_fu2[uData[uIdx]];

            bgr[0] = yData[yIdx] + bdif;    
            bgr[1] = yData[yIdx] - invgdif;
            bgr[2] = yData[yIdx] + rdif;

            for (int k = 0;k < 3;k++){
                idx = (i * width + j) * 3 + k;
                if(bgr[k] >= 0 && bgr[k] <= 255)
                    pBGR24[idx] = bgr[k];
                else
                    pBGR24[idx] = (bgr[k] < 0)?0:255;
            }
        }
    }
    return 1;
}
void testYuv2Bgr_yv12(){
    
   printf("lilei testYuv2Bgr_yv12() 000 \n");
   int width =  640;
   int height = 480;
   unsigned char* pBGR24 = NULL;
   unsigned char* yv12_buff = NULL;
   char strYuvPath[80]={"camera_taobao_yv12_640x480.yuv"};
   FILE *pYuvfile;
    printf("lilei testYuv2Bgr_yv12() 111 \n");
   if((pYuvfile = fopen(strYuvPath,"rb")) == NULL){
     printf("lilei testYuv2Bgr_yv12() open camera_taobao_yv12_640x480.yuv  error !! \n");
     return;
   } 
   //从bgr文件读取bgr到变量 bgr888_buff   
   pBGR24 =  (unsigned char*)malloc(width * height * 3);
   yv12_buff = (unsigned char*)malloc(width * height * 1.5);
   fseek(pYuvfile,0,SEEK_END);  
   int lenyuvFile= ftell(pYuvfile);
   fseek(pYuvfile,0,SEEK_SET); /* 定位到文件开头 */
   size_t result = fread(yv12_buff,1,lenyuvFile,pYuvfile);
   fclose(pYuvfile);
   printf("lilei testYuv2Bgr_yv12() 222 lenyuvFile:%d \n",lenyuvFile);
   
   YV12ToBGR24_Table(yv12_buff,pBGR24,width,height);
   
   char strDumpFilePath[80]={"out/dump_bgr_"};
   char strSubfixp[] ={".bgr"};
   dumpToFile(yv12_buff,width * height * 1.5,width,10,strDumpFilePath,strSubfixp);

   if(yv12_buff != NULL){
      free(yv12_buff);
   }
   if(pBGR24 != NULL){
         free(pBGR24);
   }
}
int main()
{
	printf("lilei testyuv.c main() 000 \n");
    //testModifyBgr();
	//testYuv2Bgr_yv12();
	//testString();
	test();
//        get_camera_buff();
//
//        merge_yuv2yuv_for_yv12("image1_320x280.yuv",320,280,yuv420_buff_big,WIDTH_BIG,HIGHT_BIG);

//        merge_yuv2yuv_for_nv21("image1_320x280.yuv",320,280,yuv420_buff_big,WIDTH_BIG,HIGHT_BIG);

        //merge_jpeg2yuv_for_yv12("1.jpg",320,240,yuv420_buff_big,WIDTH_BIG,HIGHT_BIG);
        //merage_jpeg2yuv_for_nv21("1_300x200.jpg",640,480,yuv420_buff_big,WIDTH_BIG,HIGHT_BIG);
}
