/**
 * @file Image.h
 * @brief Image validation helpers for application slots.
 */
#ifndef IMAGE_SERVICE_H
#define IMAGE_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "BootloaderConfig.h"
#include "stm32f1xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

    /** @brief Reports the validation result of an application image. */
    typedef struct
    {
        bool is_valid;
        bool metadata_valid;
        bool vector_valid;
        bool crc_valid;
        uint32_t stack_pointer;
        uint32_t reset_handler;
        uint32_t calculated_crc;
    } ImageValidationResult_t;

    /**
     * @brief Validates the vector table of an image inside the selected slot.
     * @param slot Target application slot.
     * @param base_address Start address of the image.
     * @param image_size Image size in bytes.
     * @param stack_pointer_out Optional output for the initial stack pointer.
     * @param reset_handler_out Optional output for the reset handler address.
     * @return true if the vector table is valid, otherwise false.
     */
    bool Image_IsVectorTableValid(BootSlot_t slot, uint32_t base_address, uint32_t image_size,
                                  uint32_t *stack_pointer_out, uint32_t *reset_handler_out);

    /**
     * @brief Computes the CRC of an image stored in a slot.
     * @param slot Target application slot.
     * @param image_size Image size in bytes.
     * @param crc_out Output CRC value.
     * @return HAL status of the CRC operation.
     */
    HAL_StatusTypeDef Image_ComputeCrc(BootSlot_t slot, uint32_t image_size, uint32_t *crc_out);

    /**
     * @brief Validates metadata, vector table, and CRC for an image.
     * @param slot Target application slot.
     * @param image_size Image size in bytes.
     * @param expected_crc Expected image CRC from metadata.
     * @param result_out Optional detailed validation result.
     * @return true if the image is valid, otherwise false.
     */
    bool Image_Validate(BootSlot_t slot, uint32_t image_size, uint32_t expected_crc,
                        ImageValidationResult_t *result_out);

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_SERVICE_H */
