/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdlib.h>
#include "lis3dh.h"
#include "stm32l041xx.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define  TICK_INT_PRIORITY            ((uint32_t)0U)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_CRC_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#include "port_platform.h"
#include "deca_types.h"
#include "deca_param_types.h"
#include "deca_regs.h"
#include "deca_device_api.h"
#include "default_config.h"
#include "instance.h"
#include "config.h"
#include "translate.h"

/* Structure for all global variables */
app_cfg_t app;


/*! ----------------------------------------------------------------------------
 *  @file    main.c
 *  @brief   Single-sided two-way ranging (SS TWR) API example code
 *
 *           This code implements a Single Sided Two Way Ranging between 2 x DWM1004C devices
 *           The same code supports both modes: Initiator and Responder
 *           The Initiator sends a "poll" frame (recording the TX time-stamp of the poll), after which it waits for a "response" message from the Responder
 *           to complete the exchange. The response message contains the remote responder's time-stamps of poll RX, and response TX. With this data and the
 *           local time-stamps, (of poll TX and response RX), the Initator works out a value for the time-of-flight over-the-air and, thus, the estimated 
 *           distance between the two devices, which it writes to the UART console output.
 *
 * @attention
 *
 * Copyright 2020 (c) Decawave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author Decawave
 */

#include <stdio.h>
#include <string.h>

#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_spi.h"
#include "port_platform.h"

extern void set_los_nlos_config( uint8_t nlos );

/* Default antenna delay values for 64 MHz PRF. */
#define TX_ANT_DLY 16436
#define RX_ANT_DLY 16436

/* Frames used in the ranging process. */
static uint8 tx_poll_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'W', 'A', 'V', 'E', 0xE0, 0, 0};
static uint8 rx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
/* Length of the common part of the message (up to and including the function code). */
#define ALL_MSG_COMMON_LEN 10
/* Indexes to access some of the fields in the frames defined above. */
#define ALL_MSG_SN_IDX 2
#define RESP_MSG_POLL_RX_TS_IDX 10
#define RESP_MSG_RESP_TX_TS_IDX 14  // actually NOT used : should be 15 , as 5 bytes timestamps exchanged in this FW, for temporary compatibility with diag tool
#define RESP_MSG_TS_LEN 4
/* Frame sequence number, incremented after each transmission. */
static uint8 frame_seq_nb = 0;

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
#define RX_BUF_LEN 64 // 20
static uint8 rx_buffer[RX_BUF_LEN];
static uint8_t seq_number;


/* Hold copy of status register state here for reference so that it can be examined at a debug breakpoint. */
static uint32 status_reg = 0;

/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) conversion factor.
 * 1 uus = 512 / 499.2 µs and 1 µs = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME 65536

/* Delay between frames, in UWB microseconds. */
#define POLL_TX_TO_RESP_RX_DLY_UUS 140
/* Receive response timeout. */
#define RESP_RX_TIMEOUT_UUS 210

/* Speed of light in air, in metres per second. */
#define SPEED_OF_LIGHT 299702547

/*
 * Code for Log10() and dB computation using integers only *
 *
 */
#define LOG_TAB_SZ  32

#define LOG10_2_x1000 301 // 100.0*(10*Log10(2))

const uint16_t log10_tab_1_2_int[LOG_TAB_SZ] = {
    0,  // = 100.0 * (10.0 * Log10( i / LOG_TAB_SZ)) i  = index in the table ranging from 0 to LOG_TAB_SZ - 1
    13,
    26,
    38,
    51,
    63,
    74,
    85,
    96,
    107,
    118,
    128,
    138,
    148,
    157,
    166,
    176,
    185,
    193,
    202,
    210,
    219,
    227,
    235,
    243,
    250,
    258,
    265,
    273,
    280,
    287,
    294
};

