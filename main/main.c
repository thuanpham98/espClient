/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"

#include "esp_partition.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

char ID[13];
uint32_t device ;
uint8_t wifiMode = 0;
uint8_t user_wifi[33];
uint8_t pass_wifi[65];

static void earse_all_nvs(void);
static void write_nvs(void);

// NVS handler
nvs_handle_t my_handle;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const int WIFI_FAIL_BIT = BIT2;

static const char *TAG = "smartconfig_example";
static const char *TAG_NVS = "NVS";


static void smartconfig_example_task(void * parm);

static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if(wifiMode==1)
        {
            esp_wifi_connect();
        }
        else
        {
            xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
        }
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // esp_wifi_connect();
        // xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        if (s_retry_num < 15) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);

        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };

        uint8_t pass[65] = { 0 };
        char *temp_id = (char * )malloc(13*sizeof(char));
        char *temp_dev = (char *)malloc(10*sizeof(char));

        
        uint8_t j =0,k=0,i=0,index_id=0,index_dev=0,index_pass=0;
        for(uint8_t n = 0 ; n < 32; n++)
        {
            if(evt->password[n]=='.')
            {
                j++;
                continue;
            }
            if(j==0)
            {
                *(n + temp_id)=evt->password[n];
                index_id ++;
            }
            if(j==1)
            {
                *(k + temp_dev)=evt->password[n];
                k++;
                index_dev++;
            }
            if(j==2)
            {
                pass[i]=evt->password[n];
                i++;
                index_pass++;
                
            }
        }
        for(int x = 0 ; x < index_id ; x ++ )
        {
            ID[x] =*(x+ temp_id);
        }
        char *subDev = (char *)malloc(index_dev*sizeof(char));
        for(int x = 0 ; x < index_dev ; x ++ )
        {
            *(x + subDev)=*(x+ temp_dev);
        }
        device=atoi(subDev);
        ESP_LOGI(TAG,"%d",device);
        ESP_LOGI(TAG,"%s",ID);

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid,evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid , sizeof(evt->ssid));
        memcpy(password, pass, sizeof(pass));

        memcpy(user_wifi, evt->ssid , sizeof(evt->ssid));
        memcpy(pass_wifi, pass, sizeof(pass));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);

        free(subDev);
        free(temp_dev);
        free(temp_id);

        earse_all_nvs();
	    // if it is invalid, try to erase it

	    write_nvs();

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
        ESP_ERROR_CHECK( esp_wifi_connect() );
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

static void initialise_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

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
    memcpy(wifi_config.sta.ssid,user_wifi, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, pass_wifi, sizeof(wifi_config.sta.password));

    // wifi_config_t wifi_config = {
    //     .sta = {
    //         .ssid = "lau101",
    //         .password = "9999999",
    //     },
    // };
    ESP_LOGE(TAG,"%s",wifi_config.sta.ssid);
    ESP_LOGE(TAG,"%s",wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap ");
    }
    else if (bits & WIFI_FAIL_BIT)
    {
		esp_err_t err = nvs_erase_all(my_handle);
		if(err != ESP_OK) {
			ESP_LOGE(TAG," error! (%04X)\n", err);
			esp_restart();
		}
		err = nvs_commit(my_handle);	
		if(err != ESP_OK) {
			ESP_LOGE(TAG," error in commit! (%04X)\n", err);
			esp_restart();
		}
		ESP_LOGI(TAG,"Done erase");
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

static void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY); 
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            ESP_LOGI(TAG,"Eveything is ok");
            vTaskDelete(NULL);
            ESP_LOGI(TAG,"OK r nha");
        }
    }
}

void test (void *pv)
{
    while (1)
    {
        ESP_LOGI(TAG,"thuan dep tai");
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    
}
void earse_all_nvs()
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
		
		ESP_LOGE(TAG_NVS,"Got NO_FREE_PAGES error, trying to erase the partition...\n");
		
		// find the NVS partition
        const esp_partition_t* nvs_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);      
		if(!nvs_partition) {
			
			ESP_LOGE(TAG_NVS,"FATAL ERROR: No NVS partition found\n");
			while(1) vTaskDelay(10 / portTICK_PERIOD_MS);
		}
		
		// erase the partition
        err = (esp_partition_erase_range(nvs_partition, 0, nvs_partition->size));
		if(err != ESP_OK) {
			ESP_LOGE(TAG_NVS,"FATAL ERROR: Unable to erase the partition\n");
			while(1) vTaskDelay(10 / portTICK_PERIOD_MS);
		}
		ESP_LOGI(TAG_NVS,"Partition erased!\n");
		
		// now try to initialize it again
		err = nvs_flash_init();
		if(err != ESP_OK) {
			
			ESP_LOGE(TAG_NVS,"FATAL ERROR: Unable to initialize NVS\n");
			while(1) vTaskDelay(10 / portTICK_PERIOD_MS);
		}
	}
	ESP_LOGI(TAG_NVS,"NVS init OK!\n");
}
void write_nvs()
{
    // open the partition in RW mode
	esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
		
		ESP_LOGE(TAG_NVS,"FATAL ERROR: Unable to open NVS\n");
		while(1) vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	ESP_LOGI(TAG_NVS,"NVS open OK\n");

    err = nvs_set_str(my_handle, "USER", (char *)user_wifi);
    if(err != ESP_OK) {
        ESP_LOGE(TAG_NVS,"\nError in nvs_set_str! (%04X)\n", err);
        return;
    }
    
    err = nvs_set_str(my_handle, "PASS", (char *)pass_wifi);
    if(err != ESP_OK) {
        ESP_LOGE(TAG_NVS,"\nError in nvs_set_str! (%04X)\n", err);
        return;
    }
    
    err = nvs_set_str(my_handle, "ID", (char *)ID);
    if(err != ESP_OK) {
        ESP_LOGE(TAG_NVS,"\nError in nvs_set_str! (%04X)\n", err);
        return;
    }

    err = nvs_set_u32(my_handle, "DEV", device);
    if(err != ESP_OK) {
        ESP_LOGE(TAG_NVS,"\nError in nvs_set_str! (%04X)\n", err);
        return;
    }

    err = nvs_commit(my_handle);
    if(err != ESP_OK) {
        ESP_LOGE(TAG_NVS,"\nError in commit! (%04X)\n", err);
        return;
    }
}

