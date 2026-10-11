/*
 * @file       port_platform.c
 *
 * @brief      HW specific definitions and functions for portability
 *
* @author     Decawave Applications
*
* @attention  Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
*             All rights reserved.
 *
 */

#include <limits.h>

#include "port_platform.h"
#include "deca_device_api.h"
#include "instance.h"
#include "config.h"
#include "deca_types.h"
#include "deca_param_types.h"
#include "deca_regs.h"
#include "lis3dh.h"

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "main.h"

#include "stm32l0xx_ll_iwdg.h"

/******************************************************************************
 *
 *                              Defines
 *
 ******************************************************************************/

/******************************************************************************
 *
 *                  Port private variables and function prototypes
 *
 ******************************************************************************/

/* Motion Detection Interrupt */
static bool gMotionDetInt = false;
bool SerialData = false;
static uint32_t gStationaryTimer;
static bool gStationary = false;
volatile uint32_t SysTickCount = 0;
/*Sensor device for accelerometer*/
lis3dh_sensor_t* lis3dh_sensor = NULL;
uint32_t total_time = 0;

/******************************************************************************
 *
 *                              Time section
 *
 ******************************************************************************/

void IncSysTick()
{
	SysTickCount++;
}

/* @fn    portGetTickCnt
 * @brief wrapper for to read a SysTickTimer, which is incremented with
 *        CLOCKS_PER_SEC frequency.
 *        The resolution of time32_incr is usually 1/1000 sec.
 * */
__INLINE uint32_t
portGetTickCount(void)
{
	return SysTickCount;
}

void HAL_AdjustTickCount(uint32_t add_ticks)
{
	__disable_irq();
	SysTickCount += add_ticks;
	__enable_irq();
}


/* @fn         start_timer(uint32 *p_timestamp)
 * @brief     save system timestamp (in CLOCKS_PER_SEC)
 * @parm     p_timestamp pointer on current system timestamp
 */
void start_timer(volatile uint32_t * p_timestamp)
{
    *p_timestamp = portGetTickCount();
}


/* @fn         check_timer(uint32 timestamp , uint32 time)
 * @brief     check if time from current timestamp over expectation
 * @param     [in] timestamp - current timestamp
 * @param     [in] time - time expectation (in CLOCKS_PER_SEC)
 * @return     true - time is over
 *             false - time is not over yet
 */
bool check_timer(uint32_t timestamp, uint32_t time)
{
    uint32_t time_passing;
    uint32_t temp_tick_time = portGetTickCount();

    if (temp_tick_time >= timestamp)
    {
        time_passing = temp_tick_time - timestamp;
    }
    else
    {
        time_passing = 0xffffffffUL - timestamp + temp_tick_time;
    }

    if (time_passing >= time)
    {
        return (true);
    }

    return (false);
}
/******************************************************************************
 *
 *                              END OF Time section
 *
 ******************************************************************************/

/******************************************************************************
 *
 *                              Configuration section
 *
 ******************************************************************************/

/******************************************************************************
 *
 *                          End of configuration section
 *
 ******************************************************************************/

/* @fn      port_set_dw1000_slowrate
 * @brief   Function is used for re-initialize the SPI freq as 2MHz which does
 *          init state check
 * */
void port_set_dw1000_slowrate(void)
{
	// wait for current transaction to complete
	while ( LL_SPI_IsActiveFlag_BSY(SPI1) != 0 )
	{
	}
	// check if there is any garbage in RX register after the previous transaction
	if ( LL_SPI_IsActiveFlag_RXNE(SPI1) != 0 )
	{
		LL_SPI_ReceiveData8(SPI1);  // dummy read
	}

	LL_SPI_Disable(SPI1);
    LL_SPI_SetBaudRatePrescaler(SPI1, LL_SPI_BAUDRATEPRESCALER_DIV16 );
	LL_SPI_Enable(SPI1);
}

/* @fn      port_set_dw1000_fastrate
 * @brief   set 16MHz
 *
 * */
void port_set_dw1000_fastrate(void)
{
	while ( LL_SPI_IsActiveFlag_BSY(SPI1) != 0 )
	{
	}
	// check if there is any garbage in RX register after the previous transaction
	if ( LL_SPI_IsActiveFlag_RXNE(SPI1) != 0 )
	{
		LL_SPI_ReceiveData8(SPI1);  // dummy read
	}

	LL_SPI_Disable(SPI1);
    LL_SPI_SetBaudRatePrescaler(SPI1, LL_SPI_BAUDRATEPRESCALER_DIV2 );
	LL_SPI_Enable(SPI1);
}

