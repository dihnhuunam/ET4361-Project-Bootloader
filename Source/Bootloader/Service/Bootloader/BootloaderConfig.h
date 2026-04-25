/**
 * @file BootloaderConfig.h
 * @brief Bootloader memory map, slot definitions, and range helpers.
 */
#ifndef BOOTLOADER_CONFIG_H
#define BOOTLOADER_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

#define BOOT_FLASH_BASE_ADDRESS 0x08000000UL
#define BOOT_FLASH_PAGE_SIZE 2048UL
#define BOOT_FLASH_TOTAL_SIZE (256UL * 1024UL)
#define BOOT_FLASH_END_ADDRESS (BOOT_FLASH_BASE_ADDRESS + BOOT_FLASH_TOTAL_SIZE - 1UL)

#define BOOT_SRAM_BASE_ADDRESS 0x20000000UL
#define BOOT_SRAM_TOTAL_SIZE (20UL * 1024UL)
#define BOOT_SRAM_END_ADDRESS (BOOT_SRAM_BASE_ADDRESS + BOOT_SRAM_TOTAL_SIZE - 1UL)

#define BOOTLOADER_REGION_START 0x08000000UL
#define BOOTLOADER_REGION_SIZE (32UL * 1024UL)
#define BOOTLOADER_REGION_END (BOOTLOADER_REGION_START + BOOTLOADER_REGION_SIZE - 1UL)

#define METADATA_REGION_START 0x08008000UL
#define METADATA_REGION_SIZE (8UL * 1024UL)
#define METADATA_REGION_END (METADATA_REGION_START + METADATA_REGION_SIZE - 1UL)

#define RESERVED_REGION_START 0x0800A000UL
#define RESERVED_REGION_SIZE (8UL * 1024UL)
#define RESERVED_REGION_END (RESERVED_REGION_START + RESERVED_REGION_SIZE - 1UL)

#define APP1_REGION_START 0x0800C000UL
#define APP1_REGION_SIZE (104UL * 1024UL)
#define APP1_REGION_END (APP1_REGION_START + APP1_REGION_SIZE - 1UL)

#define APP2_REGION_START 0x08026000UL
#define APP2_REGION_SIZE (104UL * 1024UL)
#define APP2_REGION_END (APP2_REGION_START + APP2_REGION_SIZE - 1UL)

#define METADATA_MAGIC 0x424F4F54UL
#define METADATA_FORMAT_VERSION 1UL
#define METADATA_MAX_BOOT_ATTEMPTS 3UL

    /** @brief Identifies the available application slots. */
    typedef enum
    {
        BOOT_SLOT_NONE = 0U,
        BOOT_SLOT_APP1 = 1U,
        BOOT_SLOT_APP2 = 2U
    } BootSlot_t;

    /** @brief Describes the runtime state of an application slot. */
    typedef enum
    {
        SLOT_STATE_EMPTY = 0U,
        SLOT_STATE_VALID = 1U,
        SLOT_STATE_PENDING = 2U,
        SLOT_STATE_CONFIRMED = 3U,
        SLOT_STATE_INVALID = 4U
    } BootSlotState_t;

    /** @brief Defines the flash address range assigned to a boot slot. */
    typedef struct
    {
        uint32_t start_address;
        uint32_t size;
    } BootSlotRegion_t;

    /**
     * @brief Returns the flash region assigned to a slot.
     * @param slot Target boot slot.
     * @return Pointer to the slot region description, or null on invalid slot.
     */
    const BootSlotRegion_t *BootConfig_GetSlotRegion(BootSlot_t slot);

    /**
     * @brief Returns the opposite application slot.
     * @param slot Source boot slot.
     * @return The opposite application slot, or BOOT_SLOT_NONE for invalid input.
     */
    BootSlot_t BootConfig_GetOtherSlot(BootSlot_t slot);

    /**
     * @brief Checks whether a flash range is inside the device flash map.
     * @param address Start address of the range.
     * @param length Range length in bytes.
     * @return true if the full range is inside device flash, otherwise false.
     */
    bool BootConfig_IsFlashRangeValid(uint32_t address, uint32_t length);

    /**
     * @brief Checks whether a range stays inside the specified application slot.
     * @param slot Target application slot.
     * @param address Start address of the range.
     * @param length Range length in bytes.
     * @return true if the full range is inside the selected slot, otherwise false.
     */
    bool BootConfig_IsAppRangeValid(BootSlot_t slot, uint32_t address, uint32_t length);

    /**
     * @brief Checks whether an address is inside the configured SRAM window.
     * @param address Address to validate.
     * @return true if the address is inside the configured SRAM range, otherwise false.
     */
    bool BootConfig_IsRamAddressValid(uint32_t address);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_CONFIG_H */
