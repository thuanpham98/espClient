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

/* library for connect TCP/IP */
#include "lwip/err.h"
#include "lwip/sys.h"

/* library for http connect */
#include "esp_http_client.h"
#include "esp_transport.h"
#include "esp_transport_tcp.h"

/* library for Json */
#include <limits.h>
#include <ctype.h>
#include <cJSON.h>
#include <cJSON_Utils.h>
#include "unity.h"

/* library for perihape */
#include "driver/i2c.h"

/* define from file Konfig */
#define ESP_WIFI_SSID               CONFIG_WIFI_SSID
#define ESP_WIFI_PASS               CONFIG_WIFI_PASSWORD
#define ESP_MAXIMUM_RETRY           CONFIG_MAXIMUM_RETRY
#define URL_SERVER                  CONFIG_URL_SERVER
#define ESP_MAX_HTTP_RECV_BUFFER    CONFIG_MAX_HTTP_RECV_BUFFER
#define ESP_ID                      CONFIG_ID_DEVICE
#define ESP_NUM                     CONFIG_NUMBER_DEVICE

/* define for I2C */
static const char *TAG_I2C = "i2c";

#define I2C_MASTER_SCL_IO 22               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 21               /*!< gpio number for I2C master data  */
#define I2C_MASTER_FREQ_HZ 100000        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */

//#define ESP_SLAVE_ADDR 0x42   /*!< slave address for DHT21 sensor */

#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

#define I2C_MASTER_NUM I2C_NUM_0
#define HTU21D_CRC8_POLYNOMINAL      0x13100   //crc8 polynomial for 16bit value, CRC8 -> x^8 + x^5 + x^4 + 1

uint8_t data_write[20];
uint8_t data_read[10];

/**
 * @brief test code to read esp-i2c-slave
 *        We need to fill the buffer of esp slave device, then master can read them out.
 *
 * _______________________________________________________________________________________
 * | start | slave_addr + rd_bit +ack | read n-1 bytes + ack | read 1 byte + nack | stop |
 * --------|--------------------------|----------------------|--------------------|------|
 *
 */
static esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t *data_rd, size_t size,uint8_t ESP_SLAVE_ADDR)
{
    if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
/**
 * @brief Test code to write esp-i2c-slave
 *        Master device write data to slave(both esp32),
 *        the data will be stored in slave buffer.
 *        We can read them out from slave buffer.
 *
 * ___________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write n bytes + ack  | stop |
 * --------|---------------------------|----------------------|------|
 *
 */
static esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t *data_wr, size_t size,uint8_t ESP_SLAVE_ADDR)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}
uint8_t checkCRC8(uint16_t data)
{
    for (uint8_t bit = 0; bit < 16; bit++)
    {
        if   (data & 0x8000) data = (data << 1) ^ HTU21D_CRC8_POLYNOMINAL;
        else data <<= 1;
    }
    return data >>= 8;
}


int bcdtodec(uint8_t num)
{
    return ((num>>4)*10 + (num&0x0f));
}
int dectobcd(uint8_t num)
{
    return ((num/10)<<4 | (num%10));
}

/* main */
void app_main(void)
{
    
    uint16_t mylux=0;
    float lux=0.0;
    // /* init i2C */
    ESP_ERROR_CHECK(i2c_master_init());

    data_write[0]=0x01;
    i2c_master_write_slave(I2C_MASTER_NUM, data_write, 1,0x23);

    data_write[0]=0x10;
    i2c_master_write_slave(I2C_MASTER_NUM, data_write,1,0x23);

    vTaskDelay(150/portTICK_PERIOD_MS);
    while (1)
    {
    
    i2c_master_read_slave(I2C_MASTER_NUM, data_read,2,0x23);

    mylux=0;
    mylux=((mylux | data_read[0])<<8) |data_read[1];

    lux=(float)mylux/1.2;
    

    ESP_LOGI(TAG_I2C,"%d",mylux);

    ESP_LOGI(TAG_I2C,"ok r nha");
    vTaskDelay(200/portTICK_PERIOD_MS);
    }

}