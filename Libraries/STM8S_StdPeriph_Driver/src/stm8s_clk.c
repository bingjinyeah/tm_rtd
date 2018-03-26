/**
  ******************************************************************************
  * @file    stm8s_clk.c
  * @author  MCD Application Team
  * @version V2.2.0
  * @date    30-September-2014
  * @brief   This file contains all the functions for the CLK peripheral.
   ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include "stm8s_clk.h"

/** @addtogroup STM8S_StdPeriph_Driver
  * @{
  */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private Constants ---------------------------------------------------------*/

/**
  * @addtogroup CLK_Private_Constants
  * @{
  */

CONST uint8_t HSIDivFactor[4] = {1, 2, 4, 8}; /*!< Holds the different HSI Divider factors */
CONST uint8_t CLKPrescTable[8] = {1, 2, 4, 8, 10, 16, 20, 40}; /*!< Holds the different CLK prescaler values */

/**
  * @}
  */

/* Public functions ----------------------------------------------------------*/
/**
  * @addtogroup CLK_Public_Functions
  * @{
  */

/**
  * @brief  Deinitializes the CLK peripheral registers to their default reset
  * values.
  * @param  None
  * @retval None
  * @par Warning:
  * Resetting the CCOR register: \n
  * When the CCOEN bit is set, the reset of the CCOR register require
  * two consecutive write instructions in order to reset first the CCOEN bit
  * and the second one is to reset the CCOSEL bits.
  */
void CLK_DeInit(void)
{
  CLK->ICKR = CLK_ICKR_RESET_VALUE;
  CLK->ECKR = CLK_ECKR_RESET_VALUE;
  CLK->SWR  = CLK_SWR_RESET_VALUE;
  CLK->SWCR = CLK_SWCR_RESET_VALUE;
  CLK->CKDIVR = CLK_CKDIVR_RESET_VALUE;
  CLK->PCKENR1 = CLK_PCKENR1_RESET_VALUE;
  CLK->PCKENR2 = CLK_PCKENR2_RESET_VALUE;
  CLK->CSSR = CLK_CSSR_RESET_VALUE;
  CLK->CCOR = CLK_CCOR_RESET_VALUE;
  while ((CLK->CCOR & CLK_CCOR_CCOEN)!= 0)
  {}
  CLK->CCOR = CLK_CCOR_RESET_VALUE;
  CLK->HSITRIMR = CLK_HSITRIMR_RESET_VALUE;
  CLK->SWIMCCR = CLK_SWIMCCR_RESET_VALUE;
}

/**
  * @brief  Enable or Disable the External High Speed oscillator (HSE).
  * @param   NewState new state of HSEEN, value accepted ENABLE, DISABLE.
  * @retval None
  */
void CLK_HSECmd(FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_FUNCTIONALSTATE_OK(NewState));
  
  if (NewState != DISABLE)
  {
    /* Set HSEEN bit */
    CLK->ECKR |= CLK_ECKR_HSEEN;
  }
  else
  {
    /* Reset HSEEN bit */
    CLK->ECKR &= (uint8_t)(~CLK_ECKR_HSEEN);
  }
}


/**
  * @brief  Starts or Stops manually the clock switch execution.
  * @par Full description:
  * NewState parameter set the SWEN.
  * @param   NewState new state of SWEN, value accepted ENABLE, DISABLE.
  * @retval None
  */
void CLK_ClockSwitchCmd(FunctionalState NewState)
{
  /* Check the parameters */
  assert_param(IS_FUNCTIONALSTATE_OK(NewState));
  
  if (NewState != DISABLE )
  {
    /* Enable the Clock Switch */
    CLK->SWCR |= CLK_SWCR_SWEN;
  }
  else
  {
    /* Disable the Clock Switch */
    CLK->SWCR &= (uint8_t)(~CLK_SWCR_SWEN);
  }
}

/**
  * @brief  configures the Switch from one clock to another
  * @param   CLK_SwitchMode select the clock switch mode.
  * It can be set of the values of @ref CLK_SwitchMode_TypeDef
  * @param   CLK_NewClock choice of the future clock.
  * It can be set of the values of @ref CLK_Source_TypeDef
  * @param   NewState Enable or Disable the Clock Switch interrupt.
  * @param   CLK_CurrentClockState current clock to switch OFF or to keep ON.
  * It can be set of the values of @ref CLK_CurrentClockState_TypeDef
  * @note LSI selected as master clock source only if LSI_EN option bit is set.
  * @retval ErrorStatus this shows the clock switch status (ERROR/SUCCESS).
  */
