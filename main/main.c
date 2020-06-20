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

static const char *TAG="semaphot";

// void master(void *pv)
// {
//     //Configure message to transmit
//     can_message_t message;
//     message.identifier = 0x123;
//     message.data_length_code = 8;
//     message.flags=CAN_MSG_FLAG_NONE;
//     for (int i = 0; i < 8; i++) {
//         message.data[i] = i;
//     }
//     while (1)
//     {
//         //Queue message for transmission
//         if (can_transmit(&message, 100/portTICK_PERIOD_MS) == ESP_OK) {
//             ESP_LOGI(TAG,"Message queued for transmission");
//             can_clear_transmit_queue();
//         } else {
//             ESP_LOGE(TAG,"Failed to queue message for transmission");
//         }
//         vTaskDelay(1000/portTICK_PERIOD_MS);
//     }
//     vTaskDelete(NULL);
// }

void slave(void *pv)
{
    can_message_t message;
    while (1)
    {
        can_clear_receive_queue();
        if(can_receive(&message,portMAX_DELAY)==ESP_OK)
        {
            ESP_LOGI(TAG,"%s",message.data);
        }
        else
        {
            ESP_LOGE(TAG,"waiting message");
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    //Initialize configuration structures using macro initializers
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, CAN_MODE_NORMAL);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
    can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

    //Install CAN driver
    if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGI(TAG,"Driver installed");
    } else {
        ESP_LOGE(TAG,"Failed to install driver");
        return;
    }

    //Start CAN driver
    if (can_start() == ESP_OK) {
        ESP_LOGI(TAG,"Driver started");
    } else {
        ESP_LOGE(TAG,"Failed to start driver");
        return;
    }

    // xTaskCreate(master,"master",4096*2,NULL,3,NULL);
    xTaskCreate(slave,"slave",4096*2,NULL,3,NULL);
}
