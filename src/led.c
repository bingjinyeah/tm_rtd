#include "stm8s.h"
#include "led.h"

void led_init()
{
    GPIO_Init(GPIOD,(GPIO_Pin_TypeDef)GPIO_PIN_2, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(GPIOD,(GPIO_Pin_TypeDef)GPIO_PIN_3, GPIO_MODE_OUT_PP_LOW_FAST);
}

void led_link_on()
{
    GPIO_WriteLow(GPIOD, GPIO_PIN_2);
}

void led_link_off()
{
    GPIO_WriteHigh(GPIOD, GPIO_PIN_2);
}

void led_link_blink()
{
    GPIO_WriteReverse(GPIOD, GPIO_PIN_2);
}

void led_run_on()
{
    GPIO_WriteLow(GPIOD, GPIO_PIN_3);
}

void led_run_off()
{
    GPIO_WriteHigh(GPIOD, GPIO_PIN_3);
}
void led_run_blink()
{
    GPIO_WriteReverse(GPIOD, GPIO_PIN_3);
}
