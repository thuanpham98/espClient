#include "include/DIFY_NVS.h"

void open_repository(char *repository,nvs_handle_t *nvs_handler)
{
    esp_err_t err ;
    err = nvs_open(repository, NVS_READWRITE, nvs_handler);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "FATAL ERROR: can not open NVS");
        while (1)
            vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG_NVS, "NVS open OK");
}
void erase_all_nvs(void)
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
esp_err_t delete_key_value(nvs_handle_t *nvs_handler, const char *key)
{
    esp_err_t err=nvs_erase_key(*nvs_handler, key);
    err= err| (nvs_commit(*nvs_handler));
    if(err==ESP_OK)
    {
        ESP_LOGI(TAG_NVS,"delete key-value suces");
        return ESP_OK;
    }
    else
    {
        ESP_LOGE(TAG_NVS,"delete fail");
        return -1;
    }
}
esp_err_t write_nvs(char *repository,nvs_handle_t *nvs_handle, char *key , void *value,nvs_type_t type)
{
    /* open the partition in RW mode */
    esp_err_t err ;
    nvs_handle_t nvs_handler;
    nvs_handler = *nvs_handle;
    open_repository(repository,nvs_handle);

    switch ((uint8_t)type)
    {
    case NVS_TYPE_U8:
        err = nvs_set_u8(nvs_handler,key,*((uint8_t *)value));
        break;
    case NVS_TYPE_I8:
        err = nvs_set_i8(nvs_handler,key,*((int8_t *)value));
        break;
    case NVS_TYPE_U16:
        err = nvs_set_u16(nvs_handler,key,*((uint16_t *)value));
        break;
    case NVS_TYPE_I16:
        err =nvs_set_i16(nvs_handler,key,*((int16_t *)value));
        break;
    case NVS_TYPE_U32:
        err =nvs_set_u32(nvs_handler,key,*((uint32_t *)value));
        break;
    case NVS_TYPE_I32:
        err =nvs_set_i32(nvs_handler,key,*((int32_t *)value));
        break;
    case NVS_TYPE_U64:
        err =nvs_set_u64(nvs_handler,key,*((uint64_t *)value));
        break;
    case NVS_TYPE_I64:
        err =nvs_set_i64(nvs_handler,key,*((int64_t *)value));
        break;
    case NVS_TYPE_STR:
        err = nvs_set_str(nvs_handler, key, (char *)value);
        break;
    default:
        err=-1;
        ESP_LOGE(TAG_NVS,"not found type of value");
        break;
    }
    err= err | (nvs_commit(nvs_handler));

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error in Set NVS");
        return -1;
    }
    else
    {
        ESP_LOGE(TAG_NVS, "Set key-value OK");
        return ESP_OK;
    }
}
esp_err_t read_nvs(char *repository,nvs_handle_t *nvs_handle, char *key ,void *value ,nvs_type_t type)
{
    nvs_handle_t nvs_handler;
    nvs_handler = *nvs_handle;
    open_repository(repository,nvs_handle);
    esp_err_t err ;
    size_t dismen;
    switch (type)
    {
    case NVS_TYPE_U8:
        err = nvs_get_u8(nvs_handler, key, (uint8_t *)value);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        break;
    case NVS_TYPE_I8:
        err = nvs_get_i8(nvs_handler, key, (int8_t *)value);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        break;
    case NVS_TYPE_U16:
        err = nvs_get_u16(nvs_handler, key, (uint16_t *)value);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        break;
    case NVS_TYPE_I16:
        err = nvs_get_i16(nvs_handler, key, (int16_t *)value);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        break;
    case NVS_TYPE_U32:
        err = nvs_get_u32(nvs_handler, key, (uint32_t *)value);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        break;
    case NVS_TYPE_I32:
        err = nvs_get_i32(nvs_handler, key, (int32_t *)value);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        break;
    case NVS_TYPE_U64:
        err = nvs_get_u64(nvs_handler, key, (uint64_t *)value);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        break;
    case NVS_TYPE_I64:
        err = nvs_get_i64(nvs_handler, key, (int64_t *)value);
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG_NVS, "Key not found");
        }
        break;
    case NVS_TYPE_STR:
            err = nvs_get_str(nvs_handler, key, NULL, &dismen);
            if (err != ESP_OK)
            {
                if (err == ESP_ERR_NVS_NOT_FOUND)
                {
                    ESP_LOGE(TAG_NVS, "Key not found");
                }
                break;
            }
            else
            {
                err = nvs_get_str(nvs_handler, key, (char *)value, &dismen);
                break;   
            }
    default:
        err=-1;
        break;
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error in Get NVS");
        value = NULL;
        return -1;
    }
    else
    {
        ESP_LOGE(TAG_NVS, "Get key-value OK");
        return ESP_OK;
    }
}