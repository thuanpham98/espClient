#ifndef __CONVERTTIME_H
#define __CONVERTTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

uint64_t date_to_timestamp(uint16_t s,uint16_t m,uint16_t h,uint16_t D,uint16_t M, uint16_t Y,uint16_t timezone);


#ifdef __cplusplus
}
#endif

#endif