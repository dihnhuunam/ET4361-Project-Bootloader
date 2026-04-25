/**
 * @file App.h
 * @brief Application service entry point for the bootloader firmware.
 */
#ifndef APP_SERVICE_H
#define APP_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Runs the service-level bootloader startup flow.
 */
void App_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SERVICE_H */