ErrorStatus CLK_ClockSwitchConfig(CLK_SwitchMode_TypeDef CLK_SwitchMode, CLK_Source_TypeDef CLK_NewClock, FunctionalState ITState, CLK_CurrentClockState_TypeDef CLK_CurrentClockState)
{
  CLK_Source_TypeDef clock_master;
  uint16_t DownCounter = CLK_TIMEOUT;
  ErrorStatus Swif = ERROR;
  
  /* Check the parameters */
  assert_param(IS_CLK_SOURCE_OK(CLK_NewClock));
  assert_param(IS_CLK_SWITCHMODE_OK(CLK_SwitchMode));
  assert_param(IS_FUNCTIONALSTATE_OK(ITState));
  assert_param(IS_CLK_CURRENTCLOCKSTATE_OK(CLK_CurrentClockState));
  
  /* Current clock master saving */
  clock_master = (CLK_Source_TypeDef)CLK->CMSR;
  
  /* Automatic switch mode management */
  if (CLK_SwitchMode == CLK_SWITCHMODE_AUTO)
  {
    /* Enables Clock switch */
    CLK->SWCR |= CLK_SWCR_SWEN;
    
    /* Enables or Disables Switch interrupt */
    if (ITState != DISABLE)
    {
      CLK->SWCR |= CLK_SWCR_SWIEN;
    }
    else
    {
      CLK->SWCR &= (uint8_t)(~CLK_SWCR_SWIEN);
    }
    
    /* Selection of the target clock source */
    CLK->SWR = (uint8_t)CLK_NewClock;
    
    /* Wait until the target clock source is ready */
    while((((CLK->SWCR & CLK_SWCR_SWBSY) != 0 )&& (DownCounter != 0)))
    {
      DownCounter--;
    }
    
    if(DownCounter != 0)
    {
      Swif = SUCCESS;
    }
    else
    {
      Swif = ERROR;
    }
  }
  else /* CLK_SwitchMode == CLK_SWITCHMODE_MANUAL */
  {
    /* Enables or Disables Switch interrupt  if required  */
    if (ITState != DISABLE)
    {
      CLK->SWCR |= CLK_SWCR_SWIEN;
    }
    else
    {
      CLK->SWCR &= (uint8_t)(~CLK_SWCR_SWIEN);
    }
    
    /* Selection of the target clock source */
    CLK->SWR = (uint8_t)CLK_NewClock;
    
    /* Wait until the target clock source is ready */
    while((((CLK->SWCR & CLK_SWCR_SWIF) != 0 ) && (DownCounter != 0)))
    {
      DownCounter--;
    }
    
    if(DownCounter != 0)
    {
      /* Enables Clock switch */
      CLK->SWCR |= CLK_SWCR_SWEN;
      Swif = SUCCESS;
    }
    else
    {
      Swif = ERROR;
    }
  }
  if(Swif != ERROR)
  {
    /* Switch OFF current clock if required */
    if((CLK_CurrentClockState == CLK_CURRENTCLOCKSTATE_DISABLE) && ( clock_master == CLK_SOURCE_HSI))
    {
      CLK->ICKR &= (uint8_t)(~CLK_ICKR_HSIEN);
    }
    else if((CLK_CurrentClockState == CLK_CURRENTCLOCKSTATE_DISABLE) && ( clock_master == CLK_SOURCE_LSI))
    {
      CLK->ICKR &= (uint8_t)(~CLK_ICKR_LSIEN);
    }
    else if ((CLK_CurrentClockState == CLK_CURRENTCLOCKSTATE_DISABLE) && ( clock_master == CLK_SOURCE_HSE))
    {
      CLK->ECKR &= (uint8_t)(~CLK_ECKR_HSEEN);
    }
  }
  return(Swif);
}

/**
  * @brief  Configures the HSI and CPU clock dividers.
  * @param   ClockPrescaler Specifies the HSI or CPU clock divider to apply.
  * @retval None
  */
void CLK_SYSCLKConfig(CLK_Prescaler_TypeDef CLK_Prescaler)
{
  /* check the parameters */
  assert_param(IS_CLK_PRESCALER_OK(CLK_Prescaler));
  
  if (((uint8_t)CLK_Prescaler & (uint8_t)0x80) == 0x00) /* Bit7 = 0 means HSI divider */
  {
    CLK->CKDIVR &= (uint8_t)(~CLK_CKDIVR_HSIDIV);
    CLK->CKDIVR |= (uint8_t)((uint8_t)CLK_Prescaler & (uint8_t)CLK_CKDIVR_HSIDIV);
  }
  else /* Bit7 = 1 means CPU divider */
  {
    CLK->CKDIVR &= (uint8_t)(~CLK_CKDIVR_CPUDIV);
    CLK->CKDIVR |= (uint8_t)((uint8_t)CLK_Prescaler & (uint8_t)CLK_CKDIVR_CPUDIV);
  }
}


/**
  * @brief  Returns the clock source used as system clock.
  * @param  None
  * @retval  Clock source used.
  * can be one of the values of @ref CLK_Source_TypeDef
  */
CLK_Source_TypeDef CLK_GetSYSCLKSource(void)
{
  return((CLK_Source_TypeDef)CLK->CMSR);
}

/**
  * @brief  This function returns the frequencies of different on chip clocks.
  * @param  None
  * @retval the master clock frequency
  */
uint32_t CLK_GetClockFreq(void)
{
  uint32_t clockfrequency = 0;
  CLK_Source_TypeDef clocksource = CLK_SOURCE_HSI;
  uint8_t tmp = 0, presc = 0;
  
  /* Get CLK source. */
  clocksource = (CLK_Source_TypeDef)CLK->CMSR;
  
  if (clocksource == CLK_SOURCE_HSI)
  {
    tmp = (uint8_t)(CLK->CKDIVR & CLK_CKDIVR_HSIDIV);
    tmp = (uint8_t)(tmp >> 3);
    presc = HSIDivFactor[tmp];
    clockfrequency = HSI_VALUE / presc;
  }
  else if ( clocksource == CLK_SOURCE_LSI)
  {
    clockfrequency = LSI_VALUE;
  }
  else
  {
    clockfrequency = HSE_VALUE;
  }
  
  return((uint32_t)clockfrequency);
}



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
