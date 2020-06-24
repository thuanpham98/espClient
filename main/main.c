#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

char  *TAG_TEST = "USART"; 
static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_21)
#define RXD_PIN (GPIO_NUM_22)

SemaphoreHandle_t my_sema;

void tx_task(void *pv)
{
    char data[50] ="hellothuan\r\n";
    char *send=data;
    while(1)
    {
        if(xSemaphoreTakeRecursive(my_sema,500/portTICK_PERIOD_MS))
        {
            uart_write_bytes(UART_NUM_2, send, sizeof(data));
            xSemaphoreGiveRecursive(my_sema);
        }
        
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

void rx_task(void *pv)
{

    uint8_t data[RX_BUF_SIZE+1];
    uint8_t *receive=data;
    while (1) 
    {
        if(xSemaphoreTakeRecursive(my_sema,100/portTICK_PERIOD_MS))
        {
            memset(data,0,RX_BUF_SIZE+1);
            const int rxBytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE, 200/portTICK_PERIOD_MS);
            data[rxBytes]=NULL;
            if (rxBytes > 0) 
            {
                
                ESP_LOGI(TAG_TEST, "Read %d bytes: '%s'", rxBytes, (char *)receive);
            }

            xSemaphoreGiveRecursive(my_sema);
        }

    }
    free(data);
}

void app_main(void)
{
    my_sema=xSemaphoreCreateRecursiveMutex();

    xSemaphoreGiveRecursive(my_sema);

    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_2, RX_BUF_SIZE * 2, RX_BUF_SIZE * 2, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, 2, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, 2, NULL);
}