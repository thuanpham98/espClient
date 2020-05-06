#include "driver/gpio.h"
#include "driver/can.h"

void app_main()
{
    //Initialize configuration structures using macro initializers
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_5, GPIO_NUM_4, CAN_MODE_NORMAL);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_1MBITS();
    can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

    //Install CAN driver
    if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }

    //Start CAN driver
    if (can_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        return;
    }

    //Configure message to transmit
    can_message_t message;
    message.identifier = 0x123;
    message.data_length_code = 8;
    for (int i = 0; i < 8; i++) {
        message.data[i] = i;
    }
    while(1)
    {
        //Queue message for transmission
        if (can_transmit(&message, portMAX_DELAY) == ESP_OK) {
            printf("Message queued for transmission\n");
        } else {
            printf("Failed to queue message for transmission\n");
        }
    }


}