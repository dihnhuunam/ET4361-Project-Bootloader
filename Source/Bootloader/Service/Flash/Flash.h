/**
 * @file Flash.h
 * @brief Flash access helpers used by metadata and image update flows.
 */
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

    /**
     * @brief Checks whether a range is inside device flash.
     * @param address Start address of the range.
     * @param length Range length in bytes.
     * @return true if the full range is inside flash, otherwise false.
     */
    bool Flash_IsValidRange(uint32_t address, uint32_t length);

    /**
     * @brief Checks whether a range is writable by the bootloader.
     * @param address Start address of the range.
     * @param length Range length in bytes.
     * @return true if the full range is writable, otherwise false.
     */
    bool Flash_IsWritableRange(uint32_t address, uint32_t length);

    /**
     * @brief Returns the flash page base address that contains the given address.
     * @param address Address inside the target page.
     * @return Base address of the containing flash page.
     */
    uint32_t Flash_GetPageAddress(uint32_t address);

    /**
     * @brief Returns the number of flash pages covered by a range.
     * @param address Start address of the range.
     * @param length Range length in bytes.
     * @return Number of covered flash pages, or 0 for an invalid range.
     */
    uint32_t Flash_GetPageCount(uint32_t address, uint32_t length);

    /**
     * @brief Reads bytes from flash into RAM.
     * @param address Source flash address.
     * @param data Destination RAM buffer.
     * @param length Number of bytes to read.
     * @return HAL_OK on success, otherwise HAL_ERROR.
     */
    HAL_StatusTypeDef Flash_Read(uint32_t address, uint8_t *data, uint32_t length);

    /**
     * @brief Erases one or more writable flash pages.
     * @param address Start address of the erase range.
     * @param length Erase length in bytes.
     * @return HAL status returned by the erase flow.
     */
    HAL_StatusTypeDef Flash_Erase(uint32_t address, uint32_t length);

    /**
     * @brief Programs bytes into writable flash memory.
     * @param address Destination flash address.
     * @param data Source buffer in RAM.
     * @param length Number of bytes to write.
     * @return HAL status returned by the write flow.
     */
    HAL_StatusTypeDef Flash_Write(uint32_t address, const uint8_t *data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_SERVICE_H */
