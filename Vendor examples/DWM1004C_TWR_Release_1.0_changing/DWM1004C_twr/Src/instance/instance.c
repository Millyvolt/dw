/*
 * @file       instance.c
 *
 * @brief      Application level message exchange for ranging demo
 *
 * @author     Decawave Software
 *
 * @attention  Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *             All rights reserved.
 */

#include <stdlib.h>
#include "port_platform.h"
#include "deca_regs.h"
#include "deca_device_api.h"
#include "instance.h"
#include "config.h"

/*******************************************************************************
 * Experimental values in ms of time to restart blink
 **/
/* Observed that timer interrupt is not serviced for the default value of
LOWPOWER_RESTART_TIME. So LOWPOWER_RESTART_TIME is configured as 15 */
#define LOWPOWER_RESTART_TIME           15

#define DWT_SIG_RX_TIMEOUT              4

enum inst_states
{
   TA_INIT,
   TA_SLEEP_DONE,
   TA_TXBLINK_WAIT_SEND
};

typedef struct {
   uint8 PG_DELAY;
   //TX POWER
   //31:24                BOOST_0.125ms_PWR
   //23:16                BOOST_0.25ms_PWR-TX_SHR_PWR
   //15:8                BOOST_0.5ms_PWR-TX_PHR_PWR
   //7:0                DEFAULT_PWR-TX_DATA_PWR
   uint32 tx_pwr[2]; //
}tx_struct;

/*The table below specifies the default TX spectrum configuration parameters...
  this has been tuned for DW EVK hardware units
  the table is set for smart power - see below in the instance_config function
  how this is used when not using smart power
*/
const tx_struct txSpectrumConfig[8] =
{
    //Channel 0 -----
    //this is just a place holder so the next array element is channel 1
    {
        0x0,   //0
        {
            0x0, //0
            0x0 //0
        }
    },
    //Channel 1
    {
        0xc9,   //PG_DELAY
        {
            0x15355575, //16M prf power
            0x07274767 //64M prf power
        }

    },
    //Channel 2
    {
        0xc2,   //PG_DELAY
        {
            0x15355575, //16M prf power
            0x07274767 //64M prf power
        }
    },
    //Channel 3
    {
        0xc5,   //PG_DELAY
        {
            0x0f2f4f6f, //16M prf power
            0x2b4b6b8b //64M prf power
        }
    },
    //Channel 4
    {
        0x95,   //PG_DELAY
        {
            0x1f1f3f5f, //16M prf power
            0x3a5a7a9a //64M prf power
        }
    },
    //Channel 5
    {
        0xc0,   //PG_DELAY
        {
            0x0E082848, //16M prf power
            0x25456585 //64M prf power
        }
    },
    //Channel 6 -----
    //this is just a place holder so the next array element is channel 7
    {
        0x0,   //0
        {
            0x0, //0
            0x0 //0
        }
    },
    //Channel 7
    {
        0x93,   //PG_DELAY
        {
            0x32527292, //16M prf power
            0x5171B1d1 //64M prf power
        }
    }
};

const uint16 rfDelays[2] = {
    (uint16) ((DWT_PRF_16M_RFDLY/ 2.0f) * (float)1e-9 / DWT_TIME_UNITS),//PRF 16
    (uint16) ((DWT_PRF_64M_RFDLY/ 2.0f) * (float)1e-9 / DWT_TIME_UNITS)
};
// -----------------------------------------------------------------------------

instance_data_t instance_data ;

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

/*
 * @fn   instancesettagaddress
 * @param  *inst
 *
 * */
void instancesettagaddress(instance_data_t *inst)
{
    uint8 eui64[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xCA, 0xDE};
    
    param_block_t * pbss = get_pbssConfig();

    if(pbss->tagIDset == 0)
    {
        //we want exact Address representation in Little- and Big- endian
        uint32 id= ulong2littleEndian(dwt_getpartid());
        memcpy(eui64, &id, sizeof(uint32));

        //set source address into the message structure
        memcpy(inst->msg.tagID, eui64, ADDR_BYTE_SIZE);
        memcpy(pbss->tagID, eui64, ADDR_BYTE_SIZE);
    }
    else
    {
        //set source address into the message structure
        memcpy(inst->msg.tagID, pbss->tagID, ADDR_BYTE_SIZE);
    }
}

/*
 * @fn  testapprun
 * @param *inst
 * @param message
 * @brief the main instance state machine (only supports TDoA Tag function)
 *
 * */
