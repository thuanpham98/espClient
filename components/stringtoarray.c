#include "include/stringtoarray.h"

uint32_t Arr(char* str,uint8_t *buff)
{
    uint32_t j=0,k=0;
    char *frame = (char *) malloc(10*(sizeof(char)));

    for (int i=0;i < strlen(str);i++)
    {
        if(*(str+i)!=',')
        {
            *(frame+j)=*(str+i);
            j++;
        }

        if((*(str+i+1)==',') || (*(str+i+1)== NULL) )
        {
            *(frame+j)=NULL;
            buff[k] = (uint8_t)atoi(frame);

            k++;
            j=0;
        }
    }
    free(frame);
    return k;
}