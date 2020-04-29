#include "include/converttime.h"

uint64_t date_to_timestamp(uint16_t s,uint16_t m,uint16_t h,uint16_t D,uint16_t M, uint16_t Y,uint16_t timezone)
{
    uint64_t subtimestamp[6]={0,0,0,0,0,0};

    /* convert year */
    for(int i = 1970 ; i< Y;i++)
    {
        if((i%400==0) || ((i%100!=0) && (i%4==0)))
        {
            subtimestamp[5]+=366*24*3600;
        }
        else subtimestamp[5]+=365*24*3600;

    }

    /* convert month */
    for(int i =1 ; i < M;i++)
    {
        if((i==1)||(i==3)||(i==5)||(i==7)||(i==8)||(i==10)||(i==12))
        {
            subtimestamp[4] += 31*24*3600;
        }
        else if(i==2)
        {
            if((Y %400==0) || ((Y %4==0) && (Y %100!=0)))
            {
                subtimestamp[4]+=29*24*3600;
            }
            else subtimestamp[4]+=28*24*3600;
            
        }
        else subtimestamp[4]+=30*24*3600;
    }

    /* convert day */
    for(int i = 1 ; i < D;i++)
    {
        subtimestamp[3]+=24*3600;
    }

    /* convert hour */
    for(int i=0;i < h;i++)
    {
        subtimestamp[2]+=3600;
    }

    /* convert minute */
    for(int i=0;i < m;i++)
    {
        subtimestamp[1] +=60;
    }

    /* convert secon */
    for(int i=0;i < s;i++)
    {
        subtimestamp[0]+=1;
    }

    return (subtimestamp[0]+subtimestamp[1]+subtimestamp[2]+subtimestamp[3]+subtimestamp[4]+subtimestamp[5]-(uint64_t)(timezone*3600));
}