int testapprun(instance_data_t *inst, int message)
{
    switch (inst->testAppState)
    {
        case TA_INIT :
        {
            instancesettagaddress(inst);

            //configure the on wake parameters (upload the IC config settings)
            dwt_configuresleep(AON_WCFG_ONW_RADC | DWT_PRESRV_SLEEP| DWT_CONFIG,
                               DWT_WAKE_CS|DWT_SLP_EN);

            /* change to next state - send a Blink message */
            /* instance is configured to send TDOA tag blinks */
            inst->testAppState = TA_TXBLINK_WAIT_SEND;
            break; // end case TA_INIT
        }

        case TA_SLEEP_DONE :
        {
            if(message != DWT_SIG_RX_TIMEOUT)
            {
                inst->done = 1;
                break;
            }

            // when there is no ext. power, system waking up in lowpower.c
            if (DWT_DEVICE_ID == dwt_readdevid() )
            {
                inst->done = 0;
                inst->testAppState = TA_TXBLINK_WAIT_SEND;

                break;//TA_SLEEP_DONE
            }

            /* if DW1000 did not waked up while MCU started up from lowpower,
             * then we need to perform the "slow wake up" */
            port_wakeup_dw1000();

            inst->done = 0;
            inst->testAppState = TA_TXBLINK_WAIT_SEND;

            break; //TA_SLEEP_DONE
        }

        case TA_TXBLINK_WAIT_SEND :
        {
            int length;
            //blink frames with IEEE EUI-64 tag with variable dwp/dwh set
            inst->msg.frameCtrl = FCS_EUI_64 ;
            inst->msg.seqNum = inst->frame_sn++;

            length = (FRAME_CRTL_AND_ADDRESS + FRAME_CRC);

            // write the frame data
            dwt_writetxdata(length, (uint8 *)  (&inst->msg), 0) ;
            dwt_writetxfctrl(length, 0, 0);

            dwt_starttx(DWT_START_TX_IMMEDIATE); //always using immediate TX

            inst->done = 2; //don't sleep here but kick off the TagTimeoutTimer
            inst->testAppState = TA_SLEEP_DONE;

            break; //TA_TXBLINK_WAIT_SEND
        }

        default:
            break;
    } // end switch on testAppState

    return inst->done;
} // end testapprun()

// -----------------------------------------------------------------------------

/* @fn  function to initialise instance structures
 * @brief  Returns 0 on success and -1 on error
 * */
int instance_init(int sleep_enable)
{
    int result ;
    param_block_t * pbss = get_pbssConfig();

    instance_data.testAppState = TA_INIT ;

    // Reset the IC (might be needed if not getting here from POWER ON)
    dwt_softreset();

    result = dwt_initialise( DWT_READ_OTP_PID ) ;

    if (sleep_enable) {
        //configure the on wake parameters (upload the IC config settings)
        dwt_configuresleep(AON_WCFG_ONW_LLDE | DWT_PRESRV_SLEEP|DWT_CONFIG ,
                           DWT_WAKE_CS|DWT_SLP_EN);

    }else{
        //configure the on wake parameters (upload the IC config settings)
        dwt_configuresleep(AON_WCFG_ONW_LLDE | DWT_PRESRV_SLEEP|DWT_CONFIG ,
                           DWT_WAKE_CS);
    }
    dwt_setleds(3);

    // this will set 0 if zero and 1 otherwise
    dwt_setsmarttxpower( (pbss->smartPowerEn != 0) );

    dwt_entersleepaftertx(1);

    if (DWT_SUCCESS != result)
    {
        return (DWT_ERROR) ;        // device initialize has failed
    }


    dwt_setcallbacks(instance_txcallback,
                     instance_rxgood,
                     instance_rxtimeout,
                     instance_rxerror);

    instance_data.frame_sn = 0;
    instance_data.timeron = 0;
    instance_data.event[0] = 0;
    instance_data.event[1] = 0;
    instance_data.eventCnt = 0;

    return 0 ;
}


/**
 * @fn  instance_config
 * @brief function to allow application configuration be passed into instance
 *        and affect underlying device operation
 *
 * */
void instance_config(param_block_t * pbss)
{
    dwt_txconfig_t  configTx ;

    dwt_configure(&pbss->dwt_config) ;

    configTx.PGdly = txSpectrumConfig[pbss->dwt_config.chan].PG_DELAY ;
    configTx.power = txSpectrumConfig[pbss->dwt_config.chan].tx_pwr[pbss->dwt_config.prf \
                     - DWT_PRF_16M];

    /* smart power is always used*/
    if(pbss->smartPowerEn == 0)
    {
        /* when smart TX power is not used then the low nibble should be copied
           into the whole 32 bits*/
        uint32 pow = configTx.power & 0xff;
        configTx.power = (pow << 24) + (pow << 16) + (pow << 8) + pow;
    }
    dwt_configuretxrf(&configTx);
}
/**
 * @fn  instance_txcallback
 * @brief  Tx callback (if TX irq present)
 *
 * */
void instance_txcallback(const dwt_cb_data_t *txd)
{
    //empty function
}

/**
 * @fn  instance_rxgood
 * @brief The TDoA Tag does not have any RX functionality
 *
 * */
void instance_rxgood(const dwt_cb_data_t *rxd)
{
   //empty function
}

