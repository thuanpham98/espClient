/** Coppy All Right: Phạm Minh Thuận (Vietnamese Person)
 * This is full code for send data to server, read I2c 2 mode in 2 channel, read digital, read analog 8channel  with http POST Method
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
#include "driver/adc.h"

/* library I make */
#include "components/include/"

/* define from file Konfig */
#define ESP_WIFI_SSID CONFIG_WIFI_SSID
#define ESP_WIFI_PASS CONFIG_WIFI_PASSWORD
#define ESP_MAXIMUM_RETRY CONFIG_MAXIMUM_RETRY
#define URL_SERVER CONFIG_URL_SERVER
#define ESP_MAX_HTTP_RECV_BUFFER CONFIG_MAX_HTTP_RECV_BUFFER
#define ESP_ID CONFIG_ID_DEVICE
#define ESP_NUM CONFIG_NUMBER_DEVICE

/* define for I2C */
#define I2C_MASTER_SCL_IO_STANDARD_MODE 22
#define I2C_MASTER_SDA_IO_STANDARD_MODE 21
#define I2C_MASTER_SCL_IO_FAST_MODE 26
#define I2C_MASTER_SDA_IO_FAST_MODE 25

#define I2C_MASTER_FREQ_HZ_STANDARD_MODE 100000
#define I2C_MASTER_FREQ_HZ_FAST_MODE 400000

#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define WRITE_BIT I2C_MASTER_WRITE
#define READ_BIT I2C_MASTER_READ
#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0
#define ACK_VAL 0x0
#define NACK_VAL 0x1

#define I2C_MASTER_NUM_STANDARD_MODE I2C_NUM_0
#define I2C_MASTER_NUM_FAST_MODE I2C_NUM_1

#define DHT21_ADDR 0x40                 /*!< slave address for DHT21 sensor */
#define DS1307_ADDR 0x68                /*!< slave address for RTC DS1307 */

#define HTU21D_CRC8_POLYNOMINAL 0x13100 /*!>crc8 polynomial for 16bit value, CRC8 -> x^8 + x^5 + x^4 + 1 */

/* define pin input digital */
#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_NUM_4) | (1ULL << GPIO_NUM_5) | (1ULL << GPIO_NUM_12) | (1ULL << GPIO_NUM_14) |  \
                            (1ULL << GPIO_NUM_2) | (1ULL << GPIO_NUM_23) | (1ULL << GPIO_NUM_15) | (1ULL << GPIO_NUM_19) | \
                            (1ULL << GPIO_NUM_25) | (1ULL << GPIO_NUM_26))

/* buffer read/write for i2c */
uint8_t data_write[2];
uint8_t data_read[3];
char    seccond[2],
        minute[2],
        hour[2],
        day[10],
        date[2],
        month[2],
        year[4],
  stringTime[30];
uint64_t  numTime;

/* define bit in eventgroup, which determine event wifi connected/disconnected */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

/* make a event groupt to manage event in wifi station mode */
static EventGroupHandle_t s_wifi_event_group;

/* TAG to log into screen */
static const char *TAG_HTTP = "HTTP";
static const char *TAG_WIFI = "WIFI";
static const char *TAG_I2C = "i2c";

/* variable temp to count times reconnect wifi */
static int s_retry_num = 0;

/* variable temp to receive data from server */
char temp[ESP_MAX_HTTP_RECV_BUFFER];
double data[20];

/* Alloc function here to easy see */
uint8_t checkCRC8(uint16_t data);
static esp_err_t i2c_master_standard_mode_init(void);
static esp_err_t i2c_master_fast_mode_init(void);
static esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t *data_rd, size_t size,uint8_t address_slave);
static esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t *data_wr, size_t size,uint8_t address_slave);
char *Print_JSON(char *id, double data[20],char *datetime,uint64_t timestamp);
void wifi_init_sta(void);
void readDigital(void *pv);
void readI2C_DS1307(void *pv);
void readADC_HTU21(void *pv);
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

/* using convert number in DS1307 */
int bcdtodec(uint8_t num)
{
    return ((num>>4)*10 + (num&0x0f));
}
int dectobcd(uint8_t num)
{
    return ((num/10)<<4 | (num%10));
}

