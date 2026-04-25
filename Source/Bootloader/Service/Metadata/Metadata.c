/**
 * @file Metadata.c
 * @brief Implements metadata record management and slot state updates.
 */
#include "Metadata.h"

#include "BootloaderConfig.h"
#include "Crc32.h"
#include "Flash.h"

#include <string.h>

#define METADATA_RECORD_ALIGNMENT 128U
#define METADATA_RECORD_COUNT (METADATA_REGION_SIZE / METADATA_RECORD_ALIGNMENT)

/**
 * @brief Returns the flash address of a metadata record slot.
 * @param index Metadata record index.
 * @return
 * Start address of the selected record.
 */
static uint32_t Metadata_GetRecordAddress(uint32_t index)
{
    return METADATA_REGION_START + (index * METADATA_RECORD_ALIGNMENT);
}

/**
 * @brief Recomputes and stores the CRC of a metadata object.
 * @param metadata Metadata object to update.
 *
 * @return HAL status of the CRC calculation.
 */
static HAL_StatusTypeDef Metadata_UpdateCrc(BootMetadata_t *metadata)
{
    if (metadata == 0)
    {
        return HAL_ERROR;
    }

    metadata->metadata_crc = 0U;
    return Crc32_CalculateBuffer((const uint8_t *)metadata, sizeof(BootMetadata_t) - sizeof(uint32_t),
                                 &metadata->metadata_crc);
}

/**
 * @brief Checks whether a metadata record slot is fully erased.
 * @param metadata Metadata record to inspect.
 *
 * @return true if all words are erased, otherwise false.
 */
static bool Metadata_IsErasedRecord(const BootMetadata_t *metadata)
{
    const uint32_t *words;
    uint32_t index;

    if (metadata == 0)
    {
        return false;
    }

    words = (const uint32_t *)metadata;
    for (index = 0U; index < (sizeof(BootMetadata_t) / sizeof(uint32_t)); index++)
    {
        if (words[index] != 0xFFFFFFFFUL)
        {
            return false;
        }
    }

    return true;
}

/**
 * @brief Finds the next available metadata record slot for writing.
 * @return Address of the next free record, or
 * 0 when compaction is required.
 */
static uint32_t Metadata_GetNextWriteAddress(void)
{
    BootMetadata_t record;
    uint32_t index;

    for (index = 0U; index < METADATA_RECORD_COUNT; index++)
    {
        (void)Flash_Read(Metadata_GetRecordAddress(index), (uint8_t *)&record, sizeof(record));
        if (Metadata_IsErasedRecord(&record))
        {
            return Metadata_GetRecordAddress(index);
        }
    }

    return 0U;
}

/**
 * @brief Returns read-only metadata for a slot.
 * @param metadata Boot metadata object.
 * @param slot Target
 * slot.
 * @return Pointer to slot metadata, or null for invalid input.
 */
const SlotMetadata_t *Metadata_GetSlot(const BootMetadata_t *metadata, BootSlot_t slot)
{
    if ((metadata == 0) || (slot < BOOT_SLOT_APP1) || (slot > BOOT_SLOT_APP2))
    {
        return 0;
    }

    return &metadata->slots[(uint32_t)slot - 1U];
}

/**
 * @brief Returns mutable metadata for a slot.
 * @param metadata Boot metadata object.
 * @param slot Target slot.

 * * @return Pointer to mutable slot metadata, or null for invalid input.
 */
SlotMetadata_t *Metadata_GetSlotMutable(BootMetadata_t *metadata, BootSlot_t slot)
{
    if ((metadata == 0) || (slot < BOOT_SLOT_APP1) || (slot > BOOT_SLOT_APP2))
    {
        return 0;
    }

    return &metadata->slots[(uint32_t)slot - 1U];
}

/**
 * @brief Initializes an in-memory metadata structure with default values.
 * @param metadata Metadata object to
 * initialize.
 */
