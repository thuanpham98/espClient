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
uint8_t data_read[20];

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

/* define bit in eventgroup, which determine event wifi connected/disconnected */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

/* make a event groupt to manage event in wifi station mode */
static EventGroupHandle_t s_wifi_event_group;

/* TAG to log into screen */
static const char *TAG_HTTP = "HTTP";
static const char *TAG_WIFI = "WIFI";

/* variable temp to count times reconnect wifi */
static int s_retry_num = 0;

/* variable temp to receive data from server */
char temp[ESP_MAX_HTTP_RECV_BUFFER];

/* Alloc function here to easy see */
char* Print_JSON(char* id,double data[10]);
void wifi_init_sta(void);
void postTask(void *pv);

/* handling to event wifi */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_WIFI, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG_WIFI, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* handling http event */
esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:

        break;
    case HTTP_EVENT_ON_CONNECTED:

        break;
    case HTTP_EVENT_HEADER_SENT:

        break;
    case HTTP_EVENT_ON_HEADER:

        break;
    case HTTP_EVENT_ON_DATA:

        if (!esp_http_client_is_chunked_response(evt->client))
        {
            for (int i = 0; i < evt->data_len; i++)
            {
                temp[i] = *(((char *)evt->data) + i);
            }
        }
        break;
    case HTTP_EVENT_ON_FINISH:

        break;
    case HTTP_EVENT_DISCONNECTED:

        break;
    }
    return ESP_OK;
}

