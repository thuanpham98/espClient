#ifndef __DIFU_I2C_H
#define __DIFU_I2C_H

#include "driver/i2c.h"
#include "esp_err.h"

#define I2C_MASTER_FREQ_HZ_STANDARD_MODE 100000
#define I2C_MASTER_FREQ_HZ_FAST_MODE 400000

#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define WRITE_BIT I2C_MASTER_WRITE
#define READ_BIT I2C_MASTER_READ
#define ACK_CHECK_EN I2C_MASTER_NACK
#define ACK_CHECK_DIS I2C_MASTER_ACK
#define ACK_VAL I2C_MASTER_ACK
#define NACK_VAL I2C_MASTER_NACK

#define I2C_MASTER_NUM_STANDARD_MODE I2C_NUM_0
#define I2C_MASTER_NUM_FAST_MODE I2C_NUM_1

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t init_i2c(i2c_config_t *cf,i2c_port_t port);
esp_err_t master_write_i2c(i2c_port_t port,uint8_t *data_wr, size_t size, uint8_t address_slave);
esp_err_t master_read_i2c(i2c_port_t port,uint8_t *data_rd, size_t size, uint8_t address_slave);

#ifdef __cplusplus
}
#endif

#endif /* DIFU_I2C_h */
