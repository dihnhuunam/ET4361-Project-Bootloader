#include "BootloaderConfig.h"

static const BootSlotRegion_t k_slot_regions[] = {
    {0U, 0U},
    {APP1_REGION_START, APP1_REGION_SIZE},
    {APP2_REGION_START, APP2_REGION_SIZE},
};

const BootSlotRegion_t *BootConfig_GetSlotRegion(BootSlot_t slot)
{
    if ((uint32_t)slot >= (sizeof(k_slot_regions) / sizeof(k_slot_regions[0])))
    {
        return 0;
    }

    return &k_slot_regions[slot];
}

BootSlot_t BootConfig_GetOtherSlot(BootSlot_t slot)
{
    if (slot == BOOT_SLOT_APP1)
    {
        return BOOT_SLOT_APP2;
    }

    if (slot == BOOT_SLOT_APP2)
    {
        return BOOT_SLOT_APP1;
    }

    return BOOT_SLOT_NONE;
}

bool BootConfig_IsFlashRangeValid(uint32_t address, uint32_t length)
{
    uint32_t end_address;

    if (length == 0U)
    {
        return false;
    }

    if ((address < BOOT_FLASH_BASE_ADDRESS) || (address > BOOT_FLASH_END_ADDRESS))
    {
        return false;
    }

    end_address = address + length - 1U;
    if (end_address < address)
    {
        return false;
    }

    return (end_address <= BOOT_FLASH_END_ADDRESS);
}

bool BootConfig_IsAppRangeValid(BootSlot_t slot, uint32_t address, uint32_t length)
{
    const BootSlotRegion_t *region;
    uint32_t end_address;

    region = BootConfig_GetSlotRegion(slot);
    if ((region == 0) || (region->size == 0U) || (length == 0U))
    {
        return false;
    }

    if (address < region->start_address)
    {
        return false;
    }

    end_address = address + length - 1U;
    if (end_address < address)
    {
        return false;
    }

    return end_address <= (region->start_address + region->size - 1U);
}

bool BootConfig_IsRamAddressValid(uint32_t address)
{
    return (address >= BOOT_SRAM_BASE_ADDRESS) && (address <= BOOT_SRAM_END_ADDRESS);
}
