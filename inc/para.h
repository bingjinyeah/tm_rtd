#ifndef _PARA_H
#define _PARA_H

#include "stm8s.h"
#include "stm8s_flash.h"

#define FIRST_BLOOD         FLASH_DATA_START_PHYSICAL_ADDRESS+1
#define EEP_BRATE   	    FLASH_DATA_START_PHYSICAL_ADDRESS+2
#define EEP_PARITY   	    FLASH_DATA_START_PHYSICAL_ADDRESS+3
#define EEP_STOP       	    FLASH_DATA_START_PHYSICAL_ADDRESS+4
#define EEP_ADDR            FLASH_DATA_START_PHYSICAL_ADDRESS+5

#define FIRST_BLOOD_DATA    0x5a

void para_task(void);
void para_init(void);

#endif