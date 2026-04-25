#include "Metadata.h"

#include "BootloaderConfig.h"
#include "Crc32.h"
#include "Flash.h"

#include <string.h>

#define METADATA_RECORD_ALIGNMENT 128U
#define METADATA_RECORD_COUNT (METADATA_REGION_SIZE / METADATA_RECORD_ALIGNMENT)

static uint32_t Metadata_GetRecordAddress(uint32_t index)
{
    return METADATA_REGION_START + (index * METADATA_RECORD_ALIGNMENT);
}

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

const SlotMetadata_t *Metadata_GetSlot(const BootMetadata_t *metadata, BootSlot_t slot)
{
    if ((metadata == 0) || (slot < BOOT_SLOT_APP1) || (slot > BOOT_SLOT_APP2))
    {
        return 0;
    }

    return &metadata->slots[(uint32_t)slot - 1U];
}

SlotMetadata_t *Metadata_GetSlotMutable(BootMetadata_t *metadata, BootSlot_t slot)
{
    if ((metadata == 0) || (slot < BOOT_SLOT_APP1) || (slot > BOOT_SLOT_APP2))
    {
        return 0;
    }

    return &metadata->slots[(uint32_t)slot - 1U];
}

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
