/**
 * @file Crc32.c
 * @brief Implements buffer and flash CRC32 calculation helpers.
 */
#include "Crc32.h"

#include "crc.h"

/**
 * @brief Accumulates CRC32 over a byte stream using the hardware CRC
 * peripheral.
 * @param data Input buffer.
 *
 * @param length Buffer length in bytes.
 * @param crc_out Output CRC value.
 * @return HAL_OK on success, otherwise
 * HAL_ERROR.
 */
static HAL_StatusTypeDef
Crc32_AccumulateBytes(const uint8_t *data, uint32_t length, uint32_t *crc_out) {
  uint32_t words[8];
  uint32_t chunk_length;
  uint32_t word_count;
  uint32_t word_index;
  uint32_t byte_index;
  uint32_t crc;

  if ((data == 0) || (crc_out == 0)) {
    return HAL_ERROR;
  }

  crc = 0U;
  __HAL_CRC_DR_RESET(&hcrc);

  while (length > 0U) {
    chunk_length = length;
    if (chunk_length > (sizeof(words) * sizeof(uint32_t))) {
      chunk_length = sizeof(words) * sizeof(uint32_t);
    }

    word_count = (chunk_length + 3U) / 4U;
    for (word_index = 0U; word_index < word_count; word_index++) {
      words[word_index] = 0xFFFFFFFFUL;
    }

    for (byte_index = 0U; byte_index < chunk_length; byte_index++) {
      word_index = byte_index / 4U;
      words[word_index] &= ~(0xFFUL << ((byte_index % 4U) * 8U));
      words[word_index] |=
          ((uint32_t)data[byte_index] << ((byte_index % 4U) * 8U));
    }

    crc = HAL_CRC_Accumulate(&hcrc, words, word_count);
    data += chunk_length;
    length -= chunk_length;
  }

  *crc_out = crc;
  return HAL_OK;
}

/**
 * @brief Calculates a CRC32 over a memory buffer.
 * @param data Input buffer.
 * @param length Buffer length in
 * bytes.
 * @param crc_out Output CRC value.
 * @return HAL_OK on success, otherwise HAL_ERROR.
 */
HAL_StatusTypeDef Crc32_CalculateBuffer(const uint8_t *data, uint32_t length,
                                        uint32_t *crc_out) {
  if ((data == 0) || (length == 0U) || (crc_out == 0)) {
    return HAL_ERROR;
  }

  return Crc32_AccumulateBytes(data, length, crc_out);
}

/**
 * @brief Calculates a CRC32 over a flash region.
 * @param address Start address of the flash region.
 * @param
 * length Region length in bytes.
 * @param crc_out Output CRC value.
 * @return HAL_OK on success, otherwise
 * HAL_ERROR.
 */
HAL_StatusTypeDef Crc32_CalculateFlash(uint32_t address, uint32_t length,
                                       uint32_t *crc_out) {
  return Crc32_CalculateBuffer((const uint8_t *)address, length, crc_out);
}
