/** Coppy All Right: Phạm Minh Thuận (Vietnamese Person)
 * This is full code for controll on/off, controll PWM , controll DAC with http GET Method
 * Because only me build it, it is not the best perform
 * this is my thesis for graduate im summer 2020, and free open source
 * help me make it perfect on github https://github.com/thuanpham98/espClient.git
 */

/* Basic library on C */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* library for Freetos API */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

/* library for wifi and event of system */
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/uart.h"

static const char *TAG="test_uart";

typedef struct{
    uint32_t HEAD __attribute__((packed)) __attribute__((aligned()));
    float num1 __attribute__((packed)) __attribute__((aligned(4)));
    float num2 __attribute__((packed)) __attribute__((aligned(4)));
    uint8_t num3[10] __attribute__((packed)) __attribute__((aligned()));
    char name[25] __attribute__((packed)) __attribute__((aligned()));
} DATA_send;

void app_main(void)
{
    char buffer[sizeof(DATA_send) + 1];
    DATA_send test,test2;
    
    test.HEAD=134;
    test.num1=23.54;
    test.num2=34.23;
    test.num3[0]=12;
    test.name[0]='g';



    memcpy( buffer, &test, sizeof(DATA_send));
    for (int  i = 0; i < sizeof(DATA_send) + 1; i++)
    {
        ESP_LOGE(TAG,"%d",buffer[i]);
    }

    char *buffer2 =(char *)malloc((sizeof(DATA_send) + 1)*sizeof(char));

    buffer2=buffer;

    memcpy( &test2, buffer2, sizeof(DATA_send));
    ESP_LOGE(TAG,"%0.2f",test2.num1);


}