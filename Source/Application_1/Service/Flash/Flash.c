/**
 * @file Flash.c
 * @brief Implements flash range validation, erase, read, and write helpers.
 */
#include "Flash.h"

#include <string.h>

/**
 * @brief Checks whether a single address is inside the device flash map.
 * @param address Address to validate.
 *
 * @return true if the address is inside flash, otherwise false.
 */
static bool Flash_IsAddressInRange(uint32_t address) {
  return (address >= FLASH_SERVICE_BASE_ADDRESS) &&
         (address <= FLASH_SERVICE_END_ADDRESS);
}

/**
 * @brief Checks whether a single address is inside the writable flash window.
 * @param address Address to
 * validate.
 * @return true if the address is writable by the bootloader, otherwise false.
 */
static bool Flash_IsAddressWritable(uint32_t address) {
  return (address >= FLASH_SERVICE_WRITE_START_ADDRESS) &&
         (address <= FLASH_SERVICE_WRITE_END_ADDRESS);
}

/**
 * @brief Checks whether a range is inside device flash.
 * @param address Start address of the range.
 * @param
 * length Range length in bytes.
 * @return true if the full range is valid, otherwise false.
 */
bool Flash_IsValidRange(uint32_t address, uint32_t length) {
  uint32_t end_address;

  if (length == 0U) {
    return false;
  }

  if (!Flash_IsAddressInRange(address)) {
    return false;
  }

  end_address = address + length - 1U;
  if (end_address < address) {
    return false;
  }

  return Flash_IsAddressInRange(end_address);
}

/**
 * @brief Checks whether a range is writable by the bootloader.
 * @param address Start address of the range.
 *
 * @param length Range length in bytes.
 * @return true if the full range is writable, otherwise false.
 */
bool Flash_IsWritableRange(uint32_t address, uint32_t length) {
  uint32_t end_address;

  if (!Flash_IsValidRange(address, length)) {
    return false;
  }

  end_address = address + length - 1U;
  return Flash_IsAddressWritable(address) &&
         Flash_IsAddressWritable(end_address);
}

/**
 * @brief Returns the flash page base address that contains the given address.
 * @param address Address inside the
 * target page.
 * @return Base address of the containing flash page.
 */
uint32_t Flash_GetPageAddress(uint32_t address) {
  return address - (address % FLASH_SERVICE_PAGE_SIZE);
}

/**
 * @brief Returns the number of flash pages covered by a range.
 * @param address Start address of the range.
 *
 * @param length Range length in bytes.
 * @return Number of covered pages, or 0 for an invalid range.
 */
uint32_t Flash_GetPageCount(uint32_t address, uint32_t length) {
  uint32_t start_page;
  uint32_t end_page;

  if (!Flash_IsValidRange(address, length)) {
    return 0U;
  }

  start_page = Flash_GetPageAddress(address);
  end_page = Flash_GetPageAddress(address + length - 1U);

  return ((end_page - start_page) / FLASH_SERVICE_PAGE_SIZE) + 1U;
}

/**
 * @brief Reads bytes from flash into RAM.
 * @param address Source flash address.
 * @param data Destination RAM
 * buffer.
 * @param length Number of bytes to read.
 * @return HAL_OK on success, otherwise HAL_ERROR.
 */
HAL_StatusTypeDef Flash_Read(uint32_t address, uint8_t *data, uint32_t length) {
  if ((data == NULL) || !Flash_IsValidRange(address, length)) {
    return HAL_ERROR;
  }

  memcpy(data, (const void *)address, length);
  return HAL_OK;
}

/**
 * @brief Erases one or more writable flash pages.
 * @param address Start address of the erase range.
 * @param
 * length Erase length in bytes.
 * @return HAL status returned by the erase flow.
 */
HAL_StatusTypeDef Flash_Erase(uint32_t address, uint32_t length) {
  FLASH_EraseInitTypeDef erase_init;
  HAL_StatusTypeDef status;
  uint32_t page_error = 0U;

  if (!Flash_IsWritableRange(address, length)) {
    return HAL_ERROR;
  }

  erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
  erase_init.PageAddress = Flash_GetPageAddress(address);
  erase_init.NbPages = Flash_GetPageCount(address, length);

  status = HAL_FLASH_Unlock();
  if (status != HAL_OK) {
    return status;
  }

  status = HAL_FLASHEx_Erase(&erase_init, &page_error);
  (void)HAL_FLASH_Lock();

  return status;
}

/**
 * @brief Programs bytes into writable flash memory.
 * @param address Destination flash address.
 * @param data
 * Source buffer in RAM.
 * @param length Number of bytes to write.
 * @return HAL status returned by the write flow.

 */
HAL_StatusTypeDef Flash_Write(uint32_t address, const uint8_t *data,
                              uint32_t length) {
  HAL_StatusTypeDef status;
  uint32_t index;
  uint16_t halfword;

  if ((data == NULL) || !Flash_IsWritableRange(address, length) ||
      ((address & 0x1U) != 0U)) {
    return HAL_ERROR;
  }

  status = HAL_FLASH_Unlock();
  if (status != HAL_OK) {
    return status;
  }

  for (index = 0U; index < length; index += 2U) {
    halfword = data[index];
    if ((index + 1U) < length) {
      halfword |= (uint16_t)((uint16_t)data[index + 1U] << 8U);
    } else {
      halfword |= 0xFF00U;
    }

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address + index,
                               halfword);
    if (status != HAL_OK) {
      break;
    }

    if (*(const uint16_t *)(address + index) != halfword) {
      status = HAL_ERROR;
      break;
    }
  }

  (void)HAL_FLASH_Lock();
  return status;
}
