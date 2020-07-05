/** Coppy All Right: Phạm Minh Thuận (Vietnamese Person)
 * This is full code for send data to server, read I2c 2 mode in 2 channel, read digital, read analog 8channel, send notify to user  with http POST Method
 * Because only me build it, it is not the best perform
 * this is my thesis for graduate im summer 2020, and free open source
 * help me make it perfect on github https://github.com/thuanpham98/espClient.git
 */

/* main.h */
#include "main.h"

/* library for wifi and event of system */
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"

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
#include "driver/adc.h"
#include "driver/gpio.h"

/* library I make */
#include "sync_rtc.h"

/* notify protobuf */
#include "notify.pb-c.h"

/* DIFU_* library */
#include "DIFU_I2C.h"
#include "DIFU_NVS.h"

/* define pin input digital */
#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_NUM_4) | (1ULL << GPIO_NUM_5) | (1ULL << GPIO_NUM_12) | (1ULL << GPIO_NUM_14) |  \
                            (1ULL << GPIO_NUM_2) | (1ULL << GPIO_NUM_23) | (1ULL << GPIO_NUM_15) | (1ULL << GPIO_NUM_19) )
                            // (1ULL << GPIO_NUM_25) | (1ULL << GPIO_NUM_26))

/* define number sensor */
#define NUM_SEN 20

/* URL to storage Data */
#define URL_SERVER "https://iot-server-365.herokuapp.com/storage"

/* define for I2C */
#define I2C_MASTER_SCL_IO_STANDARD_MODE 22
#define I2C_MASTER_SDA_IO_STANDARD_MODE 21
#define I2C_MASTER_SCL_IO_FAST_MODE 26
#define I2C_MASTER_SDA_IO_FAST_MODE 25

#define DHT21_ADDR 0x40  /*!< slave address for DHT21 sensor */
#define DS1307_ADDR 0x68 /*!< slave address for RTC DS1307 */

#define HTU21D_CRC8_POLYNOMINAL 0x13100 /*!>crc8 polynomial for 16bit value, CRC8 -> x^8 + x^5 + x^4 + 1 */

/* Parameter of esp32 */
uint8_t wifiMode = 0;
esp_parameter_t my_esp = {0};

/* define bit in eventgroup, which determine event wifi */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const int WIFI_FAIL_BIT = BIT2;

/* make a event groupt to manage event in wifi station mode */
static EventGroupHandle_t s_wifi_event_group;

/* TAG to log into screen */
static const char *TAG_HTTP = "HTTP";
static const char *TAG_WIFI = "WIFI";
static const char *TAG_I2C = "I2C";
// static const char *TAG_NVS = "NVS";

/* variable temp to count times reconnect wifi */
static int s_retry_num = 0;

/* NVS handler */
nvs_handle_t my_handle;

/* variable temp to receive data from server */
char temp[512];
double data[NUM_SEN];
uint8_t num_sensor = (uint8_t)(sizeof(data) / sizeof(data[0]));
uint8_t type_sync_time;

/* buffer read/write for i2c */
uint8_t data_write[2];
uint8_t data_read[3];

uint64_t numTime;

/* Alloc function here to easy see */
static uint8_t checkCRC8(uint16_t data);
static char *Print_JSON(char *id,uint32_t device, double data[20], uint8_t length, char *datetime, uint64_t timestamp);
static void wifi_init_smart(void);
static void wifi_init_sta(void);
static void initial_HTU21(void);
static void readI2C_DS1307(uint8_t data_buffer[7]);
void readDigital(void *pv);
void readADC_HTU21(void *pv);
void postTask(void *pv);

