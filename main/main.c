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

/* library for protobuf-C */
#include "message.pb-c.h"
#include "stringtoarray.h"
#include "notify.pb-c.h"

/* Library NVS memory  */
#include "esp_partition.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

/* library for perihape */
#include "driver/gpio.h"
#include "driver/dac.h"
#include "driver/ledc.h"

/* define gpio digital output*/
#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_NUM_0) | (1ULL << GPIO_NUM_2) | (1ULL << GPIO_NUM_4) | (1ULL << GPIO_NUM_5) |     \
                             (1ULL << GPIO_NUM_12) | (1ULL << GPIO_NUM_13) | (1ULL << GPIO_NUM_14) | (1ULL << GPIO_NUM_15) | \
                             (1ULL << GPIO_NUM_18) | (1ULL << GPIO_NUM_19) |                                                 \
                             (1ULL << GPIO_NUM_21) | (1ULL << GPIO_NUM_22) | (1ULL << GPIO_NUM_23) | (1ULL << GPIO_NUM_27))

/* define timer for ledc PWM */
#define LEDC_HS_TIMER LEDC_TIMER_0
#define LEDC_HS_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO (32)
#define LEDC_HS_CH0_CHANNEL LEDC_CHANNEL_0
#define LEDC_HS_CH1_GPIO (33)
#define LEDC_HS_CH1_CHANNEL LEDC_CHANNEL_1

/* URL to storage Data */
#define URL_SERVER "https://iot-server-365.herokuapp.com/storage"

/* Parameter of esp32 */
char ID[13];
uint32_t device;
uint8_t wifiMode = 0;
uint8_t user_wifi[33];
uint8_t pass_wifi[65];

/* define bit in eventgroup, which determine event wifi */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const int WIFI_FAIL_BIT = BIT2;

/* make a event groupt to manage event in wifi station mode */
static EventGroupHandle_t s_wifi_event_group;

/* TAG to log into screen */
static const char *TAG_HTTP = "HTTP";
static const char *TAG_WIFI = "WIFI";
static const char *TAG_NVS = "NVS";
// static const char *TAG = "SMART";

/* variable temp to count times reconnect wifi */
static int s_retry_num = 0;

/* NVS handler */
nvs_handle_t my_handle;

/* variable temp to receive data from server */
char temp[512];

/* Alloc function here to be easily saw */
static void wifi_init_smart(void);
static void wifi_init_sta(void);
static void erase_all_nvs(void);
static void write_nvs(void);
Sensor *protoc(char *message);
void getTask(void *pv);
// static void smartconfig_example_task(void *parm);

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
                continue;
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
            if (j == 2)
            {
                pass[i] = evt->password[n];
                i++;
                index_pass++;
            }
        }
        for (int x = 0; x < index_id; x++)
        {
            ID[x] = *(x + temp_id);
        }
        char *subDev = (char *)malloc(index_dev * sizeof(char));
        for (int x = 0; x < index_dev; x++)
        {
            *(x + subDev) = *(x + temp_dev);
        }
        device = atoi(subDev);
        ESP_LOGI(TAG_WIFI, "device is : %d", device);
        ESP_LOGI(TAG_WIFI, "ID is : %s", ID);

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

        memcpy(user_wifi, evt->ssid, sizeof(evt->ssid));
        memcpy(pass_wifi, pass, sizeof(pass));
        ESP_LOGI(TAG_WIFI, "SSID:%s", ssid);
        ESP_LOGI(TAG_WIFI, "PASSWORD:%s", password);

        free(subDev);
        free(temp_dev);
        free(temp_id);

        erase_all_nvs();
        write_nvs();

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
    // ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
    // smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
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
            // vTaskDelete(NULL);
            // ESP_LOGI(TAG, "OK r nha");
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
    memcpy(wifi_config.sta.ssid, user_wifi, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, pass_wifi, sizeof(wifi_config.sta.password));

    // wifi_config_t wifi_config = {
    //     .sta = {
    //         .ssid = "lau101",
    //         .password = "9999999",
    //     },
    // };
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

/* Erase NVS memory */
static void erase_all_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {

        ESP_LOGE(TAG_NVS, "Got NO_FREE_PAGES error, trying to erase the partition...\n");

        // find the NVS partition
        const esp_partition_t *nvs_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
        if (!nvs_partition)
        {

            ESP_LOGE(TAG_NVS, "FATAL ERROR: No NVS partition found\n");
            while (1)
                vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        // erase the partition
        err = (esp_partition_erase_range(nvs_partition, 0, nvs_partition->size));
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG_NVS, "FATAL ERROR: Unable to erase the partition\n");
            while (1)
                vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG_NVS, "Partition erased!\n");

        // now try to initialize it again
        err = nvs_flash_init();
        if (err != ESP_OK)
        {

            ESP_LOGE(TAG_NVS, "FATAL ERROR: Unable to initialize NVS\n");
            while (1)
                vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
    ESP_LOGI(TAG_NVS, "NVS init OK!\n");
}

