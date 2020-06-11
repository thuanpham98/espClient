#ifndef __CONVERTTIME_H
#define __CONVERTTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

void set_time_zone(void);
void sync_time(uint8_t type, uint8_t time_buffer[7]);
uint64_t get_timestamp(void);
void get_time_str(char time_display[64]);

#ifdef __cplusplus
}
#endif

#endif