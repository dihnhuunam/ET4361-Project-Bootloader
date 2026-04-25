#include "Led.h"

#include "main.h"

#define LED_PORT GPIOC
#define LED_PIN GPIO_PIN_13

static uint32_t s_led_tick = 0U;

void Led_Init(void)
{
    Led_Off();
    s_led_tick = HAL_GetTick();
}

void Led_On(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

void Led_Off(void)
{
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

void Led_Toggle(void)
{
    HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
}

void Led_Blink(uint32_t period_ms)
{
    uint32_t now = HAL_GetTick();

    if ((now - s_led_tick) >= period_ms)
    {
        s_led_tick = now;
        Led_Toggle();
    }
}
