#include "include/DIFY_NVS.h"

void open_repository(char *repository,nvs_handle_t *nvs_handler)
{
    esp_err_t err = nvs_flash_init();
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

esp_err_t write_nvs(char *repository,nvs_handle_t *nvs_handler, char *key , void *value,nvs_type_t type)
{
    /* open the partition in RW mode */
    esp_err_t err = nvs_open(repository, NVS_READWRITE, nvs_handler);
    if (err != ESP_OK)
    {

        ESP_LOGE(TAG_NVS, "FATAL ERROR: Unable to open NVS\n");
        while (1)
            vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG_NVS, "NVS open OK\n");

    switch (type)
    {
    case NVS_TYPE_U8:
        err =nvs_set_u8(*nvs_handle,key,*((uint8_t *)value))
        break;
    case NVS_TYPE_I8:
        err =nvs_set_i8(*nvs_handle,key,*((int8_t *)value))
        break;
    case NVS_TYPE_U16:
        err =nvs_set_u16(*nvs_handle,key,*((uint16_t *)value))
        break;
    case NVS_TYPE_I16:
        err =nvs_set_i16(*nvs_handle,key,*((int16_t *)value))
        break;
    case NVS_TYPE_U32:
        err =nvs_set_u32(*nvs_handle,key,*((uint32_t *)value))
        break;
    case NVS_TYPE_I32:
        err =nvs_set_i32(*nvs_handle,key,*((int32_t *)value))
        break;
    case NVS_TYPE_U64:
        err =nvs_set_u64(*nvs_handle,key,*((uint64_t *)value))
        break;
    case NVS_TYPE_I64:
        err =nvs_set_i64(*nvs_handle,key,*((int64_t *)value))
        break;
    case NVS_TYPE_STR:
        err = nvs_set_str(*nvs_handle, key, (char *)value);
        break;
    default:
        err=-1;
        break;
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_NVS, "Error in nvs_set_str! (%04X)", err);
        return -1;
    }
    else
    {
        ESP_LOGE(TAG_NVS, "Write key-value OK");
        return ESP_OK;
    }
    

    // /* set sspass of wifi */
    // err = nvs_set_str(my_handle, "PASS", (char *)my_esp.pass_wifi);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG_NVS, "Error in nvs_set_str! (%04X)", err);
    //     return -1;
    // }

    // /* set ID of esp */
    // err = nvs_set_str(my_handle, "ID", (char *)my_esp.ID);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG_NVS, "Error in nvs_set_str! (%04X)", err);
    //     return;
    // }

    // /* set number of dev */
    // err = nvs_set_u32(my_handle, "DEV", my_esp.device);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG_NVS, "Error in nvs_set_str! (%04X)", err);
    //     return;
    // }

    // /* set reg_digi */
    // err = nvs_set_u16(my_handle, "REG_DIGI", my_esp.reg_digi);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG_NVS, "Error in nvs_set_str! (%04X)", err);
    //     return;
    // }

    // /* set reg_dac */
    // err = nvs_set_u16(my_handle, "REG_DAC", my_esp.reg_dac);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG_NVS, "Error in nvs_set_str! (%04X)", err);
    //     return;
    // }

    // /* set reg_digi */
    // err = nvs_set_u16(my_handle, "REG_PWM", my_esp.reg_pwm);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG_NVS, "Error in nvs_set_str! (%04X)", err);
    //     return;
    // }

    // err = nvs_commit(my_handle);
    // if (err != ESP_OK)
    // {
    //     ESP_LOGE(TAG_NVS, "Error in commit! (%04X)", err);
    //     return;
    // }
}