/* handling to event wifi */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        if (wifiMode == 1)
        {
            esp_wifi_connect();
        }
        else
        {
            // xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
            ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
            smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        // esp_wifi_connect();
        // xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        if (s_retry_num < 5)
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
        // xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE)
    {
        ESP_LOGI(TAG_WIFI, "Scan done");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL)
    {
        ESP_LOGI(TAG_WIFI, "Found channel");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
    {
        ESP_LOGI(TAG_WIFI, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = {0};
        uint8_t password[65] = {0};

        uint8_t pass[65] = {0};
        char *temp_id = (char *)malloc(13 * sizeof(char));
        char *temp_dev = (char *)malloc(10 * sizeof(char));

        uint8_t j = 0, k = 0, i = 0, index_id = 0, index_dev = 0, index_pass = 0;
        for (uint8_t n = 0; n < 32; n++)
        {
            if (evt->password[n] == '.')
            {
                j++;
                if (j <= 2)
                {
                    continue;
                }
            }
            if (j == 0)
            {
                *(n + temp_id) = evt->password[n];
                index_id++;
            }
            if (j == 1)
            {
                *(k + temp_dev) = evt->password[n];
                k++;
                index_dev++;
            }
            if (j >= 2)
            {
                pass[i] = evt->password[n];
                i++;
                index_pass++;
            }
        }
        for (int x = 0; x < index_id; x++)
        {
            my_esp.ID[x] = *(x + temp_id);
        }
        char *subDev = (char *)malloc(index_dev * sizeof(char));
        for (int x = 0; x < index_dev; x++)
        {
            *(x + subDev) = *(x + temp_dev);
        }
        my_esp.device  = atoi(subDev);
        ESP_LOGI(TAG_WIFI, "device is : %d", my_esp.device );
        ESP_LOGI(TAG_WIFI, "ID is : %s", my_esp.ID);

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true)
        {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, pass, sizeof(pass));

        memcpy(my_esp.user_wifi, evt->ssid, sizeof(evt->ssid));
        memcpy(my_esp.pass_wifi, pass, sizeof(pass));
        ESP_LOGI(TAG_WIFI, "SSID:%s", ssid);
        ESP_LOGI(TAG_WIFI, "PASSWORD:%s", password);

        free(subDev);
        free(temp_dev);
        free(temp_id);

        /* first time when config SSID and PASSWORD, it should be earse NVS to config again parameter for esp32 */
        erase_all_nvs();
        esp_err_t err;
        err = 0 | write_nvs("storage", &my_handle, "USER", my_esp.user_wifi, NVS_TYPE_STR);
        err = err | write_nvs("storage", &my_handle, "PASS", my_esp.pass_wifi, NVS_TYPE_STR);
        err = err | write_nvs("storage", &my_handle, "ID", my_esp.ID, NVS_TYPE_STR);
        err = err | write_nvs("storage", &my_handle, "DEV", &my_esp.device, NVS_TYPE_U32);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG_WIFI, "error when write of Smart config, restart to start again");
            esp_restart();
        }

        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE)
    {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
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

/* Mode Connect wifi with smartconfig */
static void wifi_init_smart(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t uxBits;
    while (1)
    {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if (uxBits & CONNECTED_BIT)
        {
            ESP_LOGI(TAG_WIFI, "WiFi Connected to ap");
        }
        if (uxBits & ESPTOUCH_DONE_BIT)
        {
            ESP_LOGI(TAG_WIFI, "smartconfig over");
            esp_smartconfig_stop();
            ESP_LOGI(TAG_WIFI, "Eveything is ok");
            break;
        }
    }
}

/* Mode Connect wifi with Station normal */
static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config;
    bzero(&wifi_config, sizeof(wifi_config_t));
    memcpy(wifi_config.sta.ssid, my_esp.user_wifi, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, my_esp.pass_wifi, sizeof(wifi_config.sta.password));

    ESP_LOGE(TAG_WIFI, "%s", wifi_config.sta.ssid);
    ESP_LOGE(TAG_WIFI, "%s", wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & CONNECTED_BIT)
    {
        ESP_LOGI(TAG_WIFI, "connected to ap ");
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        esp_err_t err = nvs_erase_all(my_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG_WIFI, " error! (%04X)\n", err);
            esp_restart();
        }
        err = nvs_commit(my_handle);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG_WIFI, " error in commit! (%04X)\n", err);
            esp_restart();
        }
        ESP_LOGI(TAG_WIFI, "Done erase");
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG_WIFI, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