/******************************************************************************
 *
 *                          End APP port section
 *
 ******************************************************************************/

/*! ----------------------------------------------------------------------------
 * Function: decamutexon()
 *
 * Description: This function should disable interrupts.
 *
 *
 * input parameters: void
 *
 * output parameters: uint16
 * returns the state of the DW1000 interrupt
 */

decaIrqStatus_t decamutexon(void)
{
	// not yet implemented
	return 0;
}
/*! ----------------------------------------------------------------------------
 * Function: decamutexoff()
 *
 * Description: This function should re-enable interrupts, or at least restore
 *              their state as returned(&saved) by decamutexon
 * This is called at the end of a critical section
 *
 * input parameters:
 * @param s - the state of the DW1000 interrupt as returned by decamutexon
 *
 * output parameters
 *
 * returns the state of the DW1000 interrupt
 */
void decamutexoff(decaIrqStatus_t j)
{
	// not yet implemented
}


/****************************************************************************//**
 *
 */

/* @fn        reset_DW1000
 * @brief    DW_RESET pin on DW1000 has 2 functions
 *             In general it is output, but it also can be used to reset the digital
 *             part of DW1000 by driving this pin low.
 *             Note, the DW_RESET pin should not be driven high externally.
 * */
void reset_DW1000(void)
{
	LL_GPIO_InitTypeDef     GPIO_InitStruct;

    GPIO_InitStruct.Pin = DW_RST_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(DW_RST_GPIO_Port, &GPIO_InitStruct);

    LL_GPIO_ResetOutputPin( DW_RST_GPIO_Port, DW_RST_Pin );

    LL_mDelay(1);

    GPIO_InitStruct.Pin = DW_RST_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(DW_RST_GPIO_Port, &GPIO_InitStruct);

    LL_mDelay(5);
}

/* @fn        port_wakeup_dw1000
 * @brief    "slow" waking up of DW1000 using DW_CS only
 * */
void port_wakeup_dw1000(void)
{
    LL_GPIO_ResetOutputPin( DW_CS_GPIO_Port, DW_CS_Pin );
    LL_mDelay(1);
    LL_GPIO_SetOutputPin( DW_CS_GPIO_Port, DW_CS_Pin );
    LL_mDelay(7);
}

/******************************************************************************
 *
 *                          End DW1000 port section
 *
 ******************************************************************************/

/******************************************************************************
 *
 *                          STM32
 *
 ******************************************************************************/
// DWM1004-DEV board has LEDs connected to +3.3V
// DWB1001/4 board has LEDs connected to the ground
// for DWB1001/4 board comment the next line
#define DWM1001_DEV

void LEDS_INVERT(uint32_t LEDS_MASK)
{
	if ( LEDS_MASK & LED_GREEN_MASK ) {
		LL_GPIO_TogglePin( LED_Green_GPIO_Port, LED_Green_Pin );
	}
	if ( LEDS_MASK & LED_BLUE_MASK ) {
		LL_GPIO_TogglePin( LED_Blue_GPIO_Port, LED_Blue_Pin );
	}
	if ( LEDS_MASK & LED_RED_MASK ) {
		LL_GPIO_TogglePin( LED_Red_GPIO_Port, LED_Red_Pin );
	}
}

#ifdef DWM1001_DEV
void LEDS_ON(uint32_t LEDS_MASK)
#else
void LEDS_OFF(uint32_t LEDS_MASK)
#endif // DWM1001_DEV
{
	if ( LEDS_MASK & LED_GREEN_MASK ) {
	    LL_GPIO_ResetOutputPin( LED_Green_GPIO_Port, LED_Green_Pin );
	}
	if ( LEDS_MASK & LED_BLUE_MASK ) {
	    LL_GPIO_ResetOutputPin( LED_Blue_GPIO_Port, LED_Blue_Pin );
	}
	if ( LEDS_MASK & LED_RED_MASK ) {
	    LL_GPIO_ResetOutputPin( LED_Red_GPIO_Port, LED_Red_Pin );
	}
}

