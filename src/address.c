#include "stm8s.h"

uint8_t mb_local_address = 250;

void spi_init(void)
{
    SPI_DeInit();
    GPIO_Init(GPIOC,(GPIO_Pin_TypeDef)GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(GPIOC,(GPIO_Pin_TypeDef)GPIO_PIN_6, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(GPIOC,(GPIO_Pin_TypeDef)GPIO_PIN_7, GPIO_MODE_IN_PU_NO_IT);
    
    SPI_Init(SPI_FIRSTBIT_MSB, SPI_BAUDRATEPRESCALER_256, SPI_MODE_MASTER, SPI_CLOCKPOLARITY_HIGH,
        SPI_CLOCKPHASE_2EDGE,SPI_DATADIRECTION_1LINE_RX, SPI_NSS_SOFT,0x00);
    SPI_Cmd(ENABLE); 
}

uint8_t spi_read_byte(void)
{
    
    /* Loop while DR register in not emplty */
    while(SPI_GetFlagStatus(SPI_FLAG_TXE) == RESET);
    
    /* Send byte through the SPI1 peripheral */
    SPI_SendData(0xaa);
    
    /* Wait to receive a byte */
    while(SPI_GetFlagStatus(SPI_FLAG_RXNE) == RESET);
    
    /* Return the byte read from the SPI bus */
    return SPI_ReceiveData();
}

void delayms(uint16_t ms)
{
    uint16_t res = 500;
    while(ms--){
        while(res--){
            
        }
        res = 500;
    }
}

void get_local_address(void)
{
    GPIO_WriteLow(GPIOC,GPIO_PIN_6);
    delayms(10);
    GPIO_WriteHigh(GPIOC,GPIO_PIN_6);
    delayms(10);
    mb_local_address = spi_read_byte();
}

void local_address_init()
{
    spi_init();
    get_local_address();
}
