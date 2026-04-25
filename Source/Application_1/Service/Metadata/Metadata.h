/**
 * @file Metadata.h
 * @brief Metadata storage and state transition helpers for boot control.
 */
#ifndef METADATA_SERVICE_H
#define METADATA_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "BootloaderConfig.h"

#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

/** @brief Per-slot metadata stored inside the boot metadata record. */
typedef struct {
  uint32_t state;
  uint32_t image_size;
  uint32_t image_crc;
  uint32_t version;
} SlotMetadata_t;

/** @brief Persistent boot metadata record stored in flash. */
typedef struct {
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

/**
 * @brief Initializes an in-memory metadata structure with default values.
 * @param metadata Metadata object to initialize.
 */
void Metadata_InitDefault(BootMetadata_t *metadata);

/**
 * @brief Checks whether a metadata record has a valid header and CRC.
 * @param metadata Metadata record to validate.
 * @return true if the metadata record is valid, otherwise false.
 */
bool Metadata_IsValid(const BootMetadata_t *metadata);

/**
 * @brief Loads the newest valid metadata record from flash.
 * @param metadata Output metadata object.
 * @param found_out Output flag indicating whether a valid record was found.
 * @return HAL status of the load operation.
 */
HAL_StatusTypeDef Metadata_LoadLatest(BootMetadata_t *metadata,
                                      bool *found_out);

/**
 * @brief Saves a new metadata record instance into flash.
 * @param metadata Metadata object to store.
 * @return HAL status of the save operation.
 */
HAL_StatusTypeDef Metadata_Save(BootMetadata_t *metadata);

/**
 * @brief Sets or clears the request flag for bootloader OTA mode.
 * @param enable true to request OTA mode, false to clear the request.
 * @return HAL status of the metadata update.
 */
HAL_StatusTypeDef Metadata_RequestOtaMode(bool enable);

/**
 * @brief Marks a slot as pending after a successful image update.
 * @param slot Updated slot.
 * @param image_size Image size in bytes.
 * @param image_crc CRC of the programmed image.
 * @param version Firmware version stored in metadata.
 * @return HAL status of the metadata update.
 */
HAL_StatusTypeDef Metadata_MarkPending(BootSlot_t slot, uint32_t image_size,
                                       uint32_t image_crc, uint32_t version);

/**
 * @brief Confirms a slot after the new firmware has booted successfully.
 * @param slot Slot to confirm.
 * @return HAL status of the metadata update.
 */
HAL_StatusTypeDef Metadata_ConfirmSlot(BootSlot_t slot);

/**
 * @brief Invalidates a slot so it is no longer considered bootable.
 * @param slot Slot to invalidate.
 * @return HAL status of the metadata update.
 */
HAL_StatusTypeDef Metadata_InvalidateSlot(BootSlot_t slot);

/**
 * @brief Returns read-only metadata for a slot.
 * @param metadata Boot metadata object.
 * @param slot Target slot.
 * @return Pointer to slot metadata, or null for invalid input.
 */
const SlotMetadata_t *Metadata_GetSlot(const BootMetadata_t *metadata,
                                       BootSlot_t slot);

/**
 * @brief Returns mutable metadata for a slot.
 * @param metadata Boot metadata object.
 * @param slot Target slot.
 * @return Pointer to mutable slot metadata, or null for invalid input.
 */
SlotMetadata_t *Metadata_GetSlotMutable(BootMetadata_t *metadata,
                                        BootSlot_t slot);

#ifdef __cplusplus
}
#endif

#endif /* METADATA_SERVICE_H */