#ifdef DWM1001_DEV
void LEDS_OFF(uint32_t LEDS_MASK)
#else
void LEDS_ON(uint32_t LEDS_MASK)
#endif // DWM1001_DEV
{
	if ( LEDS_MASK & LED_GREEN_MASK ) {
	    LL_GPIO_SetOutputPin( LED_Green_GPIO_Port, LED_Green_Pin );
	}
	if ( LEDS_MASK & LED_BLUE_MASK ) {
		LL_GPIO_SetOutputPin( LED_Blue_GPIO_Port, LED_Blue_Pin );
	}
	if ( LEDS_MASK & LED_RED_MASK ) {
		LL_GPIO_SetOutputPin( LED_Red_GPIO_Port, LED_Red_Pin );
	}
}


/******************************************************************************
 *
 *                          LIS3DH
 *
 ******************************************************************************/


/* @brief Switch ON the LIS3DH power
 * Don't forget to allow for at least 50us after powering up to stabilize
 *
 */
void lis3dh_power_ON()
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_GPIO_SetOutputPin( LIS3DH_PWR_GPIO_Port, LIS3DH_PWR_Pin );

	GPIO_InitStruct.Pin = LIS3DH_CS_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;

	LL_GPIO_Init(LIS3DH_CS_GPIO_Port, &GPIO_InitStruct);

}

/* @brief Switch OFF the LIS3DH power
 *
 */
void lis3dh_power_OFF()
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = LIS3DH_CS_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LIS3DH_CS_GPIO_Port, &GPIO_InitStruct);

	LL_GPIO_ResetOutputPin( LIS3DH_PWR_GPIO_Port, LIS3DH_PWR_Pin );
}

/* @brief SPI tranfer for LIS3DH is not implemented yet
 *
 */
bool spi_device_init(uint8_t bus, uint8_t cs)
{
	return false;
}

/* @brief SPI tranfer for LIS3DH is not implemented yet
 *
 */
bool spi_transfer_pf(uint8_t bus, uint8_t cs, uint8_t * mosi, uint8_t * miso, uint16_t len)
{
	return false;
}
/* @brief I2C tranfer for LIS3DH
 *
 */
bool i2c_slave_read(uint8_t bus, uint8_t addr, uint8_t reg,  uint8_t *data, uint16_t len)
{	// bus is not used in I2C transfer
	LL_I2C_HandleTransfer(I2C1, addr, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE );
	LL_I2C_TransmitData8(I2C1, reg);

	while( !LL_I2C_IsActiveFlag_STOP(I2C1))
	{
	}

	LL_I2C_ClearFlag_STOP(I2C1);

	LL_I2C_HandleTransfer(I2C1, addr, LL_I2C_ADDRSLAVE_7BIT, len , LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ );

 	while( !LL_I2C_IsActiveFlag_STOP(I2C1))
 	{
		if ( LL_I2C_IsActiveFlag_RXNE(I2C1) )
		{
			*data++ = LL_I2C_ReceiveData8(I2C1);
		}
	}
	LL_I2C_ClearFlag_STOP(I2C1);

	return false;
}

/* @brief Init and configure LIS3DH to wake up MCU on movement event
 *
 */
void lis3dh_configure_int()
{
	lis3dh_sensor_t* sensor = lis3dh_get_device();
		  if (sensor == NULL ) {
			  sensor = lis3dh_init_sensor (I2C_BUS, LIS3DH_I2C_ADDRESS_2, 0);
		  }

		  // enable data interrupts on INT1
		  lis3dh_int_event_config_t event_config;

		  event_config.mode = lis3dh_wake_up;
		  // event_config.mode = lis3dh_free_fall;
		  // event_config.mode = lis3dh_6d_movement;
		  // event_config.mode = lis3dh_6d_position;
		  // event_config.mode = lis3dh_4d_movement;
		  // event_config.mode = lis3dh_4d_position;
		  event_config.threshold = 10;
		  event_config.x_low_enabled  = false;
		  event_config.x_high_enabled = true;
		  event_config.y_low_enabled  = false;
		  event_config.y_high_enabled = true;
		  event_config.z_low_enabled  = false;
		  event_config.z_high_enabled = true;
		  event_config.duration = 0;
		  event_config.latch = false;

		  lis3dh_set_int_event_config (sensor, &event_config, lis3dh_int_event1_gen);
		  lis3dh_enable_int (sensor, lis3dh_int_event1, lis3dh_int1_signal, true);


		  // configure HPF and reset the reference by dummy read
		  lis3dh_config_hpf (sensor, lis3dh_hpf_normal, 0, true, true, true, true);
		  lis3dh_get_hpf_ref (sensor);

		  // LAST STEP: Finally set scale and mode to start measurements
		  lis3dh_set_scale(sensor, lis3dh_scale_2_g);
		  lis3dh_set_mode (sensor, lis3dh_odr_10, lis3dh_high_res, true, true, true);
}


