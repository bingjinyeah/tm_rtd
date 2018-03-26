#include "stm8s.h"
#include "modbus_driver.h"


extern uint16_t MB_InReg[];
extern uint16_t adc_value;
uint16_t adc_value_last = 0;
float resistance_value = 0;
float temperature_value;

//table from -50C to 179 C
const float temperature_resistance[] = {
    1,2,3
};

float get_temperature(float data)
{
    uint16_t i;
    float tmp0,tmp1;
    
    if(data < temperature_resistance[0]){
        return -100;
    }
    
    for(i=0;i<sizeof(temperature_resistance)-1;i++){
        if(temperature_resistance[i]==data){
            temperature_value = i - 50;
            return temperature_value;
        }else if((temperature_resistance[i] < data)&&(data < temperature_resistance[i+1])){
            tmp0 = temperature_resistance[i+1] - temperature_resistance[i];
            tmp1 = data - temperature_resistance[i];
            temperature_value = i - 50 + tmp1 / tmp0;
            return temperature_value;
        }
    }
    return 500;
}

void rtd_task()
{
    float tmp;
    
    if(adc_value_last!=adc_value){
        adc_value_last = adc_value;
        //*5 is 5V ref voltage, 15 is the scale factor,tmp is the vol diff
        //tmp = (adc_value / 1023.0) * 5 / 15.0;
        tmp = adc_value / 3069.0;
        tmp = 5 / (0.379 + tmp) - 1;
        //when adc_value=0,rtd_value=820;when adc_value=1023,rtd_value=1660;
        resistance_value = 10000 / tmp;
        temperature_value = get_temperature(resistance_value);
        MB_InReg[0] = (uint16_t)(temperature_value * 10);
    }
}