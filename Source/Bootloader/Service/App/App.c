#include "App.h"

#include "Bootloader.h"
#include "Debug.h"
#include "Led.h"

void App_Run(void)
{
    Led_Init();
    Led_On();
    Debug("Bootloader start\r\n");
    Bootloader_Run();
}
