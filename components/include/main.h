#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Basic library on C */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* library for Freetos API */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

/* error library */
#include "esp_err.h"

typedef struct
{
    char ID[13];
    uint32_t device;
    uint8_t user_wifi[33];
    uint8_t pass_wifi[65];
    uint16_t reg_digi;
    uint16_t reg_dac;
    uint16_t reg_pwm;

}esp_parameter_t;

#ifdef __cplusplus
}
#endif

#endif