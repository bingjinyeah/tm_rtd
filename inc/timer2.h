#ifndef _TIMER2_H
#define _TIMER2_H

#include "stm8s.h"

void timer2_init(void);
void timer2_reload(uint16_t ms);
void tiemr2_update(void);

#endif