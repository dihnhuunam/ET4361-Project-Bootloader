/**
 * @file App.c
 * @brief Implements the application service startup flow.
 */
#include "App.h"

#include "Bootloader.h"
#include "Debug.h"
#include "Led.h"

/**
 * @brief Initializes application services and starts the bootloader.
 */
void App_Run(void)
{
    Led_Init();
    Led_On();
    Debug("Bootloader start\r\n");
    Bootloader_Run();
}
