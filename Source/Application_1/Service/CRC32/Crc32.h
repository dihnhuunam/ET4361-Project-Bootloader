/**
 * @file Crc32.h
 * @brief CRC32 helper functions backed by the STM32 CRC peripheral.
 */
#ifndef CRC32_SERVICE_H
#define CRC32_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

#include <stdint.h>

/**
 * @brief Calculates a CRC32 over a memory buffer.
 * @param data Input buffer.
 * @param length Buffer length in bytes.
 * @param crc_out Output CRC value.
 * @return HAL_OK on success, otherwise HAL_ERROR.
 */
HAL_StatusTypeDef Crc32_CalculateBuffer(const uint8_t *data, uint32_t length,
                                        uint32_t *crc_out);

/**
 * @brief Calculates a CRC32 over a flash region.
 * @param address Start address of the flash region.
 * @param length Region length in bytes.
 * @param crc_out Output CRC value.
 * @return HAL_OK on success, otherwise HAL_ERROR.
 */
HAL_StatusTypeDef Crc32_CalculateFlash(uint32_t address, uint32_t length,
                                       uint32_t *crc_out);

#ifdef __cplusplus
}
#endif

#endif /* CRC32_SERVICE_H */
