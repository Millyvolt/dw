/*
 * @file       port_platform.h
 *
 * @brief      HW specific definitions and functions for portability
 *
 * @author     Decawave
 *
 * @attention  Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *             All rights reserved.
 *
 */

#ifndef PORT_PLATFORM_H_
#define PORT_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
//#include <types.h>

#include "main.h"
//#include "stm32l0xx_hal.h"

#include "deca_types.h"

#define TOTAL_LEDS      3
#define LED_GREEN       0
#define LED_BLUE        1
#define LED_RED     3

#define LED_GREEN_MASK      (1<<LED_GREEN)
#define LED_BLUE_MASK       (1<<LED_BLUE)
#define LED_RED_MASK        (1<<LED_RED)

/** Leaves the minimum of the two arguments */
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* MIN */

#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif /* MAX */

#ifndef SWAP
#define SWAP(a,b) {a^=b;b^=a;a^=b;}
#endif /* SWAP */

#define LSI_FREQ            LSI_VALUE

// Value in ms to detect stationary mode of the Tag
#define MOTIONLESS_TIMER_VALUE      3000

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#define HAL_Delay           LL_mDelay

/** @defgroup UART_Error_Definition   UART Error Definition
  * @{
  */
#define  HAL_UART_ERROR_NONE             ((uint32_t)0x00000000U)    /*!< No error                */
#define  HAL_UART_ERROR_PE               ((uint32_t)0x00000001U)    /*!< Parity error            */
#define  HAL_UART_ERROR_NE               ((uint32_t)0x00000002U)    /*!< Noise error             */
#define  HAL_UART_ERROR_FE               ((uint32_t)0x00000004U)    /*!< Frame error             */
#define  HAL_UART_ERROR_ORE              ((uint32_t)0x00000008U)    /*!< Overrun error           */
#define  HAL_UART_ERROR_DMA              ((uint32_t)0x00000010U)    /*!< DMA transfer error      */

#define PWR_LOWPOWERREGULATOR_ON       PWR_CR_LPSDSR

#define RTC_TIMEOUT_VALUE  1000U

/** @defgroup UART_WakeUp_from_Stop_Selection   UART WakeUp From Stop Selection
  * @{
  */
#define UART_WAKEUP_ON_ADDRESS              0x00000000U             /*!< UART wake-up on address                         */
#define UART_WAKEUP_ON_STARTBIT             USART_CR3_WUS_1         /*!< UART wake-up on start bit                       */
#define UART_WAKEUP_ON_READDATA_NONEMPTY    USART_CR3_WUS           /*!< UART wake-up on receive data register not empty or RXFIFO is not empty */

/** @defgroup UART_Interruption_Mask    UART Interruptions Flag Mask
  * @{
  */
#define UART_IT_MASK                        0x001FU  /*!< UART interruptions flags mask */

#define UART_IT_WUF                         0x1476U                  /*!< UART wake-up from stop mode interruption       */

#define USART2_ENABLE_IT(__INTERRUPT__)   (((((uint8_t)(__INTERRUPT__)) >> 5U) == 1U)? (USART2->CR1 |= (1U << ((__INTERRUPT__) & UART_IT_MASK))): \
                                                           ((((uint8_t)(__INTERRUPT__)) >> 5U) == 2U)? (USART2->CR2 |= (1U << ((__INTERRUPT__) & UART_IT_MASK))): \
                                                           (USART2->CR3 |= (1U << ((__INTERRUPT__) & UART_IT_MASK))))

typedef enum {
    VALID = 1,
    UNVALID = -1
} check_ret_e ;

/*Mask to disable DW1000 Interrupt*/
#define DISABLE_DW1000_INTERRUPT 0xFFFFFFFF

/******************************************************************************
 *
 *                              port function prototypes
 *
 ******************************************************************************/
uint32_t portGetTickCount(void);
void port_wakeup_dw1000(void);

/* Function is used for initialize the SPI freq as 2MHz
 * port_set_dw1000_slowrate initialize the SPI freq as 2MHz which does init
 * state check
 */
void port_set_dw1000_slowrate(void);
void port_set_dw1000_fastrate(void);
void reset_DW1000(void);

int inittestapplication(void);
void peripherals_init(void);
void deca_uart_init(void);
uint32_t deca_uart_receive( uint8_t * buffer, size_t size);
void deca_uart_transmit(char *ptr);
bool deca_uart_rx_data_ready(void);
void low_power(uint32_t);
void RestartUART_timer(void);
//void RxComplete(struct __UART_HandleTypeDef *huart);

void LEDS_INVERT(uint32_t LEDS_MASK);
void LEDS_OFF(uint32_t LEDS_MASK);
void LEDS_ON(uint32_t LEDS_MASK);

bool spi_device_init(uint8_t bus, uint8_t cs);
bool spi_transfer_pf(uint8_t bus, uint8_t cs, uint8_t * mosi, uint8_t * miso, uint16_t len);
bool i2c_slave_read(uint8_t bus, uint8_t addr, uint8_t reg,  uint8_t *data, uint16_t len);
bool i2c_slave_write(uint8_t bus, uint8_t addr, uint8_t reg,  uint8_t *data, uint16_t len);

void lis3dh_power_OFF(void);
void lis3dh_power_ON(void);

void RTC_Config(void);
void RTC_DeactivateWakeUpTimer(void);

void vTestModeMotionDetect(void);
void lis3dh_configure_int(void);

void port_stop_all_UWB(void);

void IncSysTick(void);
void GPIO_EXTI3_Callback(void);
void GPIO_EXTI0_Callback(void);

bool check_timer(uint32_t timestamp, uint32_t time);
void start_timer(volatile uint32_t * p_timestamp);

void UART2_IRQHandler(void);
void UART_RxCpltCallback( uint8_t data );

void dw_printf( const char * format, ... );

#ifdef __cplusplus
}
#endif

#endif /* PORT_PLATFORM_H_ */



/*
 * Taken from the Linux Kernel
 *
 */

#ifndef _LINUX_CIRC_BUF_H
#define _LINUX_CIRC_BUF_H 1

struct circ_buf {
    char *buf;
    int head;
    int tail;
};

/* Return count in buffer.  */
#define CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(head,tail,size) CIRC_CNT((tail),((head)+1),(size))

/* Return count up to the end of the buffer.  Carefully avoid
   accessing head and tail more than once, so they can change
   underneath us without returning inconsistent results.  */
#define CIRC_CNT_TO_END(head,tail,size) \
    ({int end = (size) - (tail); \
      int n = ((head) + end) & ((size)-1); \
      n < end ? n : end;})

/* Return space available up to the end of the buffer.  */
#define CIRC_SPACE_TO_END(head,tail,size) \
    ({int end = (size) - 1 - (head); \
      int n = (end + (tail)) & ((size)-1); \
      n <= end ? n : end+1;})


#endif /* _LINUX_CIRC_BUF_H  */