/* init wifi function */
void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS},
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG_WIFI, "connected to ap SSID:%s password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG_WIFI, "Failed to connect to SSID:%s, password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    }
    else
    {
        ESP_LOGE(TAG_WIFI, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

/////------------make json to post------------------///
char* Print_JSON(char* id,double data[10])
{
    cJSON* sudo =cJSON_CreateObject();
    cJSON* form=cJSON_CreateObject();
    cJSON_AddItemToObject(sudo, "ID",cJSON_CreateString(id));
    cJSON_AddItemToObject(sudo, "form",form);

    cJSON_AddNumberToObject(form,"sensor_1",data[0]);
    cJSON_AddNumberToObject(form,"sensor_2",data[1]);
    cJSON_AddNumberToObject(form,"sensor_3",data[2]);
    cJSON_AddNumberToObject(form,"sensor_4",data[3]);

    cJSON_AddNumberToObject(form,"sensor_5",data[4]);
    cJSON_AddNumberToObject(form,"sensor_6",data[5]);
    cJSON_AddNumberToObject(form,"sensor_7",data[6]);

    cJSON_AddNumberToObject(form,"sensor_8",data[7]);
    cJSON_AddNumberToObject(form,"sensor_9",data[8]);
    cJSON_AddNumberToObject(form,"sensor_10",data[9]);
    char *a=cJSON_Print(sudo);
    cJSON_Delete(sudo);//if don't free, heap memory will be overload
    return a;
}
//-------Post method----------//
// void postTask (void *pv)
// {
//     ESP_LOGI(TAG_HTTP," Init http Post");
    
//     double data[10];
//     esp_http_client_config_t config = {
//         .url=URL_SERVER,
//         .event_handler = _http_event_handle,
//     };
//     esp_http_client_handle_t client = esp_http_client_init(&config);
//     esp_http_client_set_method(client, HTTP_METHOD_POST);
//     esp_http_client_set_header(client, "Content-Type", "application/json");

//     /* varialbe for i2c */
//     uint8_t checksum=0;
//     uint16_t rawHumidity=0;
//     uint16_t rawTemperature=0;
//     double Humidity=0.0;
//     double Temperature=0.0;

//     while(1)
//     {
//         char* post_data = (char *) malloc(512);
//         /* USER code begin here */

//         /* temperature 14 bit */
//         data_write[0]=0xF3;
//         i2c_master_write_slave(I2C_MASTER_NUM, data_write, 1);
//         i2c_master_read_slave(I2C_MASTER_NUM, data_read, 1);
//         i2c_master_read_slave(I2C_MASTER_NUM, data_read, 3);

//         rawTemperature=data_read[0] <<8;
//         rawTemperature|=data_read[1];
//         checksum=checkCRC8(rawTemperature);

//         if(checksum==data_read[2])
//         {
//             Temperature=(0.002681 * (double)rawTemperature - 46.85); 
//             data[0]=Temperature;
//         }
//         //------------------------------------------------------------//

//         /* humidity 12 bit */
//         data_write[0]=0xF5;
//         i2c_master_write_slave(I2C_MASTER_NUM, data_write, 1);
//         i2c_master_read_slave(I2C_MASTER_NUM, data_read, 1);
//         i2c_master_read_slave(I2C_MASTER_NUM, data_read, 3);

//         rawHumidity=data_read[0] <<8;
//         rawHumidity|=data_read[1];
//         checksum=checkCRC8(rawHumidity);
//         if(checksum==data_read[2])
//         {
//             rawHumidity &=0xFFFD;
//             Humidity = (0.001907 * (double)rawHumidity - 6);
//             data[1]= Humidity;
//         }

//         data_write[0]=0xFE;
//         i2c_master_write_slave(I2C_MASTER_NUM, &data_write[0], 1);
//         i2c_master_read_slave(I2C_MASTER_NUM, data_read, 1);

//         /* USER code end here */

//         post_data = Print_JSON(ESP_ID,data);
//         esp_http_client_set_post_field(client,post_data,strlen(post_data));

//         esp_err_t err = esp_http_client_perform(client);
//         if (err == ESP_OK) {
//             ESP_LOGI(TAG_HTTP, "HTTP GET Status = %d, content_length = %d",
//                     esp_http_client_get_status_code(client),
//                     esp_http_client_get_content_length(client));


//             data[2]++;data[3]++;data[4]++;data[5]++;data[6]++;data[7]++;data[8]++;data[9]++;
//             free(post_data);

//             ESP_LOGI(TAG_HTTP,"free heap size is :%d",esp_get_free_heap_size() );
//             ESP_LOGI(TAG_HTTP," post success");
//         } 
//         else 
//         {
//             ESP_LOGE(TAG_HTTP, "HTTP GET request failed: %s", esp_err_to_name(err));
//             esp_restart();
//             break;
//         }

//         vTaskDelay(100/portTICK_PERIOD_MS);
//     }
//     esp_http_client_close(client);
//     esp_http_client_cleanup(client);
//     esp_restart();
//     vTaskDelete(NULL);
// }

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
    

    // /* init i2C */
    ESP_ERROR_CHECK(i2c_master_init());

    // data_write[0]=0x07;
    // data_write[1]=0x10;
    // i2c_master_write_slave(I2C_MASTER_NUM, data_write, 2,0x68);

    // data_write[0]=0x00;
    // data_write[1]=dectobcd(0);
    // data_write[2]=dectobcd(46);
    // data_write[3]=dectobcd(15);
    // data_write[4]=dectobcd(3);
    // data_write[5]=dectobcd(28);
    // data_write[6]=dectobcd(5);
    // data_write[7]=dectobcd(20);
    // i2c_master_write_slave(I2C_MASTER_NUM, data_write, 8,0x68);

    while (1)
    {

    data_write[0]=0x00;
    i2c_master_write_slave(I2C_MASTER_NUM, data_write, 1,0x68);
    i2c_master_read_slave(I2C_MASTER_NUM, data_read, 7,0x68);

    data_read[0]=bcdtodec(data_read[0] & 0x7f);
    ESP_LOGI(TAG_I2C,"%d",data_read[0]);

    data_read[1]=bcdtodec(data_read[1]);
    ESP_LOGI(TAG_I2C,"%d",data_read[1]);

    data_read[2]=bcdtodec(data_read[2] & 0x3f);
    ESP_LOGI(TAG_I2C,"%d",data_read[2]);

    data_read[3]=bcdtodec(data_read[3]);
    ESP_LOGI(TAG_I2C,"%d",data_read[3]);

    data_read[4]=bcdtodec(data_read[4]);
    ESP_LOGI(TAG_I2C,"%d",data_read[4]);
 
    data_read[5]=bcdtodec(data_read[5]);
    ESP_LOGI(TAG_I2C,"%d",data_read[5]);

    data_read[6]=bcdtodec(data_read[6]);
    ESP_LOGI(TAG_I2C,"%d",data_read[6]+2000);
    ESP_LOGI(TAG_I2C,"------------------------------");

    vTaskDelay(1000/portTICK_PERIOD_MS);
    }

    /* start Freertos */ 
    //xTaskCreate(&postTask,"postTask",4096*4,NULL,3,NULL);
}