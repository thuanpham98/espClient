#include "include/sync_rtc.h"

static const char *TAG_TIME = "TIME";

/* init a SNTP to server  */
static void callback_sntp(struct timeval *tv)
{
    ESP_LOGI(TAG_TIME, "Notification of a time synchronization event");
}
static void initialize_sntp(void)
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "1.vn.pool.ntp.org");
    sntp_set_time_sync_notification_cb(callback_sntp);
    sntp_init();
}

static int bcdtodec(uint8_t num)
{
    return ((num >> 4) * 10 + (num & 0x0f));
}
void set_time_zone(void)
{
    setenv("TZ", "ICT-7", 1);
    tzset();
}

void sync_time(uint8_t type, uint8_t time_buffer[7])
{
    time_t now = 0;
    struct tm timeinfo = {0};
    struct timeval my_tv;
    if (type == 0)
    {
        initialize_sntp();
        // wait for time to be set

        int retry = 0;
        const int retry_count = 10;
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
        {
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    else
    {
        timeinfo.tm_sec = bcdtodec(time_buffer[0] & 0x7f);  /*!> second*/
        timeinfo.tm_min = bcdtodec(time_buffer[1]);         /*!> minute*/
        timeinfo.tm_hour = bcdtodec(time_buffer[2] & 0x3f); /*!> hour*/
        timeinfo.tm_wday = bcdtodec(time_buffer[3]) -1 ;    /*!> day week*/
        timeinfo.tm_mday = bcdtodec(time_buffer[4]);        /*!> day month*/
        timeinfo.tm_mon = bcdtodec(time_buffer[5]) -1;     /*!> month*/
        timeinfo.tm_year = bcdtodec(time_buffer[6]) + 100;  /*!> year*/

        ESP_LOGE(TAG_TIME,"%d",timeinfo.tm_sec);
        ESP_LOGE(TAG_TIME,"%d",timeinfo.tm_min);
        ESP_LOGE(TAG_TIME,"%d",timeinfo.tm_hour);
        ESP_LOGE(TAG_TIME,"%d",timeinfo.tm_mday);
        ESP_LOGE(TAG_TIME,"%d",timeinfo.tm_mon);
        ESP_LOGE(TAG_TIME,"%d",timeinfo.tm_year);

        my_tv.tv_sec = mktime(&timeinfo);
        my_tv.tv_usec = 0;
        settimeofday(&my_tv, NULL);

        ESP_LOGE(TAG_TIME,"%ld",my_tv.tv_sec);

        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

uint64_t get_timestamp(void)
{
    time_t now = 0;
    
    return (uint64_t)(time(&now));
}

void get_time_str(char time_display[64])
{
    time_t now = 0;
    struct tm timeinfo = {0};

    time(&now);
    localtime_r(&now, &timeinfo);
    
    strftime(time_display, sizeof(time_display)*8, "%c", &timeinfo);
}