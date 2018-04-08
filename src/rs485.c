#include "stm8s.h"
#include "uart.h"
#include "modbus_driver.h"
#include "crc.h"
#include "rs485.h"

MB_FRAME mb_rcv_frame;
extern uint8_t  mb_local_address;
extern uint8_t uart_rcv_buff[];
extern uint8_t uart_rcv_index;
extern uint8_t rcv_complete;

void rs485_tx_enable()
{
	GPIO_WriteHigh(GPIOD, GPIO_PIN_3);
    GPIO_WriteHigh(GPIOD, GPIO_PIN_4);
}

void rs485_rx_enable()
{
	GPIO_WriteLow(GPIOD, GPIO_PIN_3);
    GPIO_WriteLow(GPIOD, GPIO_PIN_4);
}

void rs485_init()
{
	GPIO_Init(GPIOD,(GPIO_Pin_TypeDef)GPIO_PIN_3, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(GPIOD,(GPIO_Pin_TypeDef)GPIO_PIN_4, GPIO_MODE_OUT_PP_LOW_FAST);
	rs485_rx_enable();
	uart_init();
}

void rs485_send(uint8_t* buf,uint16_t size )
{
	uint8_t i;
    
    if(buf==((void*)0))
		return;
	//for(i=0;i<size;i++){
    //    rs485_send_buff[i] = buf[i];
    //}
	rs485_tx_enable();
	uart_send(buf,size);
    rs485_rx_enable();
}

void modbus_pdu_process(uint8_t *pdata, uint8_t length)
{
	uint16_t  buffer_length;
	uint16_t  crc_result;
    uint8_t   index;		     
	uint8_t   i;
    
	if ((length <= 3) || (length > MB_BUFFER_SIZE / 2)) {
		return;
	}
	buffer_length = length;
	index = 0;
	//mb_rcv_frame.ComPort = UART_COM4;
	mb_rcv_frame.Address = pdata[index++];
	mb_rcv_frame.Function = pdata[index++];
	mb_rcv_frame.Length = buffer_length - 4;
			
	for (i = 0; i < mb_rcv_frame.Length; i++)
	{
		mb_rcv_frame.Data[i] = pdata[index++];
	}  
	*((uint8_t*)&mb_rcv_frame.CRCResult) = pdata[index++];
	*((uint8_t*)&mb_rcv_frame.CRCResult+1) = pdata[index++];
	crc_result = CRC16(pdata, buffer_length - 2);
	index = 0;
	if (crc_result != mb_rcv_frame.CRCResult){
		return;
	}
	if ((mb_rcv_frame.Address != mb_local_address) 
		&&(mb_rcv_frame.Address != MB_BROADCAST_ADDRESS)) {
		return;
	}
    
	switch(mb_rcv_frame.Function)
	{	
		/*case 1: 
            MB_DRIVER_ReadCoils(&mb_rcv_frame); 
            break;
        case 2: 
            MB_DRIVER_ReadDisIn(&mb_rcv_frame); 
            break;*/
		case 3: 
			MB_DRIVER_ReadHoldReg(&mb_rcv_frame);
			break;	
		case 4: 
			MB_DRIVER_ReadInReg(&mb_rcv_frame);
			break;
        /*case 5: 
			MB_DRIVER_WriteSingleCoil(&mb_rcv_frame);
			break;*/
		case 6: 
			MB_DRIVER_WriteSingleHoldReg(&mb_rcv_frame); 
			break;
        /*case 15:
            MB_DRIVER_WriteMoreCoils(&mb_rcv_frame); 
            break;*/
			//case 16: MB_DRIVER_WriteMoreReg(&mb_rcv_frame); break;     //16:??????
		default:
			break;
	}
}

void modbus_task()
{
    if(rcv_complete){
        
        modbus_pdu_process(uart_rcv_buff,uart_rcv_index);
        rcv_complete = 0;
        uart_rcv_index = 0;
    }
}
