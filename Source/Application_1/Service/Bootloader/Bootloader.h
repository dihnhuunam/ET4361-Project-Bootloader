/**
 * @file Bootloader.h
 * @brief Public entry point for the bootloader service.
 */
#ifndef BOOTLOADER_SERVICE_H
#define BOOTLOADER_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Executes the bootloader decision and handoff flow.
 */
void Bootloader_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_SERVICE_H */