/******************************************************************************
 *
 *                              IRQ section
 *
 ******************************************************************************/

void GPIO_EXTI3_Callback()
{
	gMotionDetInt = true;
}

// Button SW2 interrupt handler
void GPIO_EXTI0_Callback()
{
	LEDS_INVERT(LED_RED_MASK);
}

/******************************************************************************
 *
*                              Power management section
 *
 ******************************************************************************/

bool i2c_slave_write(uint8_t bus, uint8_t addr, uint8_t  reg,  uint8_t *data, uint16_t len)
{
	// bus is not used in I2C transfer

	LL_I2C_HandleTransfer(I2C1, addr, LL_I2C_ADDRSLAVE_7BIT, len + 1, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE );

	LL_I2C_TransmitData8(I2C1, reg);

	while( !LL_I2C_IsActiveFlag_STOP(I2C1)) {
		if ( LL_I2C_IsActiveFlag_TXIS(I2C1) ) {
			LL_I2C_TransmitData8(I2C1, *data++);
		}
	}

	LL_I2C_ClearFlag_STOP(I2C1);

	return false;
}

void Config_SysClk_HSI16( void )
{
	/* Enable Clock */
	RCC->CR |= RCC_CR_HSION;	// HSI16 oscillator ON
	while ( !( RCC->CR & RCC_CR_HSIRDY ) ); // wait until HSI16 is ready

	/* Switch System Clock */
	// HSI16 oscillator used as system clock
	RCC->CFGR = ( RCC->CFGR & ~RCC_CFGR_SW ) | RCC_CFGR_SW_HSI;
	while ( ( RCC->CFGR & RCC_CFGR_SWS ) != RCC_CFGR_SWS_HSI ); // wait until switched

	/* Disable other clocks (excluding LSE and LSI) */
	RCC->CR &= ~( RCC_CR_MSION | RCC_CR_HSEON | RCC_CR_PLLON );

	SystemCoreClockUpdate();
}

void Config_SysClk_MSI_131( void )
{
	/* Enable and Configure Clock */
	RCC->CR |= RCC_CR_MSION;
	RCC->ICSCR = ( RCC->ICSCR & ~RCC_ICSCR_MSIRANGE ) | RCC_ICSCR_MSIRANGE_1;
	while ( !( RCC->CR & RCC_CR_MSIRDY ) ); // wait until MSI is ready

	/* Switch System Clock */
	// MSI oscillator used as system clock
	RCC->CFGR = ( RCC->CFGR & ~RCC_CFGR_SW ) | RCC_CFGR_SW_MSI;
	while ( ( RCC->CFGR & RCC_CFGR_SWS ) != RCC_CFGR_SWS_MSI ); // wait unit switched

	/* Disable other clocks (excluding LSE and LSI) */
	RCC->CR &= ~( RCC_CR_HSION | RCC_CR_HSEON | RCC_CR_PLLON );

	SystemCoreClockUpdate();
}

void RTC_DeactivateWakeUpTimer()
// Deactivating WakeUp Timer
{  uint32_t tickstart;

  /* Disable the write protection for RTC registers */
  LL_RTC_DisableWriteProtection(RTC);

  /* Disable the Wakeup Timer */
  LL_RTC_WAKEUP_Disable(RTC);

  /* In case of interrupt mode is used, the interrupt source must disabled */
  LL_RTC_DisableIT_WUT(RTC);

  tickstart = portGetTickCount();
  /* Wait till RTC WUTWF flag is set and if Time out is reached exit */
  while (LL_RTC_IsActiveFlag_WUTW(RTC) == 0U)
  {
    if ((portGetTickCount() - tickstart) > RTC_TIMEOUT_VALUE)
    {
      /* Enable the write protection for RTC registers */
  	  LL_RTC_EnableWriteProtection(RTC);
  	  break;
    }
  }

  /* Enable the write protection for RTC registers */
  LL_RTC_EnableWriteProtection(RTC);

}

