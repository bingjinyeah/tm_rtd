#include "stm8s.h"
#include "led.h"
#include "modbus_driver.h"


extern uint8_t uart_rcv_buff[];
extern uint8_t uart_rcv_index;
extern uint8_t rcv_complete;

void timer2_init()
{
    TIM2_TimeBaseInit(TIM2_PRESCALER_128, 65535);
    /* Clear TIM2 update flag */
    TIM2_ClearFlag(TIM2_FLAG_UPDATE);
    /* Enable update interrupt */
    TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
    
}

void timer2_reload(uint16_t ms)
{
    TIM2_Cmd(DISABLE);
    TIM2_SetCounter(0);
    //timer base = 8000000/128, is 0.016ms;
    TIM2_SetAutoreload(1000*ms/16);
    TIM2_Cmd(ENABLE);
}

void timer2_update()
{
    
    TIM2_Cmd(DISABLE);
    TIM2_ClearFlag(TIM2_FLAG_UPDATE);
    TIM2_SetCounter(0);
    //led_link_blink();
	//modbus_pdu_process(uart_rcv_buff,uart_rcv_index);
    rcv_complete = 1;    
}
