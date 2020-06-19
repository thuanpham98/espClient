#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"

char  *TAG_TEST = "THUAN"; 

typedef struct
{
    char header[12] __attribute__((packed));
    double num1 __attribute__((packed));
    double data[4] __attribute__((packed)); 
    uint32_t mycrc __attribute__((packed));
    int8_t pos __attribute__((packed));
}frame;


void app_main(void)
{
    frame test1,test2;
    char *header="hello";
    double data[4]={12.424,42.12,45.22,22.11};

    memcpy(test1.header,header,sizeof(test1.header));
    test1.num1=12.321;
    memcpy(test1.data,data,sizeof(test1.data));
    test1.mycrc=12351;
    test1.pos=-5;

    ESP_LOGE(TAG_TEST,"%s",test1.header);
    ESP_LOGE(TAG_TEST,"%lf",test1.num1);
    ESP_LOGE(TAG_TEST,"%d",test1.mycrc);
    ESP_LOGE(TAG_TEST,"%lf",test1.data[0]);
    ESP_LOGE(TAG_TEST,"%lf",test1.data[1]);
    ESP_LOGE(TAG_TEST,"%lf",test1.data[2]);
    ESP_LOGE(TAG_TEST,"%lf",test1.data[3]);
    ESP_LOGE(TAG_TEST,"%d",test1.pos);

    uint8_t data_send[sizeof(frame)+1];
    memcpy(data_send,&test1,sizeof(frame));
    ESP_LOGI(TAG_TEST,"%d",sizeof(frame));

    for (int i = 0; i < sizeof(frame); i++)
    {
        ESP_LOGI(TAG_TEST,"%d",data_send[i]);
    }

    ESP_LOGE(TAG_TEST,"------------------------");

    memcpy(&test2,data_send,sizeof(frame));

    ESP_LOGE(TAG_TEST,"------------------------");
    ESP_LOGE(TAG_TEST,"%s",test2.header);
    ESP_LOGE(TAG_TEST,"%lf",test2.num1);
    ESP_LOGE(TAG_TEST,"%d",test2.mycrc);
    ESP_LOGE(TAG_TEST,"%lf",test2.data[0]);
    ESP_LOGE(TAG_TEST,"%lf",test2.data[1]);
    ESP_LOGE(TAG_TEST,"%lf",test2.data[2]);
    ESP_LOGE(TAG_TEST,"%lf",test2.data[3]);
    ESP_LOGE(TAG_TEST,"%d",test2.pos);
    
}