void RTC_SetWakeUpTimer_IT( uint32_t WakeUpCounter )
{
	  uint32_t tickstart;

	  /* Disable the write protection for RTC registers */
	  LL_RTC_DisableWriteProtection(RTC);

	  /*Check RTC WUTWF flag is reset only when wake up timer enabled*/
	  if ( LL_RTC_WAKEUP_IsEnabled( RTC ) != 0U )
	  {
	    tickstart = portGetTickCount();

	    /* Wait till RTC WUTWF flag is reset and if Time out is reached exit */
	    while (LL_RTC_IsActiveFlag_WUTW(RTC)  == 1U)
	    {
	      if ((portGetTickCount() - tickstart) > RTC_TIMEOUT_VALUE)
	      {
	        /* Enable the write protection for RTC registers */
	    	  LL_RTC_EnableWriteProtection(RTC);
	          return;
	      }
	    }
	  }
	  /* Disable the Wake-Up timer */
	  LL_RTC_WAKEUP_Disable(RTC);

	  /* Clear flag Wake-Up */
	  LL_RTC_ClearFlag_WUT(RTC);

	  tickstart = portGetTickCount();

	  /* Wait till RTC WUTWF flag is set and if Time out is reached exit */
	  while ( LL_RTC_IsActiveFlag_WUTW(RTC) == 0U)
	  {
	    if ((portGetTickCount() - tickstart) > RTC_TIMEOUT_VALUE)
	    {
	      /* Enable the write protection for RTC registers */

	    	  LL_RTC_EnableWriteProtection(RTC);
	      return ;
	    }
	  }

	  /* Configure the Wakeup Timer counter */
	  RTC->WUTR = WakeUpCounter;

	  /* Clear the Wakeup Timer clock source bits in CR register */
	  /* Configure the clock source */
	  // should be configured at startup
	  RTC->CR &= (uint32_t)~RTC_CR_WUCKSEL; // set prescaler to RTC_WAKEUPCLOCK_RTCCLK_DIV16

	  /* RTC WakeUpTimer Interrupt Configuration: EXTI configuration */
	  EXTI->IMR |= EXTI_IMR_IM20;
	  EXTI->RTSR |= EXTI_IMR_IM20;

	  /* Configure the Interrupt in the RTC_CR register */
	  LL_RTC_EnableIT_WUT( RTC );

	  /* Enable the Wakeup Timer */
	  LL_RTC_WAKEUP_Enable( RTC );

	  /* Enable the write protection for RTC registers */
	  LL_RTC_EnableWriteProtection(RTC);
}

/**
  * @}
  */
/**
  * @brief Set Wakeup from Stop mode interrupt flag selection.
  * @note It is the application responsibility to enable the interrupt used as
  *       usart_wkup interrupt source before entering low-power mode.
  */

int UARTE_StopModeWakeUpSourceConfig(  )
{
  int status = 0;
  volatile uint32_t tickstart;
  volatile uint32_t difference;

  uint32_t WakeUpType = LL_USART_GetWKUPType(USART2);
  if ( WakeUpType != USART_CR3_WUS_1 ) {
	  LL_USART_Disable(USART2);
	  MODIFY_REG(USART2->CR3, USART_CR3_WUS, USART_CR3_WUS_1);
	  LL_USART_Enable(USART2);
  }

  /* Init tickstart for timeout managment*/
  tickstart = portGetTickCount();

  /* Wait until REACK flag is set */

  uint32_t REACK_Flag = LL_USART_IsActiveFlag_REACK(USART2);
  while ( REACK_Flag == 0 )
  {
	REACK_Flag = LL_USART_IsActiveFlag_REACK(USART2);
    /* Check for the Timeout */
	difference = portGetTickCount();
	difference -= tickstart;
    if ( difference > RTC_TIMEOUT_VALUE )
      {
        /* Disable TXE, RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts for the interrupt process */
        CLEAR_BIT(USART2->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE | USART_CR1_TXEIE));
        CLEAR_BIT(USART2->CR3, USART_CR3_EIE);

        status = 1;
        break;
      }
  }

  return status;
}

