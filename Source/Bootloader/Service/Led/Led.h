#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    void Led_Init(void);
    void Led_On(void);
    void Led_Off(void);
    void Led_Toggle(void);
    void Led_Blink(uint32_t period_ms);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */
