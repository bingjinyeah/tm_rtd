#ifndef _LED_H
#define _LED_H

#include "stm8s.h"

void led_init(void);
void led_link_on(void);
void led_link_off(void);
void led_link_blink(void);
void led_run_on(void);
void led_run_off(void);
void led_run_blink(void);


#endif