/**
  * @brief  System Power Configuration
  *         The system Power is configured as follow :
  *            + VREFINT OFF, with fast wakeup enabled
  *            + No IWDG
  *            + Wakeup using PWR_WAKEUP_PIN3
  * @param None
  * @retval None
  */
void SystemPower_Config(void)
{
  /* Enable Power Control clock */
	SET_BIT(RCC->APB1ENR, (RCC_APB1ENR_PWREN));
	/* Enable Ultra low power mode */
	SET_BIT(PWR->CR, PWR_CR_ULP);
}


/**
  * @brief Enters Stop mode.
  * @note In Stop mode, all I/O pins keep the same state as in Run mode.
  * @note When exiting Stop mode by issuing an interrupt or a wakeup event,
  *        MSI or HSI16 RCoscillator is selected as system clock depending
  *        the bit STOPWUCK in the RCC_CFGR register.
  * @note When the voltage regulator operates in low power mode, an additional
  *         startup delay is incurred when waking up from Stop mode.
  *         By keeping the internal regulator ON during Stop mode, the consumption
  *         is higher although the startup time is reduced.
  * @note Before entering in this function, it is important to ensure that the WUF
  *       wakeup flag is cleared. To perform this action, it is possible to call the
  *       following macro : __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU)
  *
  * @param Regulator: Specifies the regulator state in Stop mode.
  *          This parameter can be one of the following values:
  *            @arg PWR_MAINREGULATOR_ON: Stop mode with regulator ON
  *            @arg PWR_LOWPOWERREGULATOR_ON: Stop mode with low power regulator ON
  * @param STOPEntry: Specifies if Stop mode in entered with WFI or WFE instruction.
  *          This parameter can be one of the following values:
  *            @arg PWR_STOPENTRY_WFI: Enter Stop mode with WFI instruction
  *            @arg PWR_STOPENTRY_WFE: Enter Stop mode with WFE instruction
  * @retval None
  */

void PWR_EnterSTOPMode()
{
  uint32_t Regulator = PWR_CR_LPSDSR;

  uint32_t tmpreg = 0U;

  /* Select the regulator state in Stop mode ---------------------------------*/
  tmpreg = PWR->CR;

  /* Clear PDDS and LPDS bits */
  CLEAR_BIT(tmpreg, (PWR_CR_PDDS | PWR_CR_LPSDSR));

 /* Set LPSDSR bit according to PWR_Regulator value */
  SET_BIT(tmpreg, Regulator);

  /* Store the new value */
  PWR->CR = tmpreg;

  /* Set SLEEPDEEP bit of Cortex System Control Register */
  SET_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);

  /* Select Stop mode entry --------------------------------------------------*/
  {
    /* Request Wait For Interrupt */
    __WFI();
  }

  /* Reset SLEEPDEEP bit of Cortex System Control Register */
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);

}

/* @brief Low power implementation
 * @parm delay : low power sleeping time in ms
 * */
void low_power(uint32_t delay)
{

    /* Disable Wakeup Counter */
	RTC_DeactivateWakeUpTimer();

    /* make sure that no UART transfer is on-going */
	while ( !LL_USART_IsActiveFlag_TC( USART2 ) );
    while( LL_USART_IsActiveFlag_BUSY(USART2) );
    /* make sure that UART is ready to receive
//    * (test carried out again later in UARTE_StopModeWakeUpSourceConfig) */
    while( LL_USART_IsActiveFlag_REACK(USART2) == 0 );

    /* ## Setting the Wake up time ############################################*/
    /*  RTC Wakeup Interrupt Generation:
      Wakeup Time Base = (RTC_WAKEUPCLOCK_RTCCLK_DIV /(LSE or LSI))
      Wakeup Time = Wakeup Time Base * WakeUpCounter
      = (RTC_WAKEUPCLOCK_RTCCLK_DIV /(LSE or LSI)) * WakeUpCounter
        ==> WakeUpCounter = Wakeup Time / Wakeup Time Base

      To configure the wake up timer to 20s the WakeUpCounter is set to 0x1FFF:
      RTC_WAKEUPCLOCK_RTCCLK_DIV = RTCCLK_Div16 = 16
      Wakeup Time Base = 16 /(~39.000KHz) = ~0,410 ms
      Wakeup Time = ~20s = 0,410ms  * WakeUpCounter
        ==> WakeUpCounter = ~20s/0,410ms = 48780 = 0xBE8C */
    uint32_t WakeUpCounter = (delay * LSI_FREQ) / 16000;
    RTC_SetWakeUpTimer_IT( WakeUpCounter );

    /* set the wake-up event:
     * specify wake-up on RXNE flag */

    if ( UARTE_StopModeWakeUpSourceConfig(  )!= 0 )
    {
      Error_Handler();
    }

    /* Enable the UART Wake UP from stop mode Interrupt */
    USART2_ENABLE_IT( UART_IT_WUF );
    LL_USART_EnableIT_PE( USART2 );
    LL_USART_EnableIT_ERROR( USART2 );


     /* enable MCU wake-up by UART */
     LL_USART_EnableInStopMode(USART2);
    /* Configure the system Power */
    SystemPower_Config();

    /* Enter Stop Mode */
    PWR_EnterSTOPMode();

    /* Wake Up based on RXNE flag successful */
    LL_USART_DisableInStopMode( USART2 );

    /* Correct tick value after the sleep */
    HAL_AdjustTickCount(delay);

}