/* make json to post */
char *Print_JSON(char *id, double data[20],char *datetime,uint64_t timestamp)
{
    cJSON *sudo = cJSON_CreateObject();
    cJSON *form = cJSON_CreateObject();
    cJSON_AddItemToObject(sudo, "ID", cJSON_CreateString(id));
    cJSON_AddItemToObject(sudo, "dev", cJSON_CreateNumber(ESP_NUM));
    cJSON_AddItemToObject(sudo, "timestamp", cJSON_CreateNumber(timestamp));
    cJSON_AddItemToObject(sudo, "form", form);
    cJSON_AddItemToObject(sudo, "datetime", cJSON_CreateString(datetime));

    uint8_t temp_num = 20 * (ESP_NUM / 2) + 1;
    char *strTemp = (char *)malloc(30 * sizeof(char));
    char *strindex = (char *)malloc(3 * sizeof(char));

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    ESP_LOGI(TAG_HTTP, "%s", strTemp);
    cJSON_AddNumberToObject(form, strTemp, data[0]);
    ESP_LOGI(TAG_HTTP, "%s", strTemp);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[1]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[2]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[3]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[4]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[5]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[6]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[7]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[8]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[9]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[10]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[11]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[12]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[13]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[14]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[15]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[16]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[17]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[18]);

    strcpy(strTemp, "sensor_");
    itoa(temp_num, strindex, 10);
    strcat(strTemp, strindex);
    temp_num++;
    cJSON_AddNumberToObject(form, strTemp, data[19]);
    ESP_LOGI(TAG_HTTP, "end");

    free(strindex);
    free(strTemp);

    char *a = cJSON_Print(sudo);
    ESP_LOGI(TAG_I2C,"%s",a);
    cJSON_Delete(sudo); //if don't free, heap memory will be overload

    return a;
}

