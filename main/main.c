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

/* library for protobuf-C */
#include "message.pb-c.h"
#include "stringtoarray.h"

/* library for perihape */
#include "driver/gpio.h"
#include "driver/dac.h"
#include "driver/ledc.h"

/* no ideal for it */
#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_NUM_0)  | (1ULL << GPIO_NUM_2)  | (1ULL << GPIO_NUM_4)  | (1ULL << GPIO_NUM_5) |  \
                             (1ULL << GPIO_NUM_12) | (1ULL << GPIO_NUM_13) | (1ULL << GPIO_NUM_14) | (1ULL << GPIO_NUM_15)|  \
                              (1ULL << GPIO_NUM_18)| (1ULL << GPIO_NUM_19) | \
                             (1ULL << GPIO_NUM_21) | (1ULL << GPIO_NUM_22) | (1ULL << GPIO_NUM_23) | (1ULL << GPIO_NUM_27))
///////////////////////////////////////////////////////////////////////

/* define from file Konfig */
#define ESP_WIFI_SSID               CONFIG_WIFI_SSID
#define ESP_WIFI_PASS               CONFIG_WIFI_PASSWORD
#define ESP_MAXIMUM_RETRY           CONFIG_MAXIMUM_RETRY
#define URL_SERVER                  CONFIG_URL_SERVER
#define ESP_MAX_HTTP_RECV_BUFFER    CONFIG_MAX_HTTP_RECV_BUFFER
#define ESP_ID                      CONFIG_ID_DEVICE
#define ESP_NUM                     CONFIG_NUMBER_DEVICE

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
Sensor *protoc(char *message);
void wifi_init_sta(void);
void getTask(void *pv);

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
    char *a = (char* ) malloc(15*sizeof(char));
    char *b = (char* ) malloc(10*sizeof(char));
    
    strcpy(a,ESP_ID);
    itoa(ESP_NUM,b,10);
   
    strcat(a,b);
    esp_http_client_set_header(client, "ID",a );
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

                if ((strcmp(s->id, ESP_ID) == 0) && (s->device == ESP_NUM))
                {
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
                    // else if (io == 32)
                    // {
                    //     ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, val, 0);
                    // }
                    else
                    {
                        gpio_set_level(io, val);
                    }

                    /* USER code end here */
                }

                /* ensure data is not overloading */
                for (int i = 0; i < ESP_MAX_HTTP_RECV_BUFFER; i++)
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
            esp_restart();
            break;
        }

        sensor__free_unpacked(s, NULL);

        vTaskDelay(5 / portTICK_PERIOD_MS);
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

    /* config gpio output */
    gpio_pad_select_gpio(16);
    gpio_pad_select_gpio(17);
    gpio_set_direction(16,GPIO_MODE_OUTPUT);
    gpio_set_direction(17,GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(32);
    gpio_pad_select_gpio(33);
    gpio_set_direction(32,GPIO_MODE_OUTPUT);
    gpio_set_direction(33,GPIO_MODE_OUTPUT);/*!> io32 and io33 is special io, need to choose it  */

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
    ledc_timer_config_t t_config = {

        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = 2,
        .freq_hz = 10000,
        .clk_cfg = LEDC_USE_APB_CLK,
    };

    gpio_pad_select_gpio(34);
    ledc_channel_config_t tc_config = {
        .gpio_num = 34,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_2,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_2,
        .duty = 50,
        .hpoint = 0,
    };
    ledc_timer_config(&t_config);
    ledc_channel_config(&tc_config);
    ledc_fade_func_install(0); /*!> if we don't have this function, can't update duty */

    /* start Freertos */
    xTaskCreate(&getTask, "getTask", 4096 * 3, NULL, 2, NULL);
}