/**
 * @fn  instance_rxtimeout
 * @brief The TDoA Tag does not have any RX functionality
 *
 * */
void instance_rxtimeout(const dwt_cb_data_t *rxd)
{
    //empty function
}

/**
 * @fn  instance_rxerror
 * @brief  The TDoA Tag does not have any RX functionality
 *
 * */
void instance_rxerror(const dwt_cb_data_t *rxd)
{
    //empty function
}

/**
 * @fn   check_device_id
 * @brief Read decawave device id.it's proper then return 0 otherwise
 *        return -1.
 * */
int check_device_id(void)
{
    uint32_t DEV_ID=0;

    // Set SPI clock to 2MHz
    port_set_dw1000_slowrate();
    // Read Decawave chip ID
    DEV_ID = dwt_readdevid();

    if(DWT_DEVICE_ID != DEV_ID)
    {
        //wake up device from low power mode
        //NOTE - in the ARM  code just drop chip select for 200us
        port_wakeup_dw1000();
        // SPI not working or Unsupported Device ID
        DEV_ID = dwt_readdevid() ;
    }

    dwt_setleds(3);

    // Set SPI to 16MHz clock
    port_set_dw1000_fastrate();
    // Read Decawave chip ID
    DEV_ID = dwt_readdevid() ;

    if (DWT_DEVICE_ID != DEV_ID)   // Means it is NOT MP device
    {
        // SPI not working or Unsupported Device ID
        return -1;
    }
    return 0;
}

/**
 * @fn  instance_run
 * @brief
 *
 * */
int instance_run(void)
{
    int done = instance_data.done = 0;
    int message = instance_data.event[0];
    int delay;

    param_block_t *pbss = get_pbssConfig();

    while(!done)
    {
        // run the communications application
        done = testapprun(&instance_data, message) ;

        if(message) // there was an event in the buffer
        {
            instance_data.event[0] = 0; //clear the buffer
            instance_data.eventCnt--;
        }

        //we've processed message
        message = 0;

        if(done)//ready for next event
        {
            // there was an event in the buffer
            if(instance_data.event[1])
            {
                message = instance_data.event[1];
                instance_data.event[1] = 0; //clear the buffer
                instance_data.eventCnt--;
                instance_data.done = done = 0; //wait for next done
            }
        }

    }

    /* we have sent the message and in sleep and need to timeout (Tag needs to
       send another blink after some time) */
    if(done == 2)
    {

        uint32_t currentInterval = *(app.pcurrent_blink_interval_ms);
        uint32_t currentRand = pbss->blink.randomness;

        /* randomness in % of blink pause time */ 
        // min rand 1% max randomisation 50%
        currentRand = (currentRand < 1)?(1):((currentRand > 50)?(50):currentRand);

        delay = currentInterval * ( rand()%(currentRand*2) )/100;
        delay = (currentInterval + (currentInterval * currentRand/100)) \
                - delay - LOWPOWER_RESTART_TIME ;

        if(delay > 0)
        {
          // if delay is greater then twice a maximum blink time - emit a short blink
          // if delay is less then that timeperiod - invert the led
          if ( delay < MAXIMUM_LED_ON_TIME * 2 ) {
              LEDS_INVERT(LED_BLUE_MASK);
              low_power(delay);
              if(check_device_id() != 0) {
                  // Read device id after Low_Power mode.
                  return -1;
              }
          }else{
              // just to be sure that the led is off almost all the time
              LEDS_OFF(LED_BLUE_MASK);
              low_power( delay - MAXIMUM_LED_ON_TIME );

              LEDS_ON(LED_BLUE_MASK);
              low_power( MAXIMUM_LED_ON_TIME );
              LEDS_OFF(LED_BLUE_MASK);
              if(check_device_id() != 0) { // Read device id after Low_Power mode.
                  return -1;
              }
          }
        }

        /* immediate wakeup after lowpower sleep */ 
        instance_data.timeout = portGetTickCount();
        instance_data.timeron = 1;
        instance_data.done = 0;
    }

    if(instance_data.timeron == 1)
    {
        if(instance_data.timeout <= portGetTickCount())
        {
            instance_data.timeron = 0;
            instance_data.event[instance_data.eventCnt++] \
                 = DWT_SIG_RX_TIMEOUT;
        }
    }
    return 0;
}

/*****************************************************************************
 *  @fn          ulong2littleEndian
 *  @brief  convert input value uint32 to little-endian order
 *                  assumes only little-endian and big-endian presents.
 *                  (not support middle-endian)
 */
uint32 ulong2littleEndian(uint32 prm)
{
    union {
      uint32  ui;
      uint8   uc[sizeof(uint32)];
    } v;

    v.ui = 1;        //check endian
    if(v.uc[0] == 1) {
        v.ui=prm;        //little
    } else {
        v.ui=prm;        //
        SWAP(v.uc[0], v.uc[3]);
        SWAP(v.uc[1], v.uc[2]);
    }
    return v.ui;
}