uint8_t get_msb(uint32_t x)
{
    uint8_t i = 1;
    for (i = 1; i < 32; i++)
    {
        if (x < (1UL << i))
        {
            break;
        }
    }
    return(i - 1);
}

uint32_t dB_x100(uint32_t x)
{
    uint32_t res, pow2_low_bound;
    int32_t log_tab_idx, msb;

    msb = get_msb(x);
    pow2_low_bound = (1UL << msb); // pow2_low_bound <= x < 2 x pow2_low_bound
    log_tab_idx = ((x - pow2_low_bound)*LOG_TAB_SZ) / pow2_low_bound; // 0<= log_tab_idx < LOG_TAB_SZ

    // x = r x 2^msb
    // log10(x) = log10(r) + msb x log10(2)
    // log10(x) = log10_tab[LOG_TAB_SZ x (r-1)] + msb x log10(2)
    res = (uint32_t)log10_tab_1_2_int[log_tab_idx] + msb * LOG10_2_x1000;

    return(res);
}

void get_ext_diag_info( dwt_rxdiag_t *p_diagnostics, int32_t *p_rx_lvl )
{
    uint32_t arg, n_rxpacc , n_rxpacc_nosat , br , rxprf ;
    int32_t  div , A_dBm_x100;

    dwt_readdiagnostics( p_diagnostics );

    n_rxpacc        = p_diagnostics->rxPreamCount; // was read from  = (dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXPACC_MASK) >> RX_FINFO_RXPACC_SHIFT  ;
    n_rxpacc_nosat  = dwt_read16bitoffsetreg(DRX_CONF_ID, 0x2C ); 
    if( n_rxpacc == n_rxpacc_nosat )
    {
        // then adjustment is needed
        br = (dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXBR_MASK) >> RX_FINFO_RXBR_SHIFT  ;
        if( br == RX_FINFO_RXBR_110k )
        {
/*          sfd_len = 64; The standard specifies the SFD, which consists of the preamble symbols either not sent, or sent as normal or
    sent inverted (i.e. positive and negative pulses reversed) in a defined pattern 8 symbol times long for data
    rates other than 110 kbps, and 64 symbols long for the 110 kbps mode. */
            n_rxpacc -= 64; // We assume standard SFD - see table 18 of dw1000_user_manual_2.12
        } else
        {
//          sfd_len = 8;
            n_rxpacc -= 5;  //  We assume standard SFD - see table 18 of dw1000_user_manual_2.12
        }
    }

    rxprf = (dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXPRF_MASK) >> RX_FINFO_RXPRF_SHIFT  ;
    if( rxprf == RX_FINFO_RXPRF_16M )
    {
        A_dBm_x100 = 11377; // 113.77 for a PRF of 16 MHz,
    } else
    {
        A_dBm_x100 = 12174; //  121.74 for a PRF of 64 MHz,
    }

    arg  = p_diagnostics->firstPathAmp1*p_diagnostics->firstPathAmp1;
    arg += p_diagnostics->firstPathAmp2*p_diagnostics->firstPathAmp2;
    arg += p_diagnostics->firstPathAmp3*p_diagnostics->firstPathAmp3;

    div = dB_x100( n_rxpacc*n_rxpacc ) ;

    // as documented in DW1000 User Manual section 4.7.2 Estimating the receive signal power
    // c = (uint32_t) (p_diagnostics->maxGrowthCIR) << 17;
    *p_rx_lvl = ( dB_x100( (uint32_t) (p_diagnostics->maxGrowthCIR) << 17 ) - div ) -  A_dBm_x100  ;

    return;
}


