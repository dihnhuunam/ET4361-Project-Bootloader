/**
 * @file Bootloader.c
 * @brief Implements slot validation, recovery mode, and application handoff.
 */
#include "Bootloader.h"

#include "BootloaderConfig.h"
#include "Debug.h"
#include "Flash.h"
#include "Image.h"
#include "Led.h"
#include "Metadata.h"
#include "main.h"
#include "usart.h"

#include <string.h>

typedef void (*BootEntry_t)(void);

#define BOOTLOADER_RECOVERY_MAGIC "BLFW"
#define BOOTLOADER_RECOVERY_CHUNK_SIZE 256U

/* Simple UART recovery header sent before the raw firmware image. */
typedef struct
{
    uint8_t magic[4];
    uint32_t image_size;
    uint32_t image_crc;
    uint32_t version;
} BootloaderRecoveryHeader_t;

static HAL_StatusTypeDef Bootloader_ReceiveExact(uint8_t *buffer, uint32_t length)
{
    if ((buffer == 0) || (length == 0U))
    {
        return HAL_ERROR;
    }

    return HAL_UART_Receive(&huart1, buffer, (uint16_t)length, HAL_MAX_DELAY);
}

static BootSlot_t Bootloader_SelectRecoverySlot(const BootMetadata_t *metadata)
{
    const SlotMetadata_t *app1;
    const SlotMetadata_t *app2;

    if (metadata == 0)
    {
        return BOOT_SLOT_APP1;
    }

    if ((metadata->active_slot == BOOT_SLOT_APP1) || (metadata->active_slot == BOOT_SLOT_APP2))
    {
        /* Keep the currently active firmware untouched and recover into the other slot. */
        return BootConfig_GetOtherSlot((BootSlot_t)metadata->active_slot);
    }

    app1 = Metadata_GetSlot(metadata, BOOT_SLOT_APP1);
    app2 = Metadata_GetSlot(metadata, BOOT_SLOT_APP2);

    if ((app1 != 0) && (app1->state == SLOT_STATE_EMPTY))
    {
        return BOOT_SLOT_APP1;
    }

    if ((app2 != 0) && (app2->state == SLOT_STATE_EMPTY))
    {
        return BOOT_SLOT_APP2;
    }

    return BOOT_SLOT_APP1;
}

static void Bootloader_RunRecoveryMode(BootMetadata_t *metadata)
{
    BootloaderRecoveryHeader_t header;
    const BootSlotRegion_t *region;
    BootSlot_t slot;
    uint8_t chunk[BOOTLOADER_RECOVERY_CHUNK_SIZE];
    uint32_t remaining;
    uint32_t write_address;
    uint32_t expected_crc;
    uint32_t calculated_crc;
    HAL_StatusTypeDef status;

    slot = Bootloader_SelectRecoverySlot(metadata);
    region = BootConfig_GetSlotRegion(slot);
    if (region == 0)
    {
        Debug("Recovery error: invalid slot\r\n");
        return;
    }

    Debug("Recovery mode on UART1\r\n");
    Debug("Send header: magic='BLFW', size, crc, version; then raw image\r\n");
    Debug("Target slot=%lu addr=0x%08lX size=%lu\r\n", (unsigned long)slot, (unsigned long)region->start_address,
          (unsigned long)region->size);

    while (1)
    {
        /* Wait for a complete recovery header before touching flash. */
        status = Bootloader_ReceiveExact((uint8_t *)&header, sizeof(header));
        if (status != HAL_OK)
        {
            Debug("Recovery rx header failed\r\n");
            continue;
        }

        if (memcmp(header.magic, BOOTLOADER_RECOVERY_MAGIC, sizeof(header.magic)) != 0)
        {
            Debug("Recovery invalid magic\r\n");
            continue;
        }

        if ((header.image_size == 0U) || (header.image_size > region->size))
        {
            Debug("Recovery invalid size=%lu\r\n", (unsigned long)header.image_size);
            continue;
        }

        expected_crc = header.image_crc;
        Debug("Recovery erase slot=%lu image_size=%lu crc=0x%08lX version=%lu\r\n", (unsigned long)slot,
              (unsigned long)header.image_size, (unsigned long)expected_crc, (unsigned long)header.version);

        /* Erase the full target slot so the new image is written into a clean region. */
        status = Flash_Erase(region->start_address, region->size);
        if (status != HAL_OK)
        {
            Debug("Recovery erase failed\r\n");
            continue;
        }

        remaining = header.image_size;
        write_address = region->start_address;
        while (remaining > 0U)
        {
            uint32_t chunk_size =
                (remaining > BOOTLOADER_RECOVERY_CHUNK_SIZE) ? BOOTLOADER_RECOVERY_CHUNK_SIZE : remaining;

            /* Receive one chunk from UART and program it directly into flash. */
            status = Bootloader_ReceiveExact(chunk, chunk_size);
            if (status != HAL_OK)
            {
                Debug("Recovery rx chunk failed\r\n");
                break;
            }

            status = Flash_Write(write_address, chunk, chunk_size);
            if (status != HAL_OK)
            {
                Debug("Recovery flash write failed at 0x%08lX\r\n", (unsigned long)write_address);
                break;
            }

            write_address += chunk_size;
            remaining -= chunk_size;
        }

        if (remaining != 0U)
        {
            Debug("Recovery aborted\r\n");
            continue;
        }

        /* Validate the programmed image before updating metadata. */
        status = Image_ComputeCrc(slot, header.image_size, &calculated_crc);
        if (status != HAL_OK)
        {
            Debug("Recovery crc compute failed\r\n");
            continue;
        }

        if (calculated_crc != expected_crc)
        {
            Debug("Recovery crc mismatch calc=0x%08lX expected=0x%08lX\r\n", (unsigned long)calculated_crc,
                  (unsigned long)expected_crc);
            continue;
        }

        /* Mark the new slot as pending so the normal boot flow can trial-boot it. */
        status = Metadata_MarkPending(slot, header.image_size, expected_crc, header.version);
        if (status != HAL_OK)
        {
            Debug("Recovery metadata update failed\r\n");
            continue;
        }

        Debug("Recovery success, marked pending. Resetting...\r\n");
        HAL_Delay(100U);
        NVIC_SystemReset();
    }
}

