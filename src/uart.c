#include "stm8s.h"
#include "stm8s_uart1.h"
#include "uart.h"
#include "led.h"
#include "rs485.h"
#include "timer2.h"
#include "para.h"

#define UART_RCV_SIZE		20


uint16_t com_brate = 1;		//0:4.8...1:9.6...2:19.2...3:38.4...4:57.6...5:115.2
uint8_t  com_parity = 0;	//0:no parity,1:even,2:odd
uint8_t  com_stop = 1;
uint8_t  mb_local_address = 8;

extern uint16_t MB_HoldReg[];
extern uint8_t rs485_send_buff[];

uint8_t uart_rcv_buff[UART_RCV_SIZE];
uint8_t uart_rcv_index = 0;
uint8_t rcv_complete = 0;

void delay_ms(uint16_t ms)
{
    uint16_t res = 500;
    while(ms--){
        while(res--){
            
        }
        res = 500;
    }
}

void uart_para_init()
{
	uint32_t bt;
	uint8_t pt,st,wl;
	
    UART1_DeInit();
	switch(com_brate){
		case 0:
			bt = 4800;
			break;
		case 1: 
			bt = 9600;
			break;
		case 2: 
			bt = 19200;
			break;
		case 3:
			bt = 38400;
			break;
		case 4: 
			bt = 57600;
			break;
		case 5: 
			bt = 115200;
			break;
		default: 
			bt = 9600;
			break;
			
	}
	switch(com_parity){
		case 1: 
			pt = UART1_PARITY_EVEN;
			wl = UART1_WORDLENGTH_9D;
			break;
		case 2: 
			pt = UART1_PARITY_ODD;
			wl = UART1_WORDLENGTH_9D;
			break;
		default: 
			pt = UART1_PARITY_NO;
			wl = UART1_WORDLENGTH_8D;
			break;
	}
	switch(com_stop){
		case 1: 
			st = UART1_STOPBITS_1;
			break;
		case 2: 
			st = UART1_STOPBITS_2;
			break;
		default: 
			st = UART1_STOPBITS_1;
			break;
	}
	
	UART1_Init(bt,wl,st,pt,UART1_SYNCMODE_CLOCK_DISABLE,UART1_MODE_TXRX_ENABLE);
}
void para_init()
{
    FLASH_Unlock(FLASH_MEMTYPE_DATA);
    while (FLASH_GetFlagStatus(FLASH_FLAG_DUL) == RESET){}
    if (FLASH_ReadByte(FIRST_BLOOD) != FIRST_BLOOD_DATA){
        
        FLASH_ProgramByte(EEP_ADDR,mb_local_address);
        delay_ms(10);
        FLASH_ProgramByte(EEP_BRATE,com_brate);
        delay_ms(10);
        FLASH_ProgramByte(EEP_PARITY,com_parity);
        delay_ms(10);
        FLASH_ProgramByte(EEP_STOP,com_stop);
        delay_ms(10);
        FLASH_ProgramByte(FIRST_BLOOD,FIRST_BLOOD_DATA);
        delay_ms(10);
        MB_HoldReg[0] = mb_local_address;
        MB_HoldReg[1] = com_brate;
        MB_HoldReg[2] = com_parity;
        MB_HoldReg[3] = com_stop;
        
    }else{
        mb_local_address = FLASH_ReadByte(EEP_ADDR);
        delay_ms(10);
        com_brate = FLASH_ReadByte(EEP_BRATE);
        delay_ms(10);
        com_parity = FLASH_ReadByte(EEP_PARITY);
        delay_ms(10);
        com_stop = FLASH_ReadByte(EEP_STOP);
        delay_ms(10);
        MB_HoldReg[0] = mb_local_address;
        MB_HoldReg[1] = com_brate;
        MB_HoldReg[2] = com_parity;
        MB_HoldReg[3] = com_stop;
        
    }
}

void uart_init()
{
    //GPIO_Init(GPIOD,(GPIO_Pin_TypeDef)GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_FAST);
    //GPIO_Init(GPIOD,(GPIO_Pin_TypeDef)GPIO_PIN_6, GPIO_MODE_OUT_PP_LOW_FAST);
    para_init();
    uart_para_init();
    /* Enable UART1 Receive interrupt*/
    UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);
    /* Enable UART */
    UART1_Cmd(ENABLE);
}

void uart_send(uint8_t* buf,uint8_t size )
{
    uint8_t i;

    for(i=0; i<size; i++)   
    {   
		while(UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET);   		
		UART1->DR = buf[i]; 	
	}  
	while(UART1_GetFlagStatus(UART1_FLAG_TC) == RESET); 
	
}

void uart1_receive()
{
    uint8_t res;  
    
    if(UART1_GetFlagStatus(UART1_FLAG_RXNE)){
        res = UART1->DR;
        if(rcv_complete){
            return;
        }
        if(uart_rcv_index >= UART_RCV_SIZE){
            //uart_rcv_index = 0;
            return;
        }
        timer2_reload(20);
        uart_rcv_buff[uart_rcv_index++] = res;
    }
}


void para_task()
{
    uint8_t changed = 0;
    
    if(MB_HoldReg[0]!=mb_local_address){
        mb_local_address = MB_HoldReg[0];
        FLASH_Unlock(FLASH_MEMTYPE_DATA);   
        FLASH_ProgramByte(EEP_ADDR,mb_local_address);
    }
    if(MB_HoldReg[1]!=com_brate){
        com_brate = MB_HoldReg[1];
        FLASH_Unlock(FLASH_MEMTYPE_DATA);   
        FLASH_ProgramByte(EEP_BRATE,com_brate);
        changed = 1;
    }
    if(MB_HoldReg[2] != com_parity){
        com_parity = MB_HoldReg[2];
        FLASH_Unlock(FLASH_MEMTYPE_DATA);   
        FLASH_ProgramByte(EEP_PARITY,com_parity);
        changed = 1;
    }
    if(MB_HoldReg[3] != com_stop){
        com_stop = MB_HoldReg[3];
        FLASH_Unlock(FLASH_MEMTYPE_DATA);   
        FLASH_ProgramByte(EEP_STOP,com_stop);
        changed = 1;
    }
    if(changed){
        uart_para_init();
        changed = 0;
    }
}