/* read Digital */
void readDigital(void *pv)
{
    while (1)
    {
        data[10] = gpio_get_level(4);
        data[11] = gpio_get_level(5);
        data[12] = gpio_get_level(12);
        data[13] = gpio_get_level(14);
        data[14] = gpio_get_level(15);
        data[15] = gpio_get_level(19);
        data[16] = gpio_get_level(2);
        data[17] = gpio_get_level(23);
        //data[18] = gpio_get_level(25);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    esp_restart();
    vTaskDelete(NULL);
}
void readI2C_DS1307 (void *pv)
{
    uint8_t data_buffer[7];
    char *theday[8]={    "none",
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"};
    uint64_t subtimestamp[6]={0,0,0,0,0,0};
    while(1)
    {
        data_buffer[0]=0x00;
        i2c_master_write_slave(I2C_MASTER_NUM_STANDARD_MODE, &data_buffer[0], 1,0x68);
        i2c_master_read_slave(I2C_MASTER_NUM_STANDARD_MODE, data_buffer, 7,0x68);

        itoa(bcdtodec(data_buffer[2] & 0x3f), hour, 10);
        strcpy(stringTime, hour);
        strcat(stringTime, ":");

        itoa(bcdtodec(data_buffer[1] ), minute, 10);
        strcat(stringTime, minute);
        strcat(stringTime, ":");

        itoa(bcdtodec(data_buffer[0] & 0x7f), seccond, 10);
        strcat(stringTime, seccond);
        strcat(stringTime, "-");
        
        strcpy(day, theday[bcdtodec(data_buffer[3])]);
        strcat(stringTime, day);
        strcat(stringTime, "-");

        itoa(bcdtodec(data_buffer[4] ), date, 10);
        strcat(stringTime, date);
        strcat(stringTime, "/");

        itoa(bcdtodec(data_buffer[5] ), month, 10);
        strcat(stringTime, month);
        strcat(stringTime, "/");

        itoa(bcdtodec(data_buffer[6] )+2000, year, 10);
        strcat(stringTime,year);

        int current_year = (int)(bcdtodec(data_buffer[6]))+2000;
        ESP_LOGI(TAG_I2C,"%d",current_year);

        ESP_LOGI(TAG_I2C,"%lld",numTime);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    esp_restart();
    vTaskDelete(NULL);
}
/* read i2c */
void readI2C_DHT21(void *pv)
{
    /* varialbe for i2c */
    uint8_t checksum = 0;
    uint16_t rawHumidity = 0;
    uint16_t rawTemperature = 0;
    double Humidity = 0.0;
    double Temperature = 0.0;
    while (1)
    {
        /* temperature 14 bit */
        data_write[0] = 0xF3;
        i2c_master_write_slave(I2C_MASTER_NUM_FAST_MODE, data_write, 1,DHT21_ADDR);
        i2c_master_read_slave(I2C_MASTER_NUM_FAST_MODE, data_read, 1,DHT21_ADDR);
        i2c_master_read_slave(I2C_MASTER_NUM_FAST_MODE, data_read, 3,DHT21_ADDR);

        rawTemperature = data_read[0] << 8;
        rawTemperature |= data_read[1];
        checksum = checkCRC8(rawTemperature);

        if (checksum == data_read[2])
        {
            Temperature = (0.002681 * (double)rawTemperature - 46.85);
            data[0] = Temperature;
        }
        data_write[0] = 0xFE;
        i2c_master_write_slave(I2C_MASTER_NUM_FAST_MODE, &data_write[0], 1,DHT21_ADDR);
        i2c_master_read_slave(I2C_MASTER_NUM_FAST_MODE, data_read, 1,DHT21_ADDR);
        //------------------------------------------------------------//

        /* humidity 12 bit */
        data_write[0] = 0xF5;
        i2c_master_write_slave(I2C_MASTER_NUM_FAST_MODE, data_write, 1,DHT21_ADDR);
        i2c_master_read_slave(I2C_MASTER_NUM_FAST_MODE, data_read, 1,DHT21_ADDR);
        i2c_master_read_slave(I2C_MASTER_NUM_FAST_MODE, data_read, 3,DHT21_ADDR);
        rawHumidity = data_read[0] << 8;
        rawHumidity |= data_read[1];
        checksum = checkCRC8(rawHumidity);
        if (checksum == data_read[2])
        {
            rawHumidity &= 0xFFFD;
            Humidity = (0.001907 * (double)rawHumidity - 6);
            data[1] = Humidity;
        }

        data_write[0] = 0xFE;
        i2c_master_write_slave(I2C_MASTER_NUM_FAST_MODE, &data_write[0], 1,DHT21_ADDR);
        i2c_master_read_slave(I2C_MASTER_NUM_FAST_MODE, data_read, 1,DHT21_ADDR);

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    esp_restart();
    vTaskDelete(NULL);
}

/* read ADC */
void readADC(void *pv)
{
    //-------------------------------------------------------//
    while (1)
    {
        /* read data from analog */
        data[2] = (double)adc1_get_raw(ADC1_CHANNEL_0) / 4096 * 100;
        data[3] = (double)adc1_get_raw(ADC1_CHANNEL_1) / 4096 * 100;
        data[4] = (double)adc1_get_raw(ADC1_CHANNEL_2) / 4096 * 100;
        data[5] = (double)adc1_get_raw(ADC1_CHANNEL_3) / 4096 * 100;
        data[6] = (double)adc1_get_raw(ADC1_CHANNEL_4) / 4096 * 100;
        data[7] = (double)adc1_get_raw(ADC1_CHANNEL_5) / 4096 * 100;
        data[8] = (double)adc1_get_raw(ADC1_CHANNEL_6) / 4096 * 100;
        data[9] = (double)adc1_get_raw(ADC1_CHANNEL_7) / 4096 * 100;

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    esp_restart();
    vTaskDelete(NULL);
}

/* Post method */
void postTask(void *pv)
{
    ESP_LOGI(TAG_HTTP, " Init http Post");

    esp_http_client_config_t config = {
        .url = URL_SERVER,
        .event_handler = _http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    while (1)
    {
        char *post_data = (char *)malloc(1024);

        /* USER code begin here */

        /* USER code end here */
        post_data = Print_JSON(ESP_ID, data,stringTime,numTime);
        esp_http_client_set_post_field(client, post_data, strlen(post_data));
        esp_err_t err = esp_http_client_perform(client);
        free(post_data);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG_HTTP, "HTTP GET Status = %d, content_length = %d",
                     esp_http_client_get_status_code(client),
                     esp_http_client_get_content_length(client));

            ESP_LOGI(TAG_HTTP, "free heap size is :%d", esp_get_free_heap_size());
            ESP_LOGI(TAG_HTTP, " post success");
        }
        else
        {
            ESP_LOGE(TAG_HTTP, "HTTP GET request failed: %s", esp_err_to_name(err));
            esp_restart();
            break;
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    esp_restart();
    vTaskDelete(NULL);
}

/* main */
void app_main(void)
{
    /* disable the default wifi logging */
    esp_log_level_set("wifi", ESP_LOG_NONE);

    /* Initialize NVS, which store wifi ssid and password */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* call function init wifi */
    wifi_init_sta();

    /* init i2C */
    ESP_ERROR_CHECK(i2c_master_standard_mode_init());
    ESP_ERROR_CHECK(i2c_master_fast_mode_init());

    data_write[0] = 0xFE; /*!> reset*/
    i2c_master_write_slave(I2C_MASTER_NUM_FAST_MODE, &data_write[0], 1,DHT21_ADDR);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    data_write[0] = 0xE7; /*!> resolution */
    data_write[1] = 0x02;
    i2c_master_write_slave(I2C_MASTER_NUM_FAST_MODE, data_write, 2,DHT21_ADDR);

    i2c_master_read_slave(I2C_MASTER_NUM_FAST_MODE, data_read, 1,DHT21_ADDR);
    ESP_LOGI(TAG_I2C, "%x", data_read[0]);
    vTaskDelay(50 / portTICK_PERIOD_MS);

    data_write[0] = 0xFE; /*!> reset*/
    i2c_master_write_slave(I2C_MASTER_NUM_FAST_MODE, &data_write[0], 1,DHT21_ADDR);
    vTaskDelay(5 / portTICK_PERIOD_MS);

    /* config parameter for ADC */
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_2, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);

    /* config digital input */
    gpio_config_t input_conf;
    input_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    input_conf.mode = GPIO_MODE_INPUT;
    input_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    input_conf.pull_down_en = 0;
    input_conf.pull_up_en = 0;
    gpio_config(&input_conf);

    /* start Freertos */
    xTaskCreate(&readI2C_DS1307, "readI2C_DS1307", 4096 * 2, NULL, 4, NULL);
    xTaskCreate(&readI2C_DHT21, "readI2C_DHT21", 4096 * 2, NULL, 3, NULL);
    xTaskCreate(&readADC, "readADC", 4096 * 2, NULL, 3, NULL);
    xTaskCreate(&readDigital, "readDigital", 4096 * 2, NULL, 3, NULL);
    xTaskCreate(&postTask, "postTask", 4096 * 4, NULL, 4, NULL);
}

/* checksum CRC from sensor HTU21 */
uint8_t checkCRC8(uint16_t data)
{
    for (uint8_t bit = 0; bit < 16; bit++)
    {
        if (data & 0x8000)
            data = (data << 1) ^ HTU21D_CRC8_POLYNOMINAL;
        else
            data <<= 1;
    }
    return data >>= 8;
}

/* I2C master Initial */
static esp_err_t i2c_master_standard_mode_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM_STANDARD_MODE;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO_STANDARD_MODE;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO_STANDARD_MODE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ_STANDARD_MODE;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}
static esp_err_t i2c_master_fast_mode_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM_FAST_MODE;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO_FAST_MODE;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO_FAST_MODE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ_FAST_MODE;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/* I2C master read data from slave */
static esp_err_t i2c_master_read_slave(i2c_port_t i2c_num, uint8_t *data_rd, size_t size, uint8_t address_slave)
{
    if (size == 0)
    {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address_slave << 1) | READ_BIT, ACK_CHECK_EN);
    if (size > 1)
    {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/* I2C master write data to slave */
static esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t *data_wr, size_t size, uint8_t address_slave)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address_slave << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}