/* Write parameter of esp to NVS */
static void write_nvs(void)
{
    /* open the partition in RW mode */
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {

        ESP_LOGE(TAG_NVS, "FATAL ERROR: Unable to open NVS\n");
        while (1)
            vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG_NVS, "NVS open OK\n");

    err = nvs_set_str(my_handle, "USER", (char *)user_wifi);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error in nvs_set_str! (%04X)", err);
        return;
    }

    err = nvs_set_str(my_handle, "PASS", (char *)pass_wifi);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error in nvs_set_str! (%04X)", err);
        return;
    }

    err = nvs_set_str(my_handle, "ID", (char *)ID);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error in nvs_set_str! (%04X)", err);
        return;
    }

    err = nvs_set_u32(my_handle, "DEV", device);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error in nvs_set_str! (%04X)", err);
        return;
    }

    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error in commit! (%04X)", err);
        return;
    }
}

/* get method */
void getTask(void *pv)
{
    ESP_LOGI(TAG_HTTP, " Init http get");
    char *get_data = (char *)malloc(512);

    esp_http_client_config_t config = {
        .url = URL_SERVER,
        .event_handler = _http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    char *a = (char *)malloc(15 * sizeof(char));
    char *b = (char *)malloc(10 * sizeof(char));

    strcpy(a, ID);
    itoa(device, b, 10);
    strcat(a, b);

    esp_http_client_set_header(client, "ID", a);

    ESP_LOGI(TAG_HTTP, "%s", a);
    ESP_LOGI(TAG_HTTP, " started");
    while (1)
    {
        Sensor *s = (Sensor *)malloc(sizeof(Sensor));
        sensor__init(s);

        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG_HTTP, "HTTP GET Status = %d, content_length = %d",
                     esp_http_client_get_status_code(client),
                     esp_http_client_get_content_length(client));

            if (esp_http_client_get_content_length(client))
            {
                get_data = temp;
                ESP_LOGI(TAG_HTTP, "%s", get_data);
                s = protoc(get_data);

                if ((strcmp(s->id, ID) == 0) && (s->device == device))
                {
                    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    /* USER code begin here */
                    ESP_LOGI(TAG_HTTP, "%s", temp);
                    ESP_LOGI(TAG_HTTP, "%s", s->id);

                    uint32_t val = (uint32_t)s->value;
                    uint32_t io = (uint32_t)s->io;
                    ESP_LOGI(TAG_HTTP, "%d", val);
                    ESP_LOGI(TAG_HTTP, "%d", io);

                    if (io == 25 || io == 26)
                    {
                        dac_output_voltage(io - 25, val);
                    }
                    else if (io == LEDC_HS_CH0_GPIO || io == LEDC_HS_CH1_GPIO)
                    {
                        switch (io)
                        {
                        case LEDC_HS_CH0_GPIO:
                            ledc_set_duty_and_update(LEDC_HS_MODE, LEDC_HS_CH0_CHANNEL, val, 0);
                            break;

                        case LEDC_HS_CH1_GPIO:
                            ledc_set_duty_and_update(LEDC_HS_MODE, LEDC_HS_CH1_CHANNEL, val, 0);
                            break;
                        }
                    }
                    else
                    {
                        gpio_set_level(io, val);
                    }

                    /* USER code end here */
                    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                }

                /* ensure data is not overloading */
                for (int i = 0; i < 512; i++)
                {
                    temp[i] = 0;
                }
            }
            else
                get_data = " ";
        }
        else
        {
            ESP_LOGE(TAG_HTTP, "HTTP GET request failed: %s", esp_err_to_name(err));
            // esp_restart();
            break;
        }

        sensor__free_unpacked(s, NULL);

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    esp_restart();
    vTaskDelete(NULL);
}

/* protoc-c */
Sensor *protoc(char *message)
{
    uint8_t buffer[1024];
    uint32_t k = Arr(message, buffer);

    return sensor__unpack(NULL, k, buffer);
}

