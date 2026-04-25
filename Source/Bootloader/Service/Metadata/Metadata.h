#ifndef METADATA_SERVICE_H
#define METADATA_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "BootloaderConfig.h"

#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

    typedef struct
    {
        uint32_t state;
        uint32_t image_size;
        uint32_t image_crc;
        uint32_t version;
    } SlotMetadata_t;

    typedef struct
    {
        uint32_t magic;
        uint32_t format_version;
        uint32_t sequence;
        uint32_t active_slot;
        uint32_t previous_slot;
        uint32_t pending_slot;
        uint32_t boot_attempt_count;
        uint32_t max_boot_attempts;
        uint32_t ota_request;
        uint32_t reserved0;
        SlotMetadata_t slots[2];
        uint32_t reserved1[6];
        uint32_t metadata_crc;
    } BootMetadata_t;

    void Metadata_InitDefault(BootMetadata_t *metadata);
    bool Metadata_IsValid(const BootMetadata_t *metadata);
    HAL_StatusTypeDef Metadata_LoadLatest(BootMetadata_t *metadata, bool *found_out);
    HAL_StatusTypeDef Metadata_Save(BootMetadata_t *metadata);
    HAL_StatusTypeDef Metadata_RequestOtaMode(bool enable);
    HAL_StatusTypeDef Metadata_MarkPending(BootSlot_t slot, uint32_t image_size, uint32_t image_crc, uint32_t version);
    HAL_StatusTypeDef Metadata_ConfirmSlot(BootSlot_t slot);
    HAL_StatusTypeDef Metadata_InvalidateSlot(BootSlot_t slot);
    const SlotMetadata_t *Metadata_GetSlot(const BootMetadata_t *metadata, BootSlot_t slot);
    SlotMetadata_t *Metadata_GetSlotMutable(BootMetadata_t *metadata, BootSlot_t slot);

#ifdef __cplusplus
}
#endif

#endif /* METADATA_SERVICE_H */
