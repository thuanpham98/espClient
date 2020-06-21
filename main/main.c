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
#include "driver/gpio.h"

#include "esp_intr_alloc.h"
#include "soc/soc.h"
static const char *TAG="INTR_TEST";

QueueHandle_t my_queue;
SemaphoreHandle_t my_sema;

intr_handle_t my_intr;
intr_handle_t my_intr_ex;

typedef struct
{
    char header[15];
    uint32_t ID;
    float val1;
} frame ;
int count =0;
void gpio_ex_idrq(void *para)
{
    count ++;
}

void task1(void *pv)
{
    ESP_LOGI(TAG,"start task1");
    while (1)
    {

        vTaskDelay(1000/portTICK_PERIOD_MS);
        
    }
    
}

void task2(void *pv)
{
    ESP_ERROR_CHECK(esp_intr_enable(my_intr_ex));

    int a;
    // a= esp_intr_get_cpu(my_intr);
    // ESP_LOGI(TAG,"%d",a);
    a=esp_intr_get_cpu(my_intr_ex);
    ESP_LOGI(TAG,"%d",a);

    gpio_config_t cf ={
        .pin_bit_mask=1 <<GPIO_NUM_21,     
        .mode=GPIO_MODE_INPUT,               
        .pull_up_en=0,   
        .pull_down_en=0,                                    
        .intr_type=GPIO_INTR_HIGH_LEVEL     
    };
    gpio_config(&cf);
    while (1)
    {
        ESP_LOGI(TAG,"task2 do to");

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

void todo(void *pv)
{
        while (1)
    {

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }

}

void app_main(void)
{
    
    // ESP_ERROR_CHECK(esp_intr_alloc(ETS_INTERNAL_TIMER1_INTR_SOURCE,ESP_INTR_FLAG_LEVEL2,NULL,NULL,&my_intr));
    ESP_ERROR_CHECK(esp_intr_alloc(ETS_GPIO_INTR_SOURCE,ESP_INTR_FLAG_LEVEL3,NULL,NULL,&my_intr_ex));
    // ESP_ERROR_CHECK(esp_intr_enable(my_intr));



    xTaskCreatePinnedToCore(task1,"task1",4096*2,NULL,3,NULL,1);
    xTaskCreatePinnedToCore(task2,"task2",4096*2,NULL,3,NULL,0);
    xTaskCreate(todo,"todo",4096,NULL,3,NULL);
     
}
