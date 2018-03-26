#include "stm8s.h"
#include "stm8s_adc1.h"
#include "adc.h"
#include "modbus_driver.h"

#define  ADC_BUFF_SIZE   6
uint16_t adc_buff[ADC_BUFF_SIZE];
uint16_t adc_value;
uint8_t  adc_index = 0;


void adc_init()
{
    GPIO_Init(GPIOC,(GPIO_Pin_TypeDef)GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT);
    /* De-Init ADC peripheral*/
    ADC1_DeInit();
    /* Init ADC1 peripheral */
    ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS, ADC1_CHANNEL_2, ADC1_PRESSEL_FCPU_D2, \
        ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT, ADC1_SCHMITTTRIG_CHANNEL2,\
        DISABLE); 
    /* Enable EOC interrupt */
    ADC1_ITConfig(ADC1_IT_EOCIE,ENABLE);
    
}

void adc_start()
{
    /*Start Conversion */
    ADC1_StartConversion();
}

uint16_t average_value(uint16_t* data,uint8_t size)
{
    uint16_t sum = 0;
    uint8_t  i;
    for(i=0;i<size;i++){
        sum += data[i];
    }
    sum /= size;
    return sum;
}

void adc_interrupt()
{
    if(adc_index >= ADC_BUFF_SIZE){
        adc_index = 0;
    }
    adc_buff[adc_index++] = ADC1_GetConversionValue();
    adc_value = average_value(adc_buff,ADC_BUFF_SIZE);
    ADC1_ClearITPendingBit(ADC1_IT_EOC);
}

