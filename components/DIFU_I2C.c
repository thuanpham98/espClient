#include "include/DIFU_I2C.h"

esp_err_t init_i2c(i2c_config_t *cf,i2c_port_t port)
{
    i2c_config_t config;
    config.mode = cf->mode;
    config.sda_io_num = cf->sda_io_num;
    config.sda_pullup_en = cf->sda_pullup_en;
    config.scl_io_num = cf->scl_io_num;
    config.scl_pullup_en = cf->scl_pullup_en;
    config.master.clk_speed = cf->master.clk_speed;
    i2c_param_config(port, &config);
    return i2c_driver_install(port,cf->mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

esp_err_t master_write_i2c(i2c_port_t port,uint8_t *data_wr, size_t size, uint8_t address_slave)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address_slave << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t master_read_i2c(i2c_port_t port,uint8_t *data_rd, size_t size, uint8_t address_slave)
{
    if (size == 0)
    {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address_slave << 1) | READ_BIT, ACK_CHECK_EN);
    if (size > 1)
    {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}