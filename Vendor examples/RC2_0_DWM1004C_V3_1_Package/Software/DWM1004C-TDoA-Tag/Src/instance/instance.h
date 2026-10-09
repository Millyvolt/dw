/*
 * @file       instance.h
 *
 * @brief      DecaWave header for application level instance
 *
 * @author     Decawave
 *
 * @attention  Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *             All rights reserved.
 *
 */
 
#ifndef _INSTANCE_H_
#define _INSTANCE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "default_config.h"
#include "deca_types.h"
#include "deca_device_api.h"
#include "pckt_ieee.h"



#define TWR_INT	0
#define TWR_RSP	1

#define TWR_ROLE	TWR_INT
//#define TWR_ROLE	TWR_RSP

#define TS_MASK		0xFFFFFFFFFFULL		// select 40 bits in 64 bits word


typedef struct
{
    param_block_t  	*pConfig;
    uint32_t      	blinkenable;
    uint32_t		*pcurrent_blink_interval_ms;
    uint8_t      	uartRx;
}app_cfg_t;

extern app_cfg_t app;


/*******************************************************************************
 ********************** NOTES on DW (MP) features/options **********************
 ******************************************************************************/

#define DWT_PRF_64M_RFDLY   (514.462f)
#define DWT_PRF_16M_RFDLY   (513.907f)

#define MAXIMUM_LED_ON_TIME   (50)

typedef struct
{
    int testAppState ;
    int done ;

    //message structures used for transmitted messages
    // 802.15.4  Minimum IEEE ID blink
    iso_IEEE_EUI64_blink_msg msg ;
    uint16        psduLength ;
    uint8        frame_sn;
    uint8        event[2];
    uint8        eventCnt;
    uint8        timeron;
    uint32        timeout;
    uint16_t chipSleep; // The DW1000 sleep duration

    uint64_t poll_tx;
    uint64_t poll_rx;
    uint64_t resp_tx;
    uint64_t resp_rx;
    uint64_t final_tx;
    uint64_t final_rx;

    Ranging_Frame poll_msg;
    Ranging_Frame resp_msg;
    Ranging_Frame_Final final_msg;

    uint8_t rx_buffer[27];
} instance_data_t ;

/* Exported functions prototypes */

/* Set Newchip to 1 to inform instance Tag TDoA that DW1000 has no OTP calibrated
   values */
int instance_init( int sleep_enable) ;

// Call init, then call config, then call run.
void instance_config(param_block_t *config) ;

// returns indication of status report change
int instance_run(void) ;

int testapprun_int(instance_data_t *inst, int message);
int testapprun_rsp(instance_data_t *inst, int message);
uint32 ulong2littleEndian(uint32);
void instancesettagaddress(instance_data_t *inst);
void instance_txcallback(const dwt_cb_data_t *txd);
void instance_rxgood(const dwt_cb_data_t *rxd);
void instance_rxtimeout(const dwt_cb_data_t *rxd);
void instance_rxerror(const dwt_cb_data_t *rxd);

extern int dw_ieee_payload(uint8 dwh, uint8 dwp, uint8 * buf);
void process_uartmsg(void);

#ifdef __cplusplus
}
#endif

#endif
