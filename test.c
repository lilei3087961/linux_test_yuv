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

int bobSort(int a[],int len){
  for(int i = len-1;i>0;i--){
    for(int j=0;j<i;j++){
      if(a[j]>a[j+1]){
         int temp = a[j+1];
         a[j+1] = a[j];
         a[j] =  temp;
      }
    }
  }
}
int selSort(int a[],int len){
  for(int i=0;i<len;i++){
    int minIndex = i;
    for(int j=i+1;j<len;j++){
       if(a[j]<a[i]){
         minIndex = j;     
       }
    }
    if(minIndex != i){
       int temp = a[i];
       a[i]= a[minIndex];
      a[minIndex] = temp;
    }
  }
}
int insSort(int a[],int len){
  for(int i=1;i<len;i++){
    for(int j=i;j>0;j--){
      if(a[j]<a[j-1]){
        int temp = a[j];
        a[j] = a[j-1];
        a[j-1] = temp;
      }
    }
  }  
}
int quSort(int a[],int low,int high){
  int l=low;
  int h=high;
  int pivot = a[l];
  if(l>=h)
   return;
  while(l<h){
    while(a[h]>=pivot && l<h){
      h--;
    }
    if(l<h){
     int temp = a[l];
     a[l] = a[h];
     a[h]= temp;
    }
    
    while(a[l]<=pivot && l<h){
      l++;
    }
    if(l<h){
      int temp = a[l];
      a[l] = a[h];
      a[h]= temp;
    }
  }
  printf("quSort low:%d,high:%d,l:%d,h:%d \n",low,high,l,h);
  if(l>low){
    quSort(a,low,l-1);
  }
  if(h<high){
    quSort(a,h+1,high);
  }
}
typedef struct node{
  struct node * lchild;
  struct node * rchild;
  int data;
}BiNode;
BiNode * makeNode(int data,BiNode *lchild,BiNode * rchild){
  BiNode * node  = (BiNode *)malloc(sizeof(BiNode));
  node->lchild = lchild;
  node->rchild = rchild;
  node->data = data;
}
void preOrder(BiNode *root){
  if(root){
    printf("preOrder:%d \n",root->data);
    preOrder(root->lchild);
    preOrder(root->rchild);
  }
}
void midOrder(BiNode *root){
  if(root){
    midOrder(root->lchild);
    printf("midOrder:%d \n",root->data);
    midOrder(root->rchild);
  }
}
void printArr(int arr[],int len){
  for(int i=0;i<len;i++){
     printf("%d,",arr[i]);
  }
  printf("\n");
}
#define SIZE 7
int main(){

  printf("test lilei btree test\n");
  int a [SIZE] = {4,2,3,9,7,1,8};
  printArr(a,SIZE);
  BiNode * nodes[SIZE] = {NULL};
  for(int i=SIZE-1;i>=0;i--){
    if(2*i+2<SIZE){
      printf("makeNode (%d,%d,%d) i:%d \n",a[i],nodes[2*i+1]->data,nodes[2*i+2]->data,i);
      nodes[i] = makeNode(a[i],nodes[2*i+1],nodes[2*i+2]); 
    }else if(2*i+1<SIZE){
      printf("makeNode (%d,%d,NULL) i:%d \n",a[i],nodes[2*i+1]->data,i);
      nodes[i] = makeNode(a[i],nodes[2*i+1],NULL);
    }else{
      printf("makeNode (%d,NULL,NULL) i:%d \n",a[i],i);
      nodes[i] = makeNode(a[i],NULL,NULL);
    }
  }  
  midOrder(nodes[0]);
  for(int i=0;i<SIZE;i++){ //release struct
    free(nodes[i]);
    nodes[i] = NULL; 
  }
  //preOrder(nodes[0]);  
/*  int a [6] = {4,2,3,9,7,1};
  printArr(a,6);

  //bobSort(a,6);
 //selSort(a,6);
 //insSort(a,6);
 quSort(a,0,5);
  
  printArr(a,6);  */
  
}