void Metadata_InitDefault(BootMetadata_t *metadata)
{
    if (metadata == 0)
    {
        return;
    }

    memset(metadata, 0, sizeof(*metadata));
    metadata->magic = METADATA_MAGIC;
    metadata->format_version = METADATA_FORMAT_VERSION;
    metadata->active_slot = BOOT_SLOT_NONE;
    metadata->previous_slot = BOOT_SLOT_NONE;
    metadata->pending_slot = BOOT_SLOT_NONE;
    metadata->max_boot_attempts = METADATA_MAX_BOOT_ATTEMPTS;
    metadata->slots[0].state = SLOT_STATE_EMPTY;
    metadata->slots[1].state = SLOT_STATE_EMPTY;
    (void)Metadata_UpdateCrc(metadata);
}

/**
 * @brief Checks whether a metadata record has a valid header and CRC.
 * @param metadata Metadata record to
 * validate.
 * @return true if the metadata record is valid, otherwise false.
 */
bool Metadata_IsValid(const BootMetadata_t *metadata)
{
    BootMetadata_t local_copy;

    if (metadata == 0)
    {
        return false;
    }

    if ((metadata->magic != METADATA_MAGIC) || (metadata->format_version != METADATA_FORMAT_VERSION))
    {
        return false;
    }

    memcpy(&local_copy, metadata, sizeof(local_copy));
    if (Metadata_UpdateCrc(&local_copy) != HAL_OK)
    {
        return false;
    }

    return local_copy.metadata_crc == metadata->metadata_crc;
}

/**
 * @brief Loads the newest valid metadata record from flash.
 * @param metadata Output metadata object.
 * @param
 * found_out Output flag indicating whether a valid record was found.
 * @return HAL status of the load operation.
 */
HAL_StatusTypeDef Metadata_LoadLatest(BootMetadata_t *metadata, bool *found_out)
{
    BootMetadata_t candidate;
    BootMetadata_t latest;
    bool found = false;
    uint32_t index;

    if (metadata == 0)
    {
        return HAL_ERROR;
    }

    for (index = 0U; index < METADATA_RECORD_COUNT; index++)
    {
        (void)Flash_Read(Metadata_GetRecordAddress(index), (uint8_t *)&candidate, sizeof(candidate));
        if (!Metadata_IsValid(&candidate))
        {
            continue;
        }

        if ((!found) || (candidate.sequence > latest.sequence))
        {
            latest = candidate;
            found = true;
        }
    }

    if (found_out != 0)
    {
        *found_out = found;
    }

    if (!found)
    {
        return HAL_OK;
    }

    *metadata = latest;
    return HAL_OK;
}

/**
 * @brief Saves a new metadata record instance into flash.
 * @param metadata Metadata object to store.
 * @return
 * HAL status of the save operation.
 */
HAL_StatusTypeDef Metadata_Save(BootMetadata_t *metadata)
{
    HAL_StatusTypeDef status;
    uint32_t write_address;

    if (metadata == 0)
    {
        return HAL_ERROR;
    }

    metadata->sequence++;
    status = Metadata_UpdateCrc(metadata);
    if (status != HAL_OK)
    {
        return status;
    }

    write_address = Metadata_GetNextWriteAddress();
    if (write_address == 0U)
    {
        status = Flash_Erase(METADATA_REGION_START, METADATA_REGION_SIZE);
        if (status != HAL_OK)
        {
            return status;
        }
        write_address = METADATA_REGION_START;
    }

    return Flash_Write(write_address, (const uint8_t *)metadata, sizeof(*metadata));
}

/**
 * @brief Sets or clears the request flag for bootloader OTA mode.
 * @param enable true to request OTA mode, false
 * to clear the request.
 * @return HAL status of the metadata update.
 */
HAL_StatusTypeDef Metadata_RequestOtaMode(bool enable)
{
    BootMetadata_t metadata;
    bool found;
    HAL_StatusTypeDef status;

    status = Metadata_LoadLatest(&metadata, &found);
    if (status != HAL_OK)
    {
        return status;
    }

    if (!found)
    {
        Metadata_InitDefault(&metadata);
    }

    metadata.ota_request = enable ? 1UL : 0UL;
    return Metadata_Save(&metadata);
}