/**
  * @brief  UART error callback.
  * @param  huart UART handle.
  * @retval None
  */
__weak void HAL_UART_ErrorCallback()
{
}



/**
  * @brief  End ongoing Rx transfer on UART peripheral (following error detection or Reception completion).
  * @param  huart UART handle.
  * @retval None
  */
static void UART2_EndRxTransfer()
{
}


/**
  * @brief RX interrrupt handler for 7 or 8 bits data word length .
  * @param huart UART handle.
  * @retval None
  */

void UART2_IRQHandler()
{
	  uint32_t isrflags   = READ_REG(USART2->ISR);
	  uint32_t cr1its     = READ_REG(USART2->CR1);
	  uint32_t cr3its     = READ_REG(USART2->CR3);

	  uint32_t errorflags;
	  uint32_t errorcode = HAL_UART_ERROR_NONE;

	  /* If no error occurs */
	  errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE));
	  if (errorflags == 0U)
	  {
	    /* UART in mode Receiver ---------------------------------------------------*/
	    if (((isrflags & USART_ISR_RXNE) != 0U)
	        && ((cr1its & USART_CR1_RXNEIE) != 0U))
	    {
	    	UART_RxCpltCallback( LL_USART_ReceiveData8( USART2 ) );
	    	return;
	    }
	  }

	  /* If some errors occur */
	  if ((errorflags != 0U)
	      && (((cr3its & USART_CR3_EIE) != 0U)
	          || ((cr1its & (USART_CR1_RXNEIE | USART_CR1_PEIE)) != 0U)))
	  {
	    /* UART parity error interrupt occurred -------------------------------------*/
	    if (((isrflags & USART_ISR_PE) != 0U) && ((cr1its & USART_CR1_PEIE) != 0U))
	    {
	    	LL_USART_ClearFlag_PE( USART2 );
	    	errorcode |= HAL_UART_ERROR_PE;
	    }

	    /* UART frame error interrupt occurred --------------------------------------*/
	    if (((isrflags & USART_ISR_FE) != 0U) && ((cr3its & USART_CR3_EIE) != 0U))
	    {
	    	LL_USART_ClearFlag_FE( USART2 );
	    	errorcode |= HAL_UART_ERROR_FE;
	    }

	    /* UART noise error interrupt occurred --------------------------------------*/
	    if (((isrflags & USART_ISR_NE) != 0U) && ((cr3its & USART_CR3_EIE) != 0U))
	    {
	    	LL_USART_ClearFlag_NE( USART2 );
	    	errorcode |= HAL_UART_ERROR_NE;
	    }

	    /* UART Over-Run interrupt occurred -----------------------------------------*/
	    if (((isrflags & USART_ISR_ORE) != 0U)
	        && (((cr1its & USART_CR1_RXNEIE) != 0U) ||
	            ((cr3its & USART_CR3_EIE) != 0U)))
	    {
	    	LL_USART_ClearFlag_ORE( USART2 );
	    	errorcode |= HAL_UART_ERROR_ORE;
	    }

	    /* Call UART Error Call back function if need be --------------------------*/
	    if ( errorcode != HAL_UART_ERROR_NONE)
	    {
	      /* UART in mode Receiver ---------------------------------------------------*/
	      if (((isrflags & USART_ISR_RXNE) != 0U)
	          && ((cr1its & USART_CR1_RXNEIE) != 0U))
	      {
		    	UART_RxCpltCallback( LL_USART_ReceiveData8( USART2 ) );
	      }

	      /* If Overrun error occurs, or if any error occurs in DMA mode reception,
	         consider error as blocking */
	      if ((READ_BIT(USART2->CR3, USART_CR3_DMAR)) ||
	          ((errorcode & HAL_UART_ERROR_ORE) != 0U))
	      {
	        /* Blocking error : transfer is aborted
	           Set the UART state ready to be able to start again the process,
	           Disable Rx Interrupts, and disable Rx DMA request, if ongoing */
	        UART2_EndRxTransfer();

	        /* Disable the UART DMA Rx request if enabled */
	        if (READ_BIT(USART2->CR3, USART_CR3_DMAR))
	        {
	          CLEAR_BIT(USART2->CR3, USART_CR3_DMAR);
	        }
	        else
	        {
	          /* Call user error callback */
	          /*Call legacy weak error callback*/
	          HAL_UART_ErrorCallback();
	        }
	      }
	      else
	      {
	        /* Non Blocking error : transfer could go on.
	           Error is notified to user through user error callback */
	        /*Call legacy weak error callback*/
	        HAL_UART_ErrorCallback();
	        errorcode = HAL_UART_ERROR_NONE;
	      }
	    }
	    return;
	  } /* End if some error occurs */

	  /* UART wakeup from Stop mode interrupt occurred ---------------------------*/
	  if (((isrflags & USART_ISR_WUF) != 0U) && ((cr3its & USART_CR3_WUFIE) != 0U))
	  {
		  LL_USART_ClearFlag_WKUP( USART2 );
  	      return;
	  }

	  /* UART in mode Transmitter ------------------------------------------------*/
	  if (((isrflags & USART_ISR_TXE) != 0U)
	      && ((cr1its & USART_CR1_TXEIE) != 0U))
	  {
	    // Transmit callback placeholder
	    return;
	  }

	  /* UART in mode Transmitter (transmission end) -----------------------------*/
	  if (((isrflags & USART_ISR_TC) != 0U) && ((cr1its & USART_CR1_TCIE) != 0U))
	  {
		  /* Disable the UART Transmit Complete Interrupt */
		  /* Should be already disabled anyway */
		  LL_USART_DisableIT_TC( USART2 );

	    return;
	  }

}

