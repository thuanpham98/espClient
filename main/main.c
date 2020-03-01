#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "nvs_flash.h"

#include "esp_http_client.h"
#include "esp_transport.h"
#include "esp_transport_tcp.h"

#include <limits.h>
#include <ctype.h>
#include <cJSON.h>
#include <cJSON_Utils.h>

#include "unity.h"

//----- define config from Kconfig-----//
#define S_WIFI_SSID      CONFIG_WIFI_SSID
#define S_WIFI_PASS      CONFIG_WIFI_PASSWORD
#define ID_DEVICE        CONFIG_ID_DEVICE

#define HOST             CONFIG_WEBSITE
#define URL              CONFIG_RESOURCE

#define MAX_HTTP_RECV_BUFFER 512
//-------------------------------------//

#define WIFI_CONNECTED_BIT BIT0

static const char *TAG_WIFI = "WIFI_STATION";
static const char *TAG_HTTP = "THUANNEK";
static const char *TAG_JSON = "JSON";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
char temp[512];
//---queue handle--//
xQueueHandle demo;
//// make a event for wifi ////
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id)
    {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;

	    case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            break;

	    case SYSTEM_EVENT_STA_DISCONNECTED:
		    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            break;

	    default:
            break;
    }
	return ESP_OK;
}
//--- make even for http---//
esp_err_t _http_event_handle(esp_http_client_event_t *evt)
    {
        switch(evt->event_id)
        {
            case HTTP_EVENT_ERROR:
                ESP_LOGI(TAG_HTTP, "HTTP_EVENT_ERROR");
                break;
            case HTTP_EVENT_ON_CONNECTED:
                ESP_LOGI(TAG_HTTP, "HTTP_EVENT_ON_CONNECTED");
                break;
            case HTTP_EVENT_HEADER_SENT:
                ESP_LOGI(TAG_HTTP, "HTTP_EVENT_HEADER_SENT");
                break;
            case HTTP_EVENT_ON_HEADER:
                ESP_LOGI(TAG_HTTP, "HTTP_EVENT_ON_HEADER");
                printf("%.*s", evt->data_len, (char*)evt->data);
                break;
            case HTTP_EVENT_ON_DATA:
                ESP_LOGI(TAG_HTTP, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
                if (!esp_http_client_is_chunked_response(evt->client)) {
                    //printf("%.*s", evt->data_len, (char*)evt->data);
                    for (int i = 0; i < evt->data_len; i++)
                    {
                        temp[i]=*(((char*)evt->data) +i);
                    }

                }

                break;
            case HTTP_EVENT_ON_FINISH:
                ESP_LOGI(TAG_HTTP, "HTTP_EVENT_ON_FINISH");
                break;
            case HTTP_EVENT_DISCONNECTED:
                ESP_LOGI(TAG_HTTP, "HTTP_EVENT_DISCONNECTED");
                break;
        }
        return ESP_OK;
    }

////connect to wifi////
void wifi_init_sta(void)
{
        //////// config wifi and start connect//////
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        wifi_config_t wifi_config =
        {
            .sta =
            {
                .ssid = S_WIFI_SSID,
                .password = S_WIFI_PASS
            },
        };
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
        ESP_ERROR_CHECK(esp_wifi_start() );

        ESP_LOGI(TAG_WIFI, "wifi_init_sta finished.");
        /////////////////////

        ////// wait flag connected///
        xEventGroupWaitBits(s_wifi_event_group,
                WIFI_CONNECTED_BIT,
                pdFALSE,
                pdTRUE,
                portMAX_DELAY);
    ESP_LOGI(TAG_WIFI, "connected to ap SSID:%s password:%s", S_WIFI_SSID, S_WIFI_PASS);

    // print the local IP address
	tcpip_adapter_ip_info_t ip_info;
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
	printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
	printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
	printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
	printf("\n");

    vEventGroupDelete(s_wifi_event_group);
}
/////------------make json to post------------------///
// void makeJson(void *pv)
// {
//     double a=0,b=0,c=0,d=0;
//     double data[4]={a,b,c,d};

//     while (1)
//     {


//         //a++;b++;c++;d++;
//         //cJSON_Delete(root);
//     if(xQueueSendToBack(demo,data,5000/portTICK_RATE_MS)!=pdTRUE)
//     {
//         ESP_LOGE(TAG_JSON,"error\r\n");
//     }
//         a++;b++;c++;d++;
//         vTaskDelay(1000/portTICK_PERIOD_MS);
//     }
//     vTaskDelete(NULL);
// }
//--------post method----------//
void postTask(void *pv)
{
    const char *post_data = (char *) malloc(512);
    //char *mess="hello server";
    //post_data=mess;

    double data[4]={0,0,0,0};

    cJSON *root, *form;
    //char* render;
    root=cJSON_CreateObject();
    cJSON_AddItemToObject(root, "ID",cJSON_CreateString(ID_DEVICE));
    cJSON_AddItemToObject(root, "form",form=cJSON_CreateObject());
    cJSON_AddNumberToObject(form,"sensor_1",data[0]);
    cJSON_AddNumberToObject(form,"sensor_2",data[1]);
    cJSON_AddNumberToObject(form,"sensor_3",data[2]);
    cJSON_AddNumberToObject(form,"sensor_4",data[3]);
    post_data=cJSON_Print(root);

    esp_http_client_config_t config = {
        .url = "http://192.168.168.101:6969/creator",
        .event_handler = _http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");



    while(1)
    {
        esp_http_client_set_post_field(client, post_data, strlen(post_data));
        //esp_http_client_write(client,post_data,strlen(post_data));
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG_HTTP, "HTTP POST Status = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));

        }
        else
        {
        ESP_LOGE(TAG_HTTP, "HTTP POST request failed: %s", esp_err_to_name(err));
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);

}
//----get method----//
void getTask(void *pv)
{

    char* get_data = (char *) malloc(512);

    esp_http_client_config_t config = {
        .url = "http://192.168.168.101:6969/creator",
        .event_handler = _http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_GET);

    while(1)
    {
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            ESP_LOGI(TAG_HTTP, "HTTP GET Status = %d, content_length = %d",
                    esp_http_client_get_status_code(client),
                    esp_http_client_get_content_length(client));
        } else {
            ESP_LOGE(TAG_HTTP, "HTTP GET request failed: %s", esp_err_to_name(err));
            //esp_restart();
        }
        get_data=temp;
        ESP_LOGI(TAG_HTTP,"%s",get_data);

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
}

void app_main(void)
{
    //----------------disable the default wifi logging----//
	esp_log_level_set("wifi", ESP_LOG_NONE);

    //////Initialize NVS , where store ssid and passwork of wifi////
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    /////////////////

    ///////////////// initialize the tcp stack///////
	tcpip_adapter_init();/// using get information of wifi
    /////////////////

    ///////// make a event group to manage //////
    s_wifi_event_group = xEventGroupCreate();
    ////////////////

    ///////////// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );
        ////////////////

    ///// config and connect wifi////
    wifi_init_sta();
    ///////////////////////////////////////////////////
    demo=xQueueCreate(4,sizeof(double));

    xTaskCreatePinnedToCore(&getTask,"getTask",4096*2,NULL,2,NULL, 0);
    //xTaskCreate(&getTask,"getTask",4096*2,NULL,4,NULL);
   // xTaskCreate(&postTask,"postTask",4096*2,NULL,4,NULL);
   //xTaskCreatePinnedToCore(&makeJson,"makeJson",4096*2,NULL,2,NULL, 1);
   xTaskCreatePinnedToCore(&postTask,"postTask",4096*2,NULL,2,NULL, 1);
}