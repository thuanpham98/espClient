#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/can.h"

static const char *TAG="QUEUE_TEST";

QueueHandle_t my_queue;
SemaphoreHandle_t my_sema;

typedef struct
{
    char header[15];
    uint32_t ID;
    float val1;
} frame ;


void task1(void *pv)
{
    ESP_LOGI(TAG,"start task1");
    frame data_send;
    uint8_t buffer[sizeof(frame)+1];
    memcpy(data_send.header,"thuandeptrai69",sizeof(data_send.header));
    data_send.ID=1232154;
    
    data_send.val1=12.21;
    memcpy(buffer,&data_send,sizeof(frame));
    xQueueSendToBack(my_queue,(void *)buffer,100/portTICK_PERIOD_MS);
    
    data_send.val1 ++;
    memcpy(buffer,&data_send,sizeof(frame));
    xQueueSendToBack(my_queue,(void *)buffer,100/portTICK_PERIOD_MS);
    
    // data_send.val1 ++;
    // memcpy(buffer,&data_send,sizeof(frame));
    // xQueueSendToBack(my_queue,(void *)buffer,100/portTICK_PERIOD_MS);

    while (1)
    {
        if(xSemaphoreTakeRecursive(my_sema,100/portTICK_PERIOD_MS))
        {
            ESP_LOGI(TAG,"send done nha");
            xSemaphoreGiveRecursive(my_sema);
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
        
    }
    
}
void todo(void *pv)
{
    uint8_t buffer[sizeof(frame)+1];
    frame data_receive;
    if(xQueueReceive(my_queue,(void *)buffer,portMAX_DELAY))
    {
        memcpy(&data_receive,buffer,sizeof(frame));
        ESP_LOGI(TAG,"to do value : %f",data_receive.val1);
    }
    else
    {
        ESP_LOGE(TAG,"do not receive anything");
    }
    ESP_LOGI(TAG,"Done todo");
    vTaskDelete(NULL);
}
void task2(void *pv)
{
    ESP_LOGI(TAG,"start task2");
    uint8_t buffer[sizeof(frame)+1];
    frame data_receive;
    while (1)
    {
        if(my_queue !=0)
        {
            if(xQueueReceive(my_queue,(void *)buffer,portMAX_DELAY))
            {
                xSemaphoreGiveRecursive(my_sema);
                memcpy(&data_receive,buffer,sizeof(frame));
                ESP_LOGI(TAG,"value : %f",data_receive.val1);
                if(xSemaphoreTakeRecursive(my_sema,portMAX_DELAY))
                {
                    xTaskCreate(todo,"todo",4096,NULL,4,NULL);
                    xSemaphoreGiveRecursive(my_sema);
                }   
            }
        }

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}


void app_main(void)
{
    my_sema=xSemaphoreCreateMutex();
    my_queue=xQueueCreate(3,sizeof(frame)+1);

    xTaskCreate(task1,"task1",4096*2,NULL,3,NULL);
    xTaskCreate(task2,"task2",4096*2,NULL,3,NULL);
    
    
    // xTaskCreate(task3,"task3",4096*2,NULL,3,NULL);
     
}
