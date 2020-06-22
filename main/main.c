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
#include "freertos/event_groups.h"

static const char *TAG="evenBit_TEST";
EventGroupHandle_t group_event;
#define BIT_CONNECT BIT0
#define BIT_TODO BIT2
#define BIT_DISCONN BIT3

void task1(void *pv)
{
    group_event= xEventGroupCreate();

    if( group_event == NULL )
    {
        ESP_LOGE(TAG,"The event group was not created because there was insufficient FreeRTOS heap available.");
    }
    else
    {
        ESP_LOGI(TAG,"The event group was created.");
    }

    EventBits_t wait_bit;
    wait_bit= xEventGroupWaitBits(group_event,BIT_CONNECT|BIT_TODO,pdFAIL,pdTRUE,portMAX_DELAY);

    if( ( wait_bit & ( BIT_CONNECT| BIT_TODO ) ) == ( BIT_CONNECT| BIT_TODO ) )
    {
        ESP_LOGI(TAG,"xEventGroupWaitBits() returned because both bits were set.");
    }
    else if( ( wait_bit & BIT_CONNECT) != 0 )
    {
        ESP_LOGI(TAG,"xEventGroupWaitBits() returned because just BIT_0 was set.");
    }
    else if( ( wait_bit & BIT_TODO ) != 0 )
    {
        ESP_LOGI(TAG,"xEventGroupWaitBits() returned because just BIT_4 was set. ");
    }
    else
    {
        ESP_LOGI(TAG,"xEventGroupWaitBits() returned because xTicksToWait ticks passedwithout either BIT_0 or BIT_4 becoming set.");
    }
    ESP_LOGI(TAG,"%d",wait_bit);
    ESP_LOGI(TAG,"%d",BIT_CONNECT);

    ESP_LOGI(TAG,"start task1");
    while (1)
    {
        ESP_LOGI(TAG,"task1");
        wait_bit=xEventGroupGetBits(group_event);
        ESP_LOGI(TAG,"%d",wait_bit);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        
    }
    
}
void todo(void *pv)
{
    ESP_LOGI(TAG,"start task todo");
    int count=0;
    while (1)
    {
        ESP_LOGI(TAG,"todo");
        count++;
        if(count==5)
        {
            xEventGroupSetBits(group_event,BIT_CONNECT);
            
            ESP_LOGI(TAG,"done set bit");
        }
        if(count==10)
        {
            xEventGroupSetBits(group_event,BIT_TODO);
            xEventGroupClearBits(group_event,BIT_TODO|BIT_CONNECT);
        }

        vTaskDelay(1000/portTICK_PERIOD_MS);
        
    }
    vTaskDelete(NULL);
}


void app_main(void)
{

    xTaskCreate(task1,"task1",4096*2,NULL,3,NULL);
    xTaskCreate(todo,"todo",4096*2,NULL,3,NULL);

}
