#include "Image.h"

#include "BootloaderConfig.h"
#include "Crc32.h"

static void Image_ResetValidationResult(ImageValidationResult_t *result)
{
    if (result == 0)
    {
        return;
    }

    result->is_valid = false;
    result->metadata_valid = false;
    result->vector_valid = false;
    result->crc_valid = false;
    result->stack_pointer = 0U;
    result->reset_handler = 0U;
    result->calculated_crc = 0U;
}

bool Image_IsVectorTableValid(BootSlot_t slot, uint32_t base_address, uint32_t image_size, uint32_t *stack_pointer_out,
                              uint32_t *reset_handler_out)
{
    uint32_t stack_pointer;
    uint32_t reset_handler;
    uint32_t reset_handler_address;

    if (!BootConfig_IsAppRangeValid(slot, base_address, image_size) || (image_size < 8U))
    {
        return false;
    }

    stack_pointer = *(const uint32_t *)base_address;
    reset_handler = *(const uint32_t *)(base_address + 4U);
    reset_handler_address = reset_handler & ~1UL;

    if (!BootConfig_IsRamAddressValid(stack_pointer))
    {
        return false;
    }

    if (!BootConfig_IsAppRangeValid(slot, reset_handler_address, 4U))
    {
        return false;
    }

    if (stack_pointer_out != 0)
    {
        *stack_pointer_out = stack_pointer;
    }

    if (reset_handler_out != 0)
    {
        *reset_handler_out = reset_handler;
    }

    return true;
}

HAL_StatusTypeDef Image_ComputeCrc(BootSlot_t slot, uint32_t image_size, uint32_t *crc_out)
{
    const BootSlotRegion_t *region;

    region = BootConfig_GetSlotRegion(slot);
    if ((region == 0) || (crc_out == 0) || (image_size == 0U) || (image_size > region->size))
    {
        return HAL_ERROR;
    }

    return Crc32_CalculateFlash(region->start_address, image_size, crc_out);
}

bool Image_Validate(BootSlot_t slot, uint32_t image_size, uint32_t expected_crc, ImageValidationResult_t *result_out)
{
    const BootSlotRegion_t *region;
    HAL_StatusTypeDef status;
    uint32_t calculated_crc;

    Image_ResetValidationResult(result_out);

    region = BootConfig_GetSlotRegion(slot);
    if ((region == 0) || (image_size == 0U) || (image_size > region->size))
    {
        return false;
    }

    if (result_out != 0)
    {
        result_out->metadata_valid = true;
    }

    if (!Image_IsVectorTableValid(slot, region->start_address, image_size,
                                  result_out != 0 ? &result_out->stack_pointer : 0,
                                  result_out != 0 ? &result_out->reset_handler : 0))
    {
        return false;
    }

    if (result_out != 0)
    {
        result_out->vector_valid = true;
    }

    status = Image_ComputeCrc(slot, image_size, &calculated_crc);
    if (status != HAL_OK)
    {
        return false;
    }

    if (result_out != 0)
    {
        result_out->calculated_crc = calculated_crc;
    }

    if (calculated_crc != expected_crc)
    {
        return false;
    }

    if (result_out != 0)
    {
        result_out->crc_valid = true;
        result_out->is_valid = true;
    }

    return true;
}