/*
* @brief Run the accelerometer motion detection test mode.
*
* Initialise the accelerometer to detect motion and generate
* an interrupt.  Call the accelerometer foreground task, waiting
* for the interrupt to occur.
* On detecting motion turn on the blue LED, after a period of
* inactivity, turn off the LED and wait for another motion event.
*/
void vTestModeMotionDetect(void)
{
    param_block_t *pbss = get_pbssConfig();

    __disable_irq();
    if( gMotionDetInt )
    {
    	gMotionDetInt = false;
    	gStationary = false;
    	start_timer( &gStationaryTimer );
        app.pcurrent_blink_interval_ms = &(pbss->blink.interval_in_ms);
    }
    __enable_irq();
    if ( !gStationary && check_timer(gStationaryTimer, MOTIONLESS_TIMER_VALUE ) ) {
    	gStationary = true;
        app.pcurrent_blink_interval_ms = &(pbss->blink.interval_slow_in_ms);
    }

    if(gStationary)
    {
        LEDS_OFF(LED_GREEN_MASK);
    }else{
        LEDS_ON(LED_GREEN_MASK);
    }
}



/**
  * @brief restart through IWDG
  * @param None
  * @retval None
  */
void Restart(void)
{
  LL_IWDG_Enable(IWDG);
  LL_IWDG_EnableWriteAccess(IWDG);
  LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_4);
  LL_IWDG_SetReloadCounter(IWDG, 10);
  while (LL_IWDG_IsReady(IWDG) != 1)
  {
  }

  LL_IWDG_SetWindow(IWDG, 10);
  LL_IWDG_ReloadCounter(IWDG);

  __disable_irq();
  while (1)
  {
  }

}