void app_main(void)
{
	// initialize NVS flash
	esp_err_t err = nvs_flash_init();
    // earse_all_nvs();
    wifiMode=1;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
		
		ESP_LOGE(TAG_NVS,"FATAL ERROR: Unable to open NVS\n");
		while(1) vTaskDelay(10 / portTICK_PERIOD_MS);
	}
	ESP_LOGI(TAG_NVS,"NVS open OK\n");
    size_t string_size;
    //GET data from NVS
    //-----------get USER
    err = nvs_get_str(my_handle, "USER", NULL, &string_size);
    if(err != ESP_OK) {
        ESP_LOGE(TAG_NVS,"Error in nvs_get_str to get string size! (%04X)\n", err);
        wifiMode=0;
        ESP_LOGE(TAG_NVS,"Error in  get user");
    }
    char* value = malloc(string_size);
    err = nvs_get_str(my_handle, "USER", value, &string_size);
    if(err != ESP_OK) {
        if(err == ESP_ERR_NVS_NOT_FOUND) 
        {
            ESP_LOGE(TAG_NVS,"Key not found\n");
        }
        ESP_LOGE(TAG_NVS,"Error in nvs_get_str to get string! (%04X)\n", err);
        wifiMode=0;
        ESP_LOGE(TAG_NVS,"Error in  get pass");
    }
    else
    {
        memcpy(user_wifi,value, string_size);
        ESP_LOGE(TAG_NVS,"%s",user_wifi);
    }
    

    //---------------------GET PASS WIFI
    err = nvs_get_str(my_handle, "PASS", NULL, &string_size);
    if(err != ESP_OK) {
        ESP_LOGE(TAG_NVS,"\nError in nvs_get_str to get string size! (%04X)\n", err);
        wifiMode=0;
    }
    value = malloc(string_size);
    err = nvs_get_str(my_handle, "PASS", value, &string_size);
    if(err != ESP_OK) {
        if(err == ESP_ERR_NVS_NOT_FOUND) 
        {
            ESP_LOGE(TAG_NVS,"\nKey not found\n");
        }
        ESP_LOGE(TAG_NVS,"\nError in nvs_get_str to get string! (%04X)\n", err);
        wifiMode=0;
    }
    else
    {
        memcpy(pass_wifi,value, string_size);
        ESP_LOGE(TAG_NVS,"%s",pass_wifi);
    }
    
    //-------------------- GET ID
    err = nvs_get_str(my_handle, "ID", NULL, &string_size);
    if(err != ESP_OK) {
        ESP_LOGE(TAG_NVS,"\nError in nvs_get_str to get string size! (%04X)\n", err);
        wifiMode=0;
    }
    value = malloc(string_size);
    err = nvs_get_str(my_handle, "ID", value, &string_size);
    if(err != ESP_OK) {
        if(err == ESP_ERR_NVS_NOT_FOUND) 
        {
            ESP_LOGE(TAG_NVS,"\nKey not found\n");
        }
        ESP_LOGE(TAG_NVS,"\nError in nvs_get_str to get string! (%04X)\n", err);
        wifiMode=0;
    }
    else
    {
        memcpy(ID,value, string_size);
        ESP_LOGE(TAG_NVS,"%s",ID);
    }
    free(value);
    //--------------- GET DEV
    uint32_t value_dev;
    err = nvs_get_u32(my_handle, "DEV", &value_dev);

    if(err != ESP_OK) {
        if(err == ESP_ERR_NVS_NOT_FOUND) 
        {
            ESP_LOGE(TAG_NVS,"\nKey not found\n");
        }
        ESP_LOGE(TAG_NVS,"\nError in nvs_get_str to get string! (%04X)\n", err);
        wifiMode=0;
    }
    else
    {
        device=value_dev;
        ESP_LOGE(TAG_NVS,"%d",device);
    }
    
    ESP_LOGI(TAG_NVS,"%d",wifiMode);

    if(wifiMode==1)
    {
        wifi_init_sta();
    }
    else 
    {
        initialise_wifi();
    }
    xTaskCreate(test,"test",4096,NULL,3,NULL);
}