/**
 * @brief Enters safe mode and switches into UART recovery handling.
 * @param reason Human-readable reason reported through the debug UART.
 * @return This function does not return under normal operation.
 */
static void Bootloader_EnterSafeMode(const char *reason)
{
    Debug("Bootloader safe mode: %s\r\n", reason);
    BootMetadata_t metadata;
    bool found_metadata = false;
    HAL_StatusTypeDef status;

    status = Metadata_LoadLatest(&metadata, &found_metadata);
    if ((status != HAL_OK) || !found_metadata)
    {
        Metadata_InitDefault(&metadata);
    }

    /* In safe mode, stay in UART recovery until a valid firmware image is received. */
    Bootloader_RunRecoveryMode(&metadata);

    while (1)
    {
        Led_Blink(250U);
    }
}

/**
 * @brief Selects a bootable slot only when the slot state matches the expected values.
 * @param metadata Current boot metadata.
 * @param slot Candidate slot to validate.
 * @param state_a First accepted slot state.
 * @param state_b Second accepted slot state.
 * @param result_out Optional output validation details.
 * @return true if the slot state and image are valid, otherwise false.
 */
static bool Bootloader_IsSlotBootableWithStates(const BootMetadata_t *metadata, BootSlot_t slot, uint32_t state_a,
                                                uint32_t state_b, ImageValidationResult_t *result_out)
{
    const SlotMetadata_t *slot_metadata;
    uint32_t state;

    slot_metadata = Metadata_GetSlot(metadata, slot);
    if (slot_metadata == 0)
    {
        return false;
    }

    state = slot_metadata->state;
    if ((state != state_a) && (state != state_b))
    {
        return false;
    }

    return Image_Validate(slot, slot_metadata->image_size, slot_metadata->image_crc, result_out);
}

/**
 * @brief Chooses the best confirmed fallback slot for normal boot.
 * @param metadata Current boot metadata.
 * @return Selected confirmed slot, or BOOT_SLOT_NONE if no slot is bootable.
 */
static BootSlot_t Bootloader_SelectConfirmedSlot(const BootMetadata_t *metadata)
{
    ImageValidationResult_t validation;
    BootSlot_t candidates[3];
    uint32_t count = 0U;
    uint32_t index;

    if ((metadata->active_slot == BOOT_SLOT_APP1) || (metadata->active_slot == BOOT_SLOT_APP2))
    {
        candidates[count++] = (BootSlot_t)metadata->active_slot;
    }

    if (((metadata->previous_slot == BOOT_SLOT_APP1) || (metadata->previous_slot == BOOT_SLOT_APP2)) &&
        (metadata->previous_slot != metadata->active_slot))
    {
        candidates[count++] = (BootSlot_t)metadata->previous_slot;
    }

    if (count == 0U)
    {
        candidates[count++] = BOOT_SLOT_APP1;
        candidates[count++] = BOOT_SLOT_APP2;
    }
    else
    {
        BootSlot_t other = BootConfig_GetOtherSlot(candidates[0]);
        if ((other != BOOT_SLOT_NONE) && (other != metadata->previous_slot))
        {
            candidates[count++] = other;
        }
    }

    for (index = 0U; index < count; index++)
    {
        if (Bootloader_IsSlotBootableWithStates(metadata, candidates[index], SLOT_STATE_CONFIRMED, SLOT_STATE_VALID,
                                                &validation))
        {
            return candidates[index];
        }
    }

    return BOOT_SLOT_NONE;
}

