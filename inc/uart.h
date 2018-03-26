#ifndef	_UART_H
#define	_UART_H

#include "stm8s.h"

extern void uart_send(uint8_t* buf, uint8_t size );
extern void uart1_receive(void);
extern void uart_init(void);
extern void uart_para_init(void);
extern void uart_pdu_process(void);

#endif