/**
 * @brief Marks a slot as pending after a successful image update.
 * @param slot Updated slot.
 * @param image_size
 * Image size in bytes.
 * @param image_crc CRC of the programmed image.
 * @param version Firmware version stored in
 * metadata.
 * @return HAL status of the metadata update.
 */
HAL_StatusTypeDef Metadata_MarkPending(BootSlot_t slot, uint32_t image_size, uint32_t image_crc, uint32_t version)
{
    BootMetadata_t metadata;
    SlotMetadata_t *slot_metadata;
    bool found;
    HAL_StatusTypeDef status;

    status = Metadata_LoadLatest(&metadata, &found);
    if (status != HAL_OK)
    {
        return status;
    }

    if (!found)
    {
        Metadata_InitDefault(&metadata);
    }

    slot_metadata = Metadata_GetSlotMutable(&metadata, slot);
    if (slot_metadata == 0)
    {
        return HAL_ERROR;
    }

    slot_metadata->state = SLOT_STATE_PENDING;
    slot_metadata->image_size = image_size;
    slot_metadata->image_crc = image_crc;
    slot_metadata->version = version;
    metadata.pending_slot = (uint32_t)slot;
    metadata.boot_attempt_count = 0U;

    return Metadata_Save(&metadata);
}

/**
 * @brief Confirms a slot after the new firmware has booted successfully.
 * @param slot Slot to confirm.
 * @return
 * HAL status of the metadata update.
 */
HAL_StatusTypeDef Metadata_ConfirmSlot(BootSlot_t slot)
{
    BootMetadata_t metadata;
    SlotMetadata_t *slot_metadata;
    BootSlot_t other_slot;
    bool found;
    HAL_StatusTypeDef status;

    status = Metadata_LoadLatest(&metadata, &found);
    if ((status != HAL_OK) || !found)
    {
        return HAL_ERROR;
    }

    slot_metadata = Metadata_GetSlotMutable(&metadata, slot);
    if (slot_metadata == 0)
    {
        return HAL_ERROR;
    }

    other_slot = BootConfig_GetOtherSlot(slot);
    metadata.previous_slot = metadata.active_slot;
    metadata.active_slot = (uint32_t)slot;
    metadata.pending_slot = BOOT_SLOT_NONE;
    metadata.boot_attempt_count = 0U;
    metadata.ota_request = 0U;
    slot_metadata->state = SLOT_STATE_CONFIRMED;

    if (other_slot != BOOT_SLOT_NONE)
    {
        SlotMetadata_t *other_metadata = Metadata_GetSlotMutable(&metadata, other_slot);
        if ((other_metadata != 0) && (other_metadata->state == SLOT_STATE_PENDING))
        {
            other_metadata->state = SLOT_STATE_INVALID;
        }
    }

    return Metadata_Save(&metadata);
}

/**
 * @brief Invalidates a slot so it is no longer considered bootable.
 * @param slot Slot to invalidate.
 * @return
 * HAL status of the metadata update.
 */
HAL_StatusTypeDef Metadata_InvalidateSlot(BootSlot_t slot)
{
    BootMetadata_t metadata;
    SlotMetadata_t *slot_metadata;
    bool found;
    HAL_StatusTypeDef status;

    status = Metadata_LoadLatest(&metadata, &found);
    if ((status != HAL_OK) || !found)
    {
        return HAL_ERROR;
    }

    slot_metadata = Metadata_GetSlotMutable(&metadata, slot);
    if (slot_metadata == 0)
    {
        return HAL_ERROR;
    }

    slot_metadata->state = SLOT_STATE_INVALID;
    if (metadata.pending_slot == (uint32_t)slot)
    {
        metadata.pending_slot = BOOT_SLOT_NONE;
        metadata.boot_attempt_count = 0U;
    }
    if (metadata.active_slot == (uint32_t)slot)
    {
        metadata.active_slot = BOOT_SLOT_NONE;
    }

    return Metadata_Save(&metadata);
}
