#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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