/* Delay between frames, in UWB microseconds. */
#define POLL_RX_TO_RESP_TX_DLY_UUS 1000 // 330
static uint8 tx_resp_msg[] = {0x41, 0x88, 0, 0xCA, 0xDE, 'V', 'E', 'W', 'A', 0xE1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void resp_to_poll( uint8_t *p_rx_buffer , uint8_t len )
{
    tx_poll_msg[ ALL_MSG_SN_IDX ] = 0;
    if (memcmp(p_rx_buffer, tx_poll_msg, ALL_MSG_COMMON_LEN) == 0)
    {
        uint32 resp_tx_time , pream_len , bitrate_kbps ,t_xmt_pld_uus;
        uint64_t poll_rx_ts , resp_tx_ts ;
        int ret;

        /* Retrieve poll reception timestamp. */
        poll_rx_ts = 0;
        dwt_readrxtimestamp( (uint8_t*)&poll_rx_ts );

        /* Compute final message transmission time. */
        pream_len = deca_to_plen( app.pConfig->dwt_config.txPreambLength );
        /* Compute final message transmission time. */
        bitrate_kbps    = deca_to_bitrate( app.pConfig->dwt_config.dataRate );
        if( bitrate_kbps != 0 )
        {
            t_xmt_pld_uus = ( sizeof(tx_resp_msg) * 8 *1000 )/bitrate_kbps;
/*          if( bitrate_kbps  == 110 )
            {
                t_xmt_pld_uus += EXTRA_DLY_110K;
            } */
        }
        resp_tx_time = (poll_rx_ts + ((POLL_RX_TO_RESP_TX_DLY_UUS + pream_len + t_xmt_pld_uus ) * UUS_TO_DWT_TIME)) >> 8;

        resp_tx_time = (poll_rx_ts + ((POLL_RX_TO_RESP_TX_DLY_UUS + pream_len) * UUS_TO_DWT_TIME)) >> 8;
        dwt_setdelayedtrxtime(resp_tx_time);

        /* Response TX timestamp is the transmission time we programmed plus the antenna delay. */
        resp_tx_ts = (((uint64_t)(resp_tx_time & 0xFFFFFFFEUL)) << 8) + TX_ANT_DLY;

        /* Write all timestamps in the final message. */
        memset( &tx_resp_msg[RESP_MSG_POLL_RX_TS_IDX   ], 0 , 2*5 ); // currently 5 bytes per timestamp in the payload
        memcpy( &tx_resp_msg[RESP_MSG_POLL_RX_TS_IDX   ], &poll_rx_ts , sizeof(uint32_t) );
        memcpy( &tx_resp_msg[RESP_MSG_POLL_RX_TS_IDX +5], &resp_tx_ts , sizeof(uint32_t) );

        /* Write and send the response message. */
        tx_resp_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
        dwt_writetxdata( RESP_MSG_POLL_RX_TS_IDX + 2*5 + 2 , tx_resp_msg, 0); /* Zero offset in TX buffer. */
        dwt_writetxfctrl(RESP_MSG_POLL_RX_TS_IDX + 2*5 + 2 , 0, 1); /* Zero offset in TX buffer, ranging. */
        ret = dwt_starttx(DWT_START_TX_DELAYED);

        /* If dwt_starttx() returns an error, abandon this ranging exchange and proceed to the next one. */
        if (ret == DWT_SUCCESS)
        {
            /* Poll DW1000 until TX frame sent event set. */
            while (!(dwt_read32bitreg(SYS_STATUS_ID) & SYS_STATUS_TXFRS))
            { };

            /* Clear TXFRS event. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

            /* Increment frame sequence number after transmission of the poll message (modulo 256). */
            frame_seq_nb++;
        } else
        {
            dw_printf("ERR TX_DLYED\n");
        }
    }
    return;
}

void calc_tof_from_rcved_resp( uint8_t *p_rx_buffer , uint8_t len )
{
    uint32 poll_tx_ts, resp_rx_ts, poll_rx_ts, resp_tx_ts;
    int32_t rtd_init, rtd_resp , rx_lvl , clk_offs_ppm_x100;
    dwt_rxdiag_t diagnostics;
    uint64_t dist_cm , c_hz2ppm_mult_chan;
    int64_t clk_offs;

    if (memcmp(p_rx_buffer, rx_resp_msg, ALL_MSG_COMMON_LEN) == 0)
    {
        /* Retrieve poll transmission and response reception timestamps. */
        poll_tx_ts = dwt_readtxtimestamplo32();
        resp_rx_ts = dwt_readrxtimestamplo32();
        /* Get timestamps embedded in response message. */
        memcpy(&poll_rx_ts , &p_rx_buffer[RESP_MSG_POLL_RX_TS_IDX    ] , sizeof(uint32_t) );
        memcpy(&resp_tx_ts , &p_rx_buffer[RESP_MSG_POLL_RX_TS_IDX + 5] , sizeof(uint32_t) );
            
        /* Compute time of flight and distance, using clock offset ratio to correct for differing local and remote clock rates */
        rtd_init = resp_rx_ts - poll_tx_ts;
        rtd_resp = resp_tx_ts - poll_rx_ts;

        // tof = ((rtd_init - rtd_resp * (1 - clockOffsetRatio)) / 2.0) * DWT_TIME_UNITS;
        // distance = tof * SPEED_OF_LIGHT;

// Multiplication factors to convert frequency offset in Hertz to PPM crystal offset
// NB: also changes sign so a positive value means the local RX clock is running slower than the remote TX device.
#define C_x_FREQ_OFFS_MULT_x_HZ2PPM_CH1 (318994)
#define C_x_FREQ_OFFS_MULT_x_HZ2PPM_CH2 (279120) // = (uint32_t)(FREQ_OFFSET_MULTIPLIER * HERTZ_TO_PPM_MULTIPLIER_CHAN_2 * SPEED_OF_LIGHT )
#define C_x_FREQ_OFFS_MULT_x_HZ2PPM_CH3 (248106)
#define C_x_FREQ_OFFS_MULT_x_HZ2PPM_CH5 (171766)

        /* Read carrier integrator value and calculate clock offset ratio */
        switch( app.pConfig->dwt_config.chan) // config.chan )
        {
            case 1:       c_hz2ppm_mult_chan = C_x_FREQ_OFFS_MULT_x_HZ2PPM_CH1;break;
            case 2:case 4:c_hz2ppm_mult_chan = C_x_FREQ_OFFS_MULT_x_HZ2PPM_CH2;break;
            case 3:       c_hz2ppm_mult_chan = C_x_FREQ_OFFS_MULT_x_HZ2PPM_CH3;break;
            case 5:case 7:c_hz2ppm_mult_chan = C_x_FREQ_OFFS_MULT_x_HZ2PPM_CH5;break;
            default:      c_hz2ppm_mult_chan = C_x_FREQ_OFFS_MULT_x_HZ2PPM_CH5;break;
        }
        if( app.pConfig->dwt_config.dataRate == DWT_BR_110K )
        {
            c_hz2ppm_mult_chan /= 8;
        }
        clk_offs = (int64_t)dwt_readcarrierintegrator() * c_hz2ppm_mult_chan;
        clk_offs_ppm_x100 = (int32_t)( ( clk_offs * 100 ) / (int64_t)SPEED_OF_LIGHT );

        dist_cm   = (uint64_t)1000000*(uint64_t)SPEED_OF_LIGHT*(uint64_t)rtd_init;
        dist_cm  -= (uint64_t)rtd_resp * ((uint64_t)1000000*(uint64_t)SPEED_OF_LIGHT + clk_offs );
        dist_cm /= 2;   // to get one single Time Of Flight
        dist_cm /= 499200000;
        dist_cm /= 128;
        dist_cm /= 10000; // if expressed in meters, would be 1000000 (i.e the scale factor used 5 lines above)

        get_ext_diag_info( &diagnostics, &rx_lvl );

        dw_printf("Dist_cm: %4d " , (uint32_t)dist_cm );
        dw_printf("RX: %4d.%2d " ,    rx_lvl/100 , (-rx_lvl)%100 );
        dw_printf("ClkPPM: %2d.%2d " , clk_offs_ppm_x100 / 100 , abs(clk_offs_ppm_x100) % 100 );
        dw_printf("SeqN: %3d\r\n" , (uint32_t)seq_number );
    }
    return;
}

uint8_t b_user_button = 0;

/**
* set the TX POW register 0x1E to its default recommended values as per table 20 of section 7.2.31 of User Manual
**/
const uint32_t txpow_STXP_DISabled_table20[ NUM_CH_SUPPORTED ][ NUM_PRF ] =
{
    // PRF 16MHz   PRF 64MHz
    { 0x00000000 , 0x00000000 }, // Chan '0' NOT RELEVANT
    { 0x75757575 , 0x67676767 }, // Chan 1
    { 0x75757575 , 0x67676767 }, // Chan 2
    { 0x6F6F6F6F , 0x8B8B8B8B }, // Chan 3
    { 0x5F5F5F5F , 0x9A9A9A9A }, // Chan 4
    { 0x48484848 , 0x85858585 }, // Chan 5
    { 0x00000000 , 0x00000000 }, // Chan '6' NOT RELEVANT
    { 0x92929292 , 0xD1D1D1D1 }  // Chan 7
};


/**
* set the pulse generator delay value - see User Manual - Table 40: Sub-Register 0x2A:0B – TC_PGDELAY recommended values
*/
const uint8_t pg_delay_table40[ NUM_CH_SUPPORTED ] =
{
    0,      // Not relevant
    TC_PGDELAY_CH1, // 0xC9,    Recommended value for Chan 1
    TC_PGDELAY_CH2, // 0xC2,    Recommended value for Chan 2
    TC_PGDELAY_CH3, // 0xC5,    Recommended value for Chan 3
    TC_PGDELAY_CH4, // 0x95,    Recommended value for Chan 4
    TC_PGDELAY_CH5, // 0xC0,    Recommended value for Chan 5
    0,      // Not relevant
    TC_PGDELAY_CH7, // 0x93     Recommended value for Chan 7
};

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* USER CODE BEGIN 1 */
    SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP | DBGMCU_CR_DBG_STOP | DBGMCU_CR_DBG_STANDBY );
    /* USER CODE END 1 */
    

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    /* System interrupt init*/

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    //  NVIC_SetPriority(SysTick_IRQn, TICK_INT_PRIORITY, 0);
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
                                         SysTick_CTRL_TICKINT_Msk |
                                         SysTick_CTRL_ENABLE_Msk;

    LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();
    MX_CRC_Init();
    MX_RTC_Init();
    /* USER CODE BEGIN 2 */

    lis3dh_power_ON();

    memset(&app,0,sizeof(app));

    /* reset DW1000 */
    reset_DW1000();

    /* set default PRF and bit rate etc.. */
    load_bssConfig();                 /**< load the RAM Configuration parameters from NVM block */

    /* Reset and initialise DW1000.
     * For initialisation, DW1000 clocks must be temporarily set to crystal speed. After initialisation SPI rate can be increased for optimum
     * performance. */
    reset_DW1000(); /* Target specific drive of RSTn line into DW1000 low for a period. */
    port_set_dw1000_slowrate();
    if (dwt_initialise(DWT_LOADUCODE) == DWT_ERROR)
    {
        dw_printf("INIT FAILED\n");
        while (1)
        { };
    }

    port_set_dw1000_fastrate();

    /* Configure DW1000. */
    dwt_configure(&(app.pConfig->dwt_config)); // &config);

    /* Configure TX power */
    dwt_write32bitreg(TX_POWER_ID, txpow_STXP_DISabled_table20[ app.pConfig->dwt_config.chan ][ app.pConfig->dwt_config.prf - 1 ] );
    // Configure RF TX PG_DELAY
    dwt_write8bitoffsetreg(TX_CAL_ID, TC_PGDELAY_OFFSET, pg_delay_table40[ app.pConfig->dwt_config.chan ] );

    /* The optimized configuration for 64 length preamble is achieved by calling dwt_configurefor64plen API function */
    if( app.pConfig->dwt_config.txPreambLength == DWT_PLEN_64 )
    {
        dwt_configurefor64plen(app.pConfig->dwt_config.prf);
    }
    // Configure LDE (NTM, PMULT register) optimization targeting LOS or NLOS environment
    set_los_nlos_config( app.pConfig->nlos );


    /* Apply default antenna delay value. */
    dwt_setrxantennadelay(RX_ANT_DLY);
    dwt_settxantennadelay(TX_ANT_DLY);

    if( app.pConfig->twr_role == TWR_INIT )
    {
        /* Set expected response's delay and timeout.
         * As this example only handles one incoming frame with always the same delay and timeout, those values can be set here once for all. */
      dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    }
    dwt_setrxtimeout(65000 /* RESP_RX_TIMEOUT_UUS */ );
    
    dwt_setleds(3);

    dw_printf("Starting\n");

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        if( b_user_button )
        {
            b_user_button = 0;
            dw_printf("User button pressed\n");
            if( app.pConfig->twr_role == TWR_INIT )
            {
                app.pConfig->twr_role = TWR_RESP;
                dw_printf("Switching from TWR_INIT to TWR_RESP\n");
                /* Set expected response's delay and timeout.
                 * As this example only handles one incoming frame with always the same delay and timeout, those values can be set here once for all. */
                dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
                dwt_setrxtimeout(65000 /* RESP_RX_TIMEOUT_UUS */ );
            } else
            {
                app.pConfig->twr_role = TWR_INIT;
                dw_printf("Switching from TWR_RESP to TWR_INIT\n");
            }
        }
        if( deca_uart_rx_data_ready() )
        {
            /* process UART msg based on user input. */
            process_uartmsg();
            LEDS_OFF(LED_BLUE_MASK);
        }
        if( app.pConfig->twr_role == TWR_INIT )
        {
            LEDS_ON(LED_RED_MASK);
            /* Write frame data to DW1000 and prepare transmission. */
            tx_poll_msg[ALL_MSG_SN_IDX] = frame_seq_nb;
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
            dwt_writetxdata(sizeof(tx_poll_msg), tx_poll_msg, 0); /* Zero offset in TX buffer. */
            dwt_writetxfctrl(sizeof(tx_poll_msg), 0, 1); /* Zero offset in TX buffer, ranging. */

            /* Start transmission, indicating that a response is expected so that reception is enabled automatically after the frame is sent and the delay
             * set by dwt_setrxaftertxdelay() has elapsed. */
            dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

            /* Increment frame sequence number after transmission of the poll message (modulo 256). */
            frame_seq_nb++;
        } else
        {
            LEDS_ON(LED_GREEN_MASK);
            /* Activate reception immediately. */
            dwt_rxenable(DWT_START_RX_IMMEDIATE);
        }

        /* We assume that the transmission is achieved correctly, poll for reception of a frame or error/timeout. */
        while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR)))
        { };

        if (status_reg & SYS_STATUS_RXFCG)
        {
            uint32 frame_len;

            /* Clear good RX frame event in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);

            /* A frame has been received, read it into the local buffer. */
            frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFLEN_MASK;
            if (frame_len <= RX_BUF_LEN)
            {
                            dwt_readrxdata(rx_buffer, frame_len, 0);
            }

            /* Check that the frame is the expected response from the companion "SS TWR responder" example.
             * As the sequence number field of the frame is not relevant, it is cleared to simplify the validation of the frame. */
                                                    seq_number = rx_buffer[ALL_MSG_SN_IDX];
            rx_buffer[ALL_MSG_SN_IDX] = 0;

            if( app.pConfig->twr_role == TWR_INIT )
            {
                    calc_tof_from_rcved_resp( rx_buffer , frame_len );
            } 
            else
            {
                    resp_to_poll( rx_buffer , frame_len );
            }
        }
        else
        {
            /* Clear RX error/timeout events in the DW1000 status register. */
            dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_TO | SYS_STATUS_ALL_RX_ERR);

            /* Reset RX to properly reinitialise LDE operation. */
            dwt_rxreset();
        }

        LEDS_OFF(LED_RED_MASK|LED_GREEN_MASK|LED_BLUE_MASK);
        if( app.pConfig->twr_role == TWR_INIT )
        {
            /* Execute a delay between ranging exchanges. */
            deca_sleep(app.pConfig->blink.interval_in_ms);// RNG_DELAY_MS);
        }
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
  {
  Error_Handler();  
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {
    
  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_LSI_Enable();

   /* Wait till LSI is ready */
  while(LL_RCC_LSI_IsReady() != 1)
  {
    
  }
  LL_PWR_EnableBkUpAccess();
  LL_RCC_ForceBackupDomainReset();
  LL_RCC_ReleaseBackupDomainReset();
  LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
  LL_RCC_EnableRTC();
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {
  
  }

  LL_Init1msTick(16000000);

  LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
  LL_SetSystemCoreClock(16000000);
  LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_HSI);
  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_PCLK1);
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* Peripheral clock enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);

  /* USER CODE BEGIN CRC_Init 1 */
  // TODO check compatibilty between CRC calculations in LL and HAL versions
  /* USER CODE END CRC_Init 1 */
  LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_NONE);
  LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_NONE);
  LL_CRC_SetPolynomialCoef(CRC, LL_CRC_DEFAULT_CRC32_POLY);
  LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_32B);
  LL_CRC_SetInitialData(CRC, LL_CRC_DEFAULT_CRC_INITVALUE);
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  LL_I2C_InitTypeDef I2C_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  /**I2C1 GPIO Configuration  
  PA9   ------> I2C1_SCL
  PA10   ------> I2C1_SDA 
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  /** I2C Initialization 
  */
  LL_I2C_EnableAutoEndMode(I2C1);
  LL_I2C_DisableOwnAddress2(I2C1);
  LL_I2C_DisableGeneralCall(I2C1);
  LL_I2C_EnableClockStretching(I2C1);
  I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
  I2C_InitStruct.Timing = 0x0010061A;
  I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
  I2C_InitStruct.DigitalFilter = 0;
  I2C_InitStruct.OwnAddress1 = 0;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  LL_I2C_Init(I2C1, &I2C_InitStruct);
  LL_I2C_SetOwnAddress2(I2C1, 0, LL_I2C_OWNADDRESS2_NOMASK);
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  LL_RTC_InitTypeDef RTC_InitStruct = {0};

  /* Peripheral clock enable */
  LL_RCC_EnableRTC();

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC and set the Time and Date 
  */
  RTC_InitStruct.HourFormat = LL_RTC_HOURFORMAT_24HOUR;
  RTC_InitStruct.AsynchPrescaler = 127;
  RTC_InitStruct.SynchPrescaler = 255;
  LL_RTC_Init(RTC, &RTC_InitStruct);
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  LL_SPI_InitTypeDef SPI_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);
  
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  /**SPI1 GPIO Configuration  
  PA5   ------> SPI1_SCK
  PA6   ------> SPI1_MISO
  PA7   ------> SPI1_MOSI 
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
  SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
  SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
  SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
  SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
  SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV16;
  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
  SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
  SPI_InitStruct.CRCPoly = 7;
  LL_SPI_Init(SPI1, &SPI_InitStruct);
  LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
  /* USER CODE BEGIN SPI1_Init 2 */

  LL_SPI_Enable(SPI1);

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_6);
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_7);

  /* USER CODE END USART2_Init 0 */

  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
  
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
  /**USART2 GPIO Configuration  
  PB6   ------> USART2_TX
  PB7   ------> USART2_RX 
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USART2 interrupt Init */
  NVIC_SetPriority(USART2_IRQn, 0);
  NVIC_EnableIRQ(USART2_IRQn);

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  USART_InitStruct.BaudRate = 38400;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_8;
  LL_USART_Init(USART2, &USART_InitStruct);
  LL_USART_DisableOverrunDetect(USART2);
  LL_USART_ConfigAsyncMode(USART2);
  LL_USART_Enable(USART2);
  /* USER CODE BEGIN USART2_Init 2 */

  uint32_t WakeUpType = LL_USART_GetWKUPType(USART2);
  if ( WakeUpType != USART_CR3_WUS_1 ) {
      LL_USART_Disable(USART2);
      MODIFY_REG(USART2->CR3, USART_CR3_WUS, USART_CR3_WUS_1);
      LL_USART_Enable(USART2);
//      LEDS_INVERT(LED_RED1_MASK);
  }

  LL_USART_EnableIT_PE( USART2 );
  LL_USART_EnableIT_ERROR( USART2 );
  LL_USART_EnableIT_RXNE(USART2);

    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_6);
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_7);

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

  /**/
  LL_GPIO_SetOutputPin(LED_Blue_GPIO_Port, LED_Blue_Pin);

  /**/
  LL_GPIO_SetOutputPin(LIS3DH_PWR_GPIO_Port, LIS3DH_PWR_Pin);

  /**/
  LL_GPIO_SetOutputPin(DW_CS_GPIO_Port, DW_CS_Pin);

  /**/
  LL_GPIO_SetOutputPin(LED_Red_GPIO_Port, LED_Red_Pin);

  /**/
  LL_GPIO_SetOutputPin(LIS3DH_CS_GPIO_Port, LIS3DH_CS_Pin);

  /**/
  LL_GPIO_SetOutputPin(LED_Green_GPIO_Port, LED_Green_Pin);

  /**/
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE0);

  /**/
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE2);

  /**/
  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE3);

  /**/
  LL_GPIO_SetPinPull(WakeUpBtn_GPIO_Port, WakeUpBtn_Pin, LL_GPIO_PULL_UP);

  /**/
  LL_GPIO_SetPinPull(LIS3DH_IRQ_GPIO_Port, LIS3DH_IRQ_Pin, LL_GPIO_PULL_NO);

  /**/
  LL_GPIO_SetPinPull(DW_IRQ_GPIO_Port, DW_IRQ_Pin, LL_GPIO_PULL_NO);

  /**/
  LL_GPIO_SetPinMode(WakeUpBtn_GPIO_Port, WakeUpBtn_Pin, LL_GPIO_MODE_INPUT);

  /**/
  LL_GPIO_SetPinMode(LIS3DH_IRQ_GPIO_Port, LIS3DH_IRQ_Pin, LL_GPIO_MODE_INPUT);

  /**/
  LL_GPIO_SetPinMode(DW_IRQ_GPIO_Port, DW_IRQ_Pin, LL_GPIO_MODE_INPUT);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_0;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_2;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /**/
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_3;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
  LL_EXTI_Init(&EXTI_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LED_Blue_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LED_Blue_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LIS3DH_PWR_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LIS3DH_PWR_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = DW_CS_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(DW_CS_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = DW_RST_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(DW_RST_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LED_Red_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LED_Red_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LIS3DH_CS_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LIS3DH_CS_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = LED_Green_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LED_Green_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = M_PIN17_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(M_PIN17_GPIO_Port, &GPIO_InitStruct);

  /**/
  GPIO_InitStruct.Pin = M_PIN16_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(M_PIN16_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  NVIC_SetPriority(EXTI0_1_IRQn, 0);
  NVIC_EnableIRQ(EXTI0_1_IRQn);
  NVIC_SetPriority(EXTI2_3_IRQn, 0);
  NVIC_EnableIRQ(EXTI2_3_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