/* make json to post */
static char *Print_JSON(char *id,uint32_t device, double data_form[20], uint8_t length, char *datetime, uint64_t timestamp)
{
    cJSON *sudo = cJSON_CreateObject();
    cJSON *form = cJSON_CreateObject();
    cJSON_AddItemToObject(sudo, "ID", cJSON_CreateString(id));
    cJSON_AddItemToObject(sudo, "dev", cJSON_CreateNumber(device));
    cJSON_AddItemToObject(sudo, "timestamp", cJSON_CreateNumber(timestamp));
    cJSON_AddItemToObject(sudo, "datetime", cJSON_CreateString(datetime));
    cJSON_AddItemToObject(sudo, "form", form);

    // uint8_t temp_num = 20 * (device / 2) + 1;
    char *strTemp = (char *)malloc(30 * sizeof(char));
    char *strindex = (char *)malloc(3 * sizeof(char));

    for (int i = 0; i < length; i++)
    {
        strcpy(strTemp, "sensor_");
        itoa((i + 1), strindex, 10);
        strcat(strTemp, strindex);
        cJSON_AddNumberToObject(form, strTemp, data_form[i]);
    }

    free(strindex);
    free(strTemp);

    char *a = cJSON_Print(sudo);
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
        master_write_i2c(I2C_MASTER_NUM_FAST_MODE, data_write, 1, DHT21_ADDR);
        master_read_i2c(I2C_MASTER_NUM_FAST_MODE, data_read, 1, DHT21_ADDR);
        master_read_i2c(I2C_MASTER_NUM_FAST_MODE, data_read, 3, DHT21_ADDR);

        rawTemperature = data_read[0] << 8;
        rawTemperature |= data_read[1];
        checksum = checkCRC8(rawTemperature);

        if (checksum == data_read[2])
        {
            Temperature = (0.002681 * (double)rawTemperature - 46.85);
            data[0] = Temperature;
        }
        data_write[0] = 0xFE;
        master_write_i2c(I2C_MASTER_NUM_FAST_MODE, &data_write[0], 1, DHT21_ADDR);
        master_read_i2c(I2C_MASTER_NUM_FAST_MODE, data_read, 1, DHT21_ADDR);
        //------------------------------------------------------------//

        /* humidity 12 bit */
        data_write[0] = 0xF5;
        master_write_i2c(I2C_MASTER_NUM_FAST_MODE, data_write, 1, DHT21_ADDR);
        master_read_i2c(I2C_MASTER_NUM_FAST_MODE, data_read, 1, DHT21_ADDR);
        master_read_i2c(I2C_MASTER_NUM_FAST_MODE, data_read, 3, DHT21_ADDR);
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
        master_write_i2c(I2C_MASTER_NUM_FAST_MODE, &data_write[0], 1, DHT21_ADDR);
        master_read_i2c(I2C_MASTER_NUM_FAST_MODE, data_read, 1, DHT21_ADDR);

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
        // ESP_LOGE(TAG_HTTP,"%lf",data[2]);
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
        char *post_data = (char *)malloc(1024 * sizeof(char));
        char *str_time = (char *)malloc(64 * sizeof(char));
        get_time_str(str_time);
        /* USER code begin here */

//-------------------------------------------------------------------------------------------------------------------------------------------------

        /* USER code end here */
        post_data = Print_JSON(my_esp.ID,my_esp.device, data, num_sensor, str_time, get_timestamp());
        ESP_LOGI(TAG_I2C, "%s", post_data);
        esp_http_client_set_post_field(client, post_data, strlen(post_data));
        esp_err_t err = esp_http_client_perform(client);
        free(post_data);
        free(str_time);

        if (err == ESP_OK)
        {
            ESP_LOGI(TAG_HTTP, "HTTP POST Status = %d, content_length = %d",
                     esp_http_client_get_status_code(client),
                     esp_http_client_get_content_length(client));

            ESP_LOGI(TAG_HTTP, "free heap size is :%d", esp_get_free_heap_size());
            ESP_LOGI(TAG_HTTP, " post success");
            
        }
        else
        {
            ESP_LOGE(TAG_HTTP, "HTTP POST request failed: %s", esp_err_to_name(err));
            esp_restart();
            break;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
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

    /** initialize NVS flash */
    wifiMode = 1; /*!> default have data in NVS */
    esp_err_t err = nvs_flash_init();
    open_repository("storage", &my_handle);

    /* GET data from NVS */
    /* Get SSID of wifi */
    err = err | read_nvs("storage", &my_handle, "USER", my_esp.user_wifi, NVS_TYPE_STR);

    /* Get SSPASS of Wifi */
    err = err |  read_nvs("storage", &my_handle, "PASS", my_esp.pass_wifi, NVS_TYPE_STR);

    /* Get ID of Esp */
    err = err |  read_nvs("storage", &my_handle, "ID", my_esp.ID, NVS_TYPE_STR);

    /* Get dev numer of Esp */
    err = err |  read_nvs("storage", &my_handle, "DEV", &my_esp.device, NVS_TYPE_U32);

    if(err!=ESP_OK)
    {
        wifiMode =0 ;
    }
    else
    {
        wifiMode =1;
    }
    
    if (wifiMode==0)
    {
        wifi_init_smart();
    }
    else
    {
        wifi_init_sta();
    }

    /* init i2C */
    i2c_config_t conf_s;
    conf_s.mode = I2C_MODE_MASTER;
    conf_s.sda_io_num = I2C_MASTER_SDA_IO_STANDARD_MODE;
    conf_s.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_s.scl_io_num = I2C_MASTER_SCL_IO_STANDARD_MODE;
    conf_s.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf_s.master.clk_speed = I2C_MASTER_FREQ_HZ_STANDARD_MODE;

    i2c_config_t conf_f;
    conf_f.mode = I2C_MODE_MASTER;
    conf_f.sda_io_num = I2C_MASTER_SDA_IO_FAST_MODE;
    conf_f.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf_f.scl_io_num = I2C_MASTER_SCL_IO_FAST_MODE;
    conf_f.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf_f.master.clk_speed = I2C_MASTER_FREQ_HZ_FAST_MODE;

    ESP_ERROR_CHECK(init_i2c(&conf_s,I2C_MASTER_NUM_STANDARD_MODE));
    ESP_ERROR_CHECK(init_i2c(&conf_f,I2C_MASTER_NUM_FAST_MODE));

    initial_HTU21();/*!> init for HTU21 */

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

    /* config RTC */
    type_sync_time = 0;
    uint8_t data_time[7];
    set_time_zone();
    if (type_sync_time == 0)
    {
        sync_time(type_sync_time, data_time);
    }
    else
    {
        readI2C_DS1307(data_time);
        sync_time(type_sync_time, data_time);
    }

    /* start Freertos */
    xTaskCreate(&readI2C_DHT21, "readI2C_DHT21", 4096 * 2, NULL, 3, NULL);
    xTaskCreate(&readADC, "readADC", 4096 * 2, NULL, 3, NULL);
    xTaskCreate(&readDigital, "readDigital", 4096 * 2, NULL, 3, NULL);
    xTaskCreate(&postTask, "postTask", 4096 * 4, NULL, 4, NULL);
}

static void initial_HTU21(void)
{
    data_write[0] = 0xFE; /*!> reset*/
    master_write_i2c(I2C_MASTER_NUM_FAST_MODE, &data_write[0], 1, DHT21_ADDR);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    data_write[0] = 0xE7; /*!> resolution */
    data_write[1] = 0x02;
    master_write_i2c(I2C_MASTER_NUM_FAST_MODE, data_write, 2, DHT21_ADDR);

    master_read_i2c(I2C_MASTER_NUM_FAST_MODE, data_read, 1, DHT21_ADDR);
    ESP_LOGI(TAG_I2C, "%x", data_read[0]);
    vTaskDelay(50 / portTICK_PERIOD_MS);

    data_write[0] = 0xFE; /*!> reset*/
    master_write_i2c(I2C_MASTER_NUM_FAST_MODE, &data_write[0], 1, DHT21_ADDR);
    vTaskDelay(5 / portTICK_PERIOD_MS);
}

static void readI2C_DS1307(uint8_t data_buffer[7])
{
    data_buffer[0] = 0x00;
    master_write_i2c(I2C_MASTER_NUM_STANDARD_MODE, &data_buffer[0], 1,DS1307_ADDR);
    master_read_i2c(I2C_MASTER_NUM_STANDARD_MODE, data_buffer, 7, DS1307_ADDR);
}

/* checksum CRC from sensor HTU21 */
static uint8_t checkCRC8(uint16_t data)
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