#ifndef __DIFY_NVS_H
#define __DIFY_NVS_H

#ifdef __cplusplus
extern "C" {
#endif

/* library for Freetos API */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Library NVS memory  */
#include "esp_partition.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG_NVS = "NVS";

void open_repository(char *repository,nvs_handle_t *nvs_handler);
void erase_all_nvs(void);
esp_err_t write_nvs(char *repository,nvs_handle_t *nvs_handler, char *key , void *value,nvs_type_t type);
#ifdef __cplusplus
}
#endif

#endif