/* main */
void app_main(void)
{
    /** initialize NVS flash */
    esp_err_t err = nvs_flash_init();
    wifiMode = 1; /*!> default have data in NVS */

    /** Open NVS and Check data  */
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "FATAL ERROR: can not open NVS");
        while (1)
            vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG_NVS, "NVS open OK");

    /* GET data from NVS */
    size_t string_size;

    /* Get SSID of wifi */
    err = nvs_get_str(my_handle, "USER", NULL, &string_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error in nvs_get_str to get string size! (%04X)", err);
        wifiMode = 0;
        ESP_LOGE(TAG_NVS, "Error in  get user");
    }
    char *value = malloc(string_size);
    err = nvs_get_str(my_handle, "USER", value, &string_size);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        ESP_LOGE(TAG_NVS, "Error in nvs_get_str to get string! (%04X)", err);
        wifiMode = 0;
        ESP_LOGE(TAG_NVS, "Error in  get pass");
    }
    else
    {
        memcpy(user_wifi, value, string_size);
        ESP_LOGE(TAG_NVS, "%s", user_wifi);
    }

    /* Get SSPASS of Wifi */
    err = nvs_get_str(my_handle, "PASS", NULL, &string_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error in nvs_get_str to get string size! (%04X)", err);
        wifiMode = 0;
    }
    value = malloc(string_size);
    err = nvs_get_str(my_handle, "PASS", value, &string_size);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        ESP_LOGE(TAG_NVS, "Error in nvs_get_str to get string! (%04X)", err);
        wifiMode = 0;
    }
    else
    {
        memcpy(pass_wifi, value, string_size);
        ESP_LOGE(TAG_NVS, "%s", pass_wifi);
    }

    /* Get ID of Esp */
    err = nvs_get_str(my_handle, "ID", NULL, &string_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error in nvs_get_str to get string size! (%04X)", err);
        wifiMode = 0;
    }
    value = malloc(string_size);
    err = nvs_get_str(my_handle, "ID", value, &string_size);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        ESP_LOGE(TAG_NVS, "Error in nvs_get_str to get string! (%04X)", err);
        wifiMode = 0;
    }
    else
    {
        memcpy(ID, value, string_size);
        ESP_LOGE(TAG_NVS, "%s", ID);
    }
    free(value);

    /* Get index of Esp */
    uint32_t value_dev;
    err = nvs_get_u32(my_handle, "DEV", &value_dev);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        ESP_LOGE(TAG_NVS, "Error in nvs_get_str to get string! (%04X)", err);
        wifiMode = 0;
    }
    else
    {
        device = value_dev;
        ESP_LOGE(TAG_NVS, "%d", device);
    }
    // wifiMode =0;
    /* choose mode connect wifi */
    if (wifiMode == 1)
    {
        wifi_init_sta();
    }
    else
    {
        wifi_init_smart();
    }

    /** Config for Perihap  */

    /* config gpio output */
    gpio_pad_select_gpio(16);
    gpio_pad_select_gpio(17);

    gpio_set_direction(16, GPIO_MODE_OUTPUT);
    gpio_set_direction(17, GPIO_MODE_OUTPUT); /*!> io32 and io33 is special io, need to choose it  */

    gpio_config_t output_conf;
    output_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    output_conf.mode = GPIO_MODE_OUTPUT;
    output_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    output_conf.pull_down_en = 0;
    output_conf.pull_up_en = 0;
    gpio_config(&output_conf);

    /* config DAC */
    dac_output_enable(DAC_CHANNEL_1); /*!> if io 25 */
    dac_output_enable(DAC_CHANNEL_2); /*!> io 26 */

    /* config pwm */
    ledc_timer_config_t timer_0 = {

        .speed_mode = LEDC_HS_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_HS_TIMER,
        .freq_hz = 32000,
        .clk_cfg = LEDC_USE_APB_CLK,
    };
    ledc_timer_config(&timer_0);

    gpio_pad_select_gpio(32);
    gpio_pad_select_gpio(33);

    ledc_channel_config_t ledc_hs_0 = {
        .gpio_num = LEDC_HS_CH0_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .channel = LEDC_HS_CH0_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_HS_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ledc_hs_0);

    ledc_channel_config_t ledc_hs_1 = {
        .gpio_num = LEDC_HS_CH1_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .channel = LEDC_HS_CH1_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_HS_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ledc_hs_1);

    ledc_fade_func_install(0); /*!> if we don't have this function, can't update duty */

    /* start Freertos */
    xTaskCreate(&getTask, "getTask", 4096 * 4, NULL, 2, NULL);
}

// static void smartconfig_example_task(void *parm)
// {
//     EventBits_t uxBits;
//     ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
//     smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
//     while (1)
//     {
//         uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
//         if (uxBits & CONNECTED_BIT)
//         {
//             ESP_LOGI(TAG, "WiFi Connected to ap");
//         }
//         if (uxBits & ESPTOUCH_DONE_BIT)
//         {
//             ESP_LOGI(TAG, "smartconfig over");
//             esp_smartconfig_stop();
//             ESP_LOGI(TAG, "Eveything is ok");
//             vTaskDelete(NULL);
//             ESP_LOGI(TAG, "OK r nha");
//         }
//     }
// }