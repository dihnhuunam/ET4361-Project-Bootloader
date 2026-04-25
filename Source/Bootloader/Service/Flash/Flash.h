#ifndef FLASH_SERVICE_H
#define FLASH_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "BootloaderConfig.h"
#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

#define FLASH_SERVICE_BASE_ADDRESS BOOT_FLASH_BASE_ADDRESS
#define FLASH_SERVICE_PAGE_SIZE BOOT_FLASH_PAGE_SIZE
#define FLASH_SERVICE_TOTAL_SIZE BOOT_FLASH_TOTAL_SIZE
#define FLASH_SERVICE_END_ADDRESS (FLASH_SERVICE_BASE_ADDRESS + FLASH_SERVICE_TOTAL_SIZE - 1U)
#define FLASH_SERVICE_WRITE_START_ADDRESS METADATA_REGION_START
#define FLASH_SERVICE_WRITE_END_ADDRESS APP2_REGION_END

    bool Flash_IsValidRange(uint32_t address, uint32_t length);
    bool Flash_IsWritableRange(uint32_t address, uint32_t length);
    uint32_t Flash_GetPageAddress(uint32_t address);
    uint32_t Flash_GetPageCount(uint32_t address, uint32_t length);
    HAL_StatusTypeDef Flash_Read(uint32_t address, uint8_t *data, uint32_t length);
    HAL_StatusTypeDef Flash_Erase(uint32_t address, uint32_t length);
    HAL_StatusTypeDef Flash_Write(uint32_t address, const uint8_t *data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_SERVICE_H */
