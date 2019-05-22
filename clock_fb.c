#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

const u_int32_t CIRCLE_COLOR=0xffffffu;//white
const u_int32_t HOUR_COLOR=0xff0000u;//red
const u_int32_t MINUTE_COLOR=0x008000u;//green
const u_int32_t SECOND_COLOR=0x0000ffu;//blue
const double PI=3.1415926;

struct image{
    u_int32_t **data;
    u_int32_t rows;
    u_int32_t cols;
};

void initImage(struct image *pImage,u_int32_t rows,u_int32_t cols){
    pImage->data=(u_int32_t**)malloc(sizeof(u_int32_t*)*rows);
    for(size_t i=0;i<rows;++i){
        pImage->data[i]=(u_int32_t*)malloc(sizeof(u_int32_t)*cols);
        memset(pImage->data[i],0,sizeof(u_int32_t)*cols);
    }
    pImage->rows=rows;
    pImage->cols=cols;
}

void resetImage(struct image *pImage){
    for(u_int32_t i=0;i<pImage->rows;++i){
        memset(pImage->data[i],0,sizeof(u_int32_t)*pImage->cols);
    }
}

void initCircle(struct image *pCircle,u_int32_t x0, u_int32_t y0, u_int32_t radius){
    double radians;
    u_int32_t x,y;
    for(int i=0;i<3600;++i){
        radians=i*2*PI/3600;
        x=x0-(u_int32_t)(radius*sin(radians));
        y=y0+(u_int32_t)(radius*cos(radians));
        pCircle->data[x][y]=CIRCLE_COLOR;
    }
}

void add(struct image *pBoard,struct image *pImage){
    for(u_int32_t x=0;x<pBoard->rows;++x){
        for(u_int32_t y=0;y<pBoard->cols;++y){
            pBoard->data[x][y] |= pImage->data[x][y];
        }
    }
}

void initTime(struct image *pHour,struct image *pMinute,struct image *pSecond,u_int32_t radius){
    const int secondLength=radius*0.618;
    const int minuteLength=secondLength*0.618;
    const int hourLength=minuteLength*0.618;

    time_t t=time(NULL);
    struct tm *pTm=localtime(&t);

    double secondRadiansBase12=pTm->tm_sec*2*PI/60;
    double minuteRadiansBase12=pTm->tm_min*2*PI/60;
    double hourRadiansBase12=(pTm->tm_hour%12)*2*PI/12;
    
    u_int32_t x0=pHour->rows/2;
    u_int32_t y0=pHour->cols/2;

    for(int i=0;i<secondLength;++i){
        pSecond->data[x0-(u_int32_t)(i*cos(secondRadiansBase12))][y0+(u_int32_t)(i*sin(secondRadiansBase12))]=SECOND_COLOR;
    }

    for(int i=0;i<minuteLength;++i){
        pMinute->data[x0-(u_int32_t)(i*cos(minuteRadiansBase12))][y0+(u_int32_t)(i*sin(minuteRadiansBase12))]=MINUTE_COLOR;
    }

    for(int i=0;i<hourLength;++i){
        pHour->data[x0-(u_int32_t)(i*cos(hourRadiansBase12))][y0+(u_int32_t)(i*sin(hourRadiansBase12))]=HOUR_COLOR;
    }
}

int main(){
    int fb=open("/dev/fb0",O_RDWR);
    if(fb==-1){
        perror("open");
        exit(1);
    }
    struct fb_fix_screeninfo info;
    if(0!=ioctl(fb,FBIOGET_FSCREENINFO,&info)){
        perror("ioctl");
        exit(1);
    }
    u_int32_t cols=info.line_length/4;
    u_int32_t rows=info.smem_len/info.line_length;
    
    u_int32_t *buf=mmap(NULL,4*rows*cols,PROT_READ|PROT_WRITE,MAP_SHARED,fb,0);
    if(buf==MAP_FAILED){
        perror("mmap");
        exit(1);
    }
    printf("%d %d\n",rows,cols);
    struct image board,circle,hour,minute,second;
    initImage(&board,rows,cols);
    initImage(&circle,rows,cols);
    initImage(&hour,rows,cols);
    initImage(&minute,rows,cols);
    initImage(&second,rows,cols);
    //printf("initCircle start.\n");
    initCircle(&circle,rows/2,cols/2,(rows<cols?rows:cols)/2*0.618);
    //printf("initCircle over.\n");
    while(1){
        resetImage(&board);
        resetImage(&hour);
        resetImage(&minute);
        resetImage(&second);
        add(&board,&circle);
        initTime(&hour,&minute,&second,(rows<cols?rows:cols)/2*0.618);
        add(&board,&hour);
        add(&board,&minute);
        add(&board,&second);
        for(size_t i=0;i<rows;++i){
            for(size_t j=0;j<cols;++j){
                buf[i*cols+j]=board.data[i][j];
            }
        }
    }
    return 0;
}
