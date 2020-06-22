#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/spi_common_internal.h"
#include "driver/spi_master.h"

static const char *TAG = "SPI_TEST";

void todo(void *pv)
{    
    spi_device_interface_config_t dev_cf;
    dev_cf.clock_speed_hz = SPI_MASTER_FREQ_10M;
    dev_cf.mode = 0;
    dev_cf.spics_io_num = 5;
    dev_cf.queue_size = 1;
    dev_cf.flags=SPI_DEVICE_3WIRE;

    spi_device_handle_t spi_hand;

    spi_bus_add_device(SPI2_HOST, &dev_cf, &spi_hand);

    spi_transaction_t data;
    memset(&data,0,sizeof(data));
    uint8_t data_transmit[4]={1,2,3,4}, data_receive[4];

    // data.flags=SPI_TRANS_MODE_DIO;
    data.length=32;
    data.tx_buffer=data_transmit;
    data.user=(void *)1;


// SPI_TRANS_USE_TXDATA
    ESP_LOGI(TAG, "Initializing task todo");
    while (1)
    {
        
        ESP_LOGI(TAG, "task to do is doing");
        spi_device_acquire_bus(spi_hand,portMAX_DELAY);

        spi_device_polling_transmit(spi_hand,&data);

        spi_device_release_bus(spi_hand);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{

    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing bus SPI...");
    spi_bus_config_t buscfg = {
        .miso_io_num = 12,
        .mosi_io_num = 13,
        .sclk_io_num = 14,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
    };
    //Initialize the SPI bus
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, 2);
    ESP_ERROR_CHECK(ret);

    xTaskCreate(todo, "todo", 4096 * 3, NULL, 3, NULL);
}
