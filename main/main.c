// Coppy All Right: Phạm Minh Thuận
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_http_client.h"
#include "esp_transport.h"
#include "esp_transport_tcp.h"

#include <limits.h>
#include <ctype.h>
#include <cJSON.h>
#include <cJSON_Utils.h>

#include "unity.h"

#include "message.pb-c.h"


#define EXAMPLE_ESP_WIFI_SSID      CONFIG_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  100
#define URL                        CONFIG_URL
#define MAX_HTTP_RECV_BUFFER       512
#define ID                         CONFIG_ID_DEVICE
static const char *TAG_HTTP = "HTTP";
char temp[512];

static EventGroupHandle_t s_wifi_event_group;


#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

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
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

//-------make even for http---------//
esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
        switch(evt->event_id)
        {
            case HTTP_EVENT_ERROR:
               // ESP_LOGI(TAG_HTTP, "HTTP_EVENT_ERROR");
                break;
            case HTTP_EVENT_ON_CONNECTED:
               // ESP_LOGI(TAG_HTTP, "HTTP_EVENT_ON_CONNECTED");
                break;
            case HTTP_EVENT_HEADER_SENT:
               // ESP_LOGI(TAG_HTTP, "HTTP_EVENT_HEADER_SENT");
                break;
            case HTTP_EVENT_ON_HEADER:
                //ESP_LOGI(TAG_HTTP, "HTTP_EVENT_ON_HEADER");
                //printf("%.*s", evt->data_len, (char*)evt->data);
                break;
            case HTTP_EVENT_ON_DATA:
               // ESP_LOGI(TAG_HTTP, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
                if (!esp_http_client_is_chunked_response(evt->client)) {
                    //printf("%.*s", evt->data_len, (char*)evt->data);
                    for (int i = 0; i < evt->data_len; i++)
                    {
                        temp[i]=*(((char*)evt->data) +i);
                    }
                }
                break;
            case HTTP_EVENT_ON_FINISH:
                //ESP_LOGI(TAG_HTTP, "HTTP_EVENT_ON_FINISH");
                break;
            case HTTP_EVENT_DISCONNECTED:
               // ESP_LOGI(TAG_HTTP, "HTTP_EVENT_DISCONNECTED");
                break;
        }
        return ESP_OK;
}

Sensor *protoc(char *message);


/////------------make json to post------------------///
// char* Print_JSON(char* id,double data[10])
// {
//     cJSON* sudo =cJSON_CreateObject();
//     cJSON* form=cJSON_CreateObject();
//     cJSON_AddItemToObject(sudo, "ID",cJSON_CreateString(id));
//     cJSON_AddItemToObject(sudo, "form",form);

//     cJSON_AddNumberToObject(form,"sensor_1",data[0]);
//     cJSON_AddNumberToObject(form,"sensor_2",data[1]);
//     cJSON_AddNumberToObject(form,"sensor_3",data[2]);
//     cJSON_AddNumberToObject(form,"sensor_4",data[3]);

//     cJSON_AddNumberToObject(form,"sensor_5",data[4]);
//     cJSON_AddNumberToObject(form,"sensor_6",data[5]);
//     cJSON_AddNumberToObject(form,"sensor_7",data[6]);

//     cJSON_AddNumberToObject(form,"sensor_8",data[7]);
//     cJSON_AddNumberToObject(form,"sensor_9",data[8]);
//     cJSON_AddNumberToObject(form,"sensor_10",data[9]);
//     char *a=cJSON_Print(sudo);
//     cJSON_Delete(sudo);//if don't free, heap memory will be overload
//     return a;
// }
//-------Post method----------//
// void postTask (void *pv)
// {
//     ESP_LOGI(TAG_HTTP," Init http Post");
    
//     double data[10];
//     esp_http_client_config_t config = {
//         .url=URL,
//         .event_handler = _http_event_handle,
//     };
//     esp_http_client_handle_t client = esp_http_client_init(&config);
//     esp_http_client_set_method(client, HTTP_METHOD_POST);
//     esp_http_client_set_header(client, "Content-Type", "application/json");

//     while(1)
//     {
//         char* post_data = (char *) malloc(512);
//         post_data = Print_JSON(ID,data);
//         esp_http_client_set_post_field(client,post_data,strlen(post_data));

//         esp_err_t err = esp_http_client_perform(client);
//         if (err == ESP_OK) {
//             ESP_LOGI(TAG_HTTP, "HTTP GET Status = %d, content_length = %d",
//                     esp_http_client_get_status_code(client),
//                     esp_http_client_get_content_length(client));

//             data[0]--;data[1]+=5;data[2]++;data[3]++;data[4]++;data[5]++;data[6]++;data[7]++;data[8]++;data[9]++;
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

//         vTaskDelay(1000/portTICK_PERIOD_MS);
//     }
//     esp_http_client_close(client);
//     esp_http_client_cleanup(client);
//     esp_restart();
//     vTaskDelete(NULL);
// }


//----------get method-------//
void getTask(void *pv)
{
    ESP_LOGI(TAG_HTTP," Init http get");
    char *get_data = (char *) malloc(512);

    esp_http_client_config_t config = {
        .url=URL,
        .event_handler = _http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    
    //---for protoc-c---//
    Sensor *s=(Sensor *) malloc(sizeof(Sensor));
    sensor__init(s);

    while(1)
    {
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            ESP_LOGI(TAG_HTTP, "HTTP GET Status = %d, content_length = %d",
                    esp_http_client_get_status_code(client),
                    esp_http_client_get_content_length(client));


            if(esp_http_client_get_content_length(client))
            {
                get_data=temp;
                s=protoc(get_data);
                ESP_LOGI(TAG,"%f",s->value);


            }
            else get_data=" ";
 
            
        } 
        else 
        {
            ESP_LOGE(TAG_HTTP, "HTTP GET request failed: %s", esp_err_to_name(err));
            esp_restart();
            break;
        }

        //esp_http_client_cleanup(client);
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    esp_restart();
    vTaskDelete(NULL);
    
}

//----protoc -c ----//
Sensor *protoc(char *message)
{


    char *frame= (char *) malloc(3*sizeof(char));
    uint8_t buff[512];

    uint8_t j=0,k=0;
    for(uint8_t i = 0; i < (strlen(message)); i++)
    {
        if(*(message+i)==',')
        {
            continue;
        }
        *(frame + j)=*(message+i);
        j++;

        if((*(message+i+1)==',') ||(*(message+i+1)==NULL))
        {
            *(frame+j)=NULL; /// to end string
            buff[k]=(uint8_t)atoi(frame);// convert tring to number (stdlib.h)
            j=0;
            k++;
        }

    }
    free(frame);
    return sensor__unpack(NULL,k,buff);
    //sensors__free_unpacked(s2,NULL);
    
}
//------------------//
//--------set up----//
void app_main(void)
{
    //----------------disable the default wifi logging----//
	esp_log_level_set("wifi", ESP_LOG_NONE);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA \r \n");
    wifi_init_sta();

    //------start FreeRtos---//
    //xTaskCreate(&postTask,"postTask",4096*2,NULL,3,NULL);
    xTaskCreate(&getTask,"getTask",4096*3,NULL,2,NULL);

}