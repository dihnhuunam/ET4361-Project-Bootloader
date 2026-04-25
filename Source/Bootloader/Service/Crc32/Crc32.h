#ifndef CRC32_SERVICE_H
#define CRC32_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f1xx_hal.h"

#include <stdint.h>

    HAL_StatusTypeDef Crc32_CalculateBuffer(const uint8_t *data, uint32_t length, uint32_t *crc_out);
    HAL_StatusTypeDef Crc32_CalculateFlash(uint32_t address, uint32_t length, uint32_t *crc_out);

#ifdef __cplusplus
}
#endif

#endif /* CRC32_SERVICE_H */