/**
 * @brief Transfers execution to the selected application slot.
 * @param slot Application slot to boot.
 * @return This function does not return under normal operation.
 */
static void Bootloader_JumpToSlot(BootSlot_t slot)
{
    const BootSlotRegion_t *region;
    uint32_t app_stack_pointer;
    uint32_t app_reset_handler;
    BootEntry_t entry;

    region = BootConfig_GetSlotRegion(slot);
    if (region == 0)
    {
        Bootloader_EnterSafeMode("slot region invalid");
    }

    app_stack_pointer = *(const uint32_t *)(region->start_address);
    app_reset_handler = *(const uint32_t *)(region->start_address + 4U);
    entry = (BootEntry_t)app_reset_handler;

    Debug("Bootloader jump slot=%lu addr=0x%08lX\r\n", (unsigned long)slot, (unsigned long)region->start_address);

    __disable_irq();
    HAL_RCC_DeInit();
    HAL_DeInit();
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;
    SCB->VTOR = region->start_address;
    __set_MSP(app_stack_pointer);
    entry();

    Bootloader_EnterSafeMode("application returned");
}

/**
 * @brief Processes a pending image before normal confirmed-slot selection.
 * @param metadata Current boot metadata.
 * @param slot_out Output selected pending slot.
 * @return true if a pending slot should be booted, otherwise false.
 */
static bool Bootloader_HandlePending(BootMetadata_t *metadata, BootSlot_t *slot_out)
{
    BootSlot_t pending_slot;
    SlotMetadata_t *pending_metadata;
    ImageValidationResult_t validation;

    pending_slot = (BootSlot_t)metadata->pending_slot;
    if ((pending_slot != BOOT_SLOT_APP1) && (pending_slot != BOOT_SLOT_APP2))
    {
        return false;
    }

    pending_metadata = Metadata_GetSlotMutable(metadata, pending_slot);
    if (pending_metadata == 0)
    {
        return false;
    }

    if (metadata->boot_attempt_count >= metadata->max_boot_attempts)
    {
        Debug("Pending slot exceeded max attempts, rollback\r\n");
        pending_metadata->state = SLOT_STATE_INVALID;
        metadata->pending_slot = BOOT_SLOT_NONE;
        metadata->boot_attempt_count = 0U;
        (void)Metadata_Save(metadata);
        return false;
    }

    if (!Bootloader_IsSlotBootableWithStates(metadata, pending_slot, SLOT_STATE_PENDING, SLOT_STATE_PENDING,
                                             &validation))
    {
        Debug("Pending slot invalid, rollback\r\n");
        pending_metadata->state = SLOT_STATE_INVALID;
        metadata->pending_slot = BOOT_SLOT_NONE;
        metadata->boot_attempt_count = 0U;
        (void)Metadata_Save(metadata);
        return false;
    }

    metadata->boot_attempt_count++;
    pending_metadata->state = SLOT_STATE_PENDING;
    (void)Metadata_Save(metadata);
    *slot_out = pending_slot;
    return true;
}

/**
 * @brief Runs the top-level bootloader flow after hardware initialization.
 */
void Bootloader_Run(void)
{
    BootMetadata_t metadata;
    BootSlot_t selected_slot = BOOT_SLOT_NONE;
    bool found_metadata;
    HAL_StatusTypeDef status;

    Debug("Bootloader run\r\n");

    status = Metadata_LoadLatest(&metadata, &found_metadata);
    if (status != HAL_OK)
    {
        Bootloader_EnterSafeMode("metadata read failed");
    }

    if (!found_metadata)
    {
        Debug("Metadata not found, initialize defaults\r\n");
        Metadata_InitDefault(&metadata);
        status = Metadata_Save(&metadata);
        if (status != HAL_OK)
        {
            Bootloader_EnterSafeMode("metadata init failed");
        }
    }

    if (metadata.ota_request != 0U)
    {
        Bootloader_EnterSafeMode("ota mode requested");
    }

    if (!Bootloader_HandlePending(&metadata, &selected_slot))
    {
        selected_slot = Bootloader_SelectConfirmedSlot(&metadata);
    }

    if (selected_slot == BOOT_SLOT_NONE)
    {
        Bootloader_EnterSafeMode("no valid application");
    }

    Bootloader_JumpToSlot(selected_slot);
}
