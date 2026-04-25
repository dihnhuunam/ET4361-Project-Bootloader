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

    bool Image_IsVectorTableValid(BootSlot_t slot, uint32_t base_address, uint32_t image_size,
                                  uint32_t *stack_pointer_out, uint32_t *reset_handler_out);
    HAL_StatusTypeDef Image_ComputeCrc(BootSlot_t slot, uint32_t image_size, uint32_t *crc_out);
    bool Image_Validate(BootSlot_t slot, uint32_t image_size, uint32_t expected_crc,
                        ImageValidationResult_t *result_out);

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_SERVICE_H */
