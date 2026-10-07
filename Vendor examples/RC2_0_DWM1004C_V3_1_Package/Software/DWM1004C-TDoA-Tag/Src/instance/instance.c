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
#include "tvc.h"

/*******************************************************************************
 * Experimental values in ms of time to restart blink
 **/
/* Observed that timer interrupt is not serviced for the default value of
LOWPOWER_RESTART_TIME. So LOWPOWER_RESTART_TIME is configured as 15 */
#define LOWPOWER_RESTART_TIME           15

#define DWT_SIG_RX_TIMEOUT              4

#define DWT_SIG_RX_OKAY 				1
#define DWT_SIG_TX_DONE 				2
#define DWT_SIG_RX_ERROR 				3


/* DW1000 device variables */
//static dwt_txconfig_t tx_cfg;
static ref_values_t ref = {0};

static bool rx_armed = false;

enum inst_states
{
   TA_INIT,
   TA_TX_POLL, 		// initiator sends the poll
   TA_WAIT_RESP, 	// initiator waits for the response
   TA_TX_FINAL, 	// initiator sends the delayed final
   TA_WAIT_POLL,	// responder waits for a poll
   TA_TX_RESP, 		// responder sends the delayed response
   TA_WAIT_FINAL	// responder waits for the final
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

instance_data_t instance_data = {
	.poll_msg  = {.ctrl1=0x41, .ctrl2=0x88, .PAN_id[0]=0xAB, .PAN_id[1]=0xCD},
	.resp_msg  = {.ctrl1=0x41, .ctrl2=0x88, .PAN_id[0]=0xAB, .PAN_id[1]=0xCD},
	.final_msg = {.ctrl1=0x41, .ctrl2=0x88, .PAN_id[0]=0xAB, .PAN_id[1]=0xCD},
																				};

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
int testapprun_int(instance_data_t *inst, int message)
{
//	param_block_t * pbss = get_pbssConfig();

    switch (inst->testAppState)
    {
        case TA_INIT :

        	inst->testAppState = TA_TX_POLL;

            break;

        case TA_TX_POLL:

        	inst->poll_msg.seq_num = inst->frame_sn++;
        	inst->poll_msg.dst[0] = 2;
        	inst->poll_msg.dst[1] = 0;
        	inst->poll_msg.src[0] = 1;
        	inst->poll_msg.src[1] = 0;
        	inst->poll_msg.fc = FC_POLL;

        	dwt_writetxdata(sizeof( inst->poll_msg), (uint8_t *)&inst->poll_msg, 0);
        	dwt_writetxfctrl(sizeof(inst->poll_msg), 0, 1);
        	dwt_setrxaftertxdelay(300);
        	dwt_setrxtimeout(5000);
        	dwt_starttx(DWT_START_TX_IMMEDIATE | DWT_RESPONSE_EXPECTED);

            inst->testAppState = TA_WAIT_RESP;
        	inst->done = 1;

            break;

        case TA_WAIT_RESP:

            /* Do temperature and voltage compensation and get the values of tx_cfg */
            /* the right way:
             * read T and V
             * check the difference wrt previous T and V
             * if greater than (in counts, dont need degrees and volts) - do TVC, store the new values
             */
//            tvc_comp(&tx_cfg, &ref, pbss->dwt_config.chan);

            /* Configure tx power with new values for temperature and voltage compensation */
//            dwt_configuretxrf(&tx_cfg);

            // write the frame data
//            dwt_writetxdata(length, (uint8 *)  (&inst->msg), 0) ;
//            dwt_writetxfctrl(length, 0, 0);

//            dwt_starttx(DWT_START_TX_IMMEDIATE); //always using immediate TX

        	if(message == 0){

        		inst->testAppState = TA_WAIT_RESP;
        		inst->done = 1;
        		break;
        	}
        	else if(message == DWT_SIG_RX_OKAY){

        		// parce rx_buffer
        		Ranging_Frame * rf = (Ranging_Frame *)instance_data.rx_buffer;

        		if(rf->fc==FC_RESPONSE && rf->dst[0]==1 && rf->dst[1]==0 && rf->src[0]==2 && rf->src[1]==0)
        		{
        			//response ok
        			uint8_t buf[5];
        			dwt_readtxtimestamp(buf);
        			inst->poll_tx = 0;
        			for(int i=0; i < 5; ++i) {
        				inst->poll_tx += (uint64_t)buf[i] << 8 * i;
        				inst->final_msg.timestamps[i] = buf[i];
        			}

        			dwt_readrxtimestamp(buf);
        			inst->resp_rx = 0;
        			for(int i=0; i < 5; ++i){
        				inst->resp_rx += (uint64_t)buf[i] << 8 * i;
        				inst->final_msg.timestamps[i + 5] = buf[i];
        			}

        			uint64_t final_tx_time = inst->resp_rx + (3000ULL * 65536ULL);
        			uint32_t delayed32 = (final_tx_time >> 8) & 0xFFFFFFFEUL;
        			dwt_setdelayedtrxtime(delayed32);
        			inst->final_tx = ((uint64_t)delayed32 << 8) + 0;   /* antenna delay stays 0 until step 6 */

        			inst->final_msg.seq_num = inst->frame_sn++;
        			inst->final_msg.dst[0] = 2; inst->final_msg.dst[1] = 0;
        			inst->final_msg.src[0] = 1; inst->final_msg.src[1] = 0;
        			inst->final_msg.fc = FC_FINAL;

        			for(int i=0; i < 5; ++i){
        				inst->final_msg.timestamps[i + 10] = ( inst->final_tx >> 8 * i ) & 0xFF;
        			}

        			inst->testAppState = TA_TX_FINAL;
        		}
        		else
        		{
        			//response error
        			inst->testAppState = TA_TX_POLL;
        			inst->done = 1;
        		}
        	}
        	else if(message == DWT_SIG_RX_TIMEOUT || message == DWT_SIG_RX_ERROR){

        		inst->testAppState = TA_TX_POLL;
        		inst->done = 1;
        	}

        	inst->done = 1;

            break;

        case TA_TX_FINAL:

        	dwt_writetxdata(sizeof(inst->final_msg), (uint8_t *)&inst->final_msg, 0);
        	dwt_writetxfctrl(sizeof(inst->final_msg), 0, 1);
        	int status =
        			dwt_starttx(DWT_START_TX_DELAYED);
        	if(status == DWT_SUCCESS){
        		uint32_t start = portGetTickCount();
        		while(portGetTickCount() - start < 100) ;
        	}

        		inst->testAppState = TA_TX_POLL;
        		inst->done = 1;

        	break;

        default:
            break;
    }

    return inst->done;
}

int testapprun_rsp(instance_data_t *inst, int message)
{
	switch (inst->testAppState)
	{
	case TA_INIT :

		inst->testAppState = TA_WAIT_POLL;

		break;

	case TA_WAIT_POLL:

		if(message == 0){
			if(rx_armed == false){
				dwt_setrxtimeout(0);
				dwt_rxenable(DWT_START_RX_IMMEDIATE);
				rx_armed = 1;
			}

			inst->done = 1;
		}
		else if(message == DWT_SIG_RX_OKAY){

			// parce rx_buffer
			Ranging_Frame * rf = (Ranging_Frame*)instance_data.rx_buffer;

			if(rf->fc==FC_POLL && rf->dst[0]==2 && rf->dst[1]==0 && rf->src[0]==1 && rf->src[1]==0)
			{
				uint8_t buf[5];
				dwt_readrxtimestamp(buf);
				inst->poll_rx = 0;
				for(int i=0; i < 5; ++i){
					inst->poll_rx += (uint64_t)buf[i] << 8 * i;
				}

				uint64_t resp_tx_time = inst->poll_rx + (3000 * 65536);
				uint32_t delayed32    = (resp_tx_time >> 8) & 0xFFFFFFFE;
				dwt_setdelayedtrxtime(delayed32);

				inst->resp_msg.seq_num = inst->frame_sn++;
				inst->resp_msg.dst[0] = 1; inst->resp_msg.dst[1] = 0;
				inst->resp_msg.src[0] = 2; inst->resp_msg.src[1] = 0;
				inst->resp_msg.fc = FC_RESPONSE;

				inst->testAppState = TA_TX_RESP;
				inst->done = 1;
			}
			else
			{
				rx_armed = false;
			}
		}
		else if(message == DWT_SIG_RX_TIMEOUT || message == DWT_SIG_RX_ERROR){
			rx_armed = false;
			inst->done = 1;
		}

		break;

	case TA_TX_RESP:

		dwt_setrxaftertxdelay(300);
		dwt_setrxtimeout(5000);
		dwt_writetxdata(sizeof(inst->resp_msg), (uint8 *)&inst->resp_msg, 0);
		dwt_writetxfctrl(sizeof(inst->resp_msg), 0, 1);

		int status =
		dwt_starttx(DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED);
		if(status == DWT_SUCCESS){
			inst->testAppState = TA_WAIT_FINAL;
		}
		else{
			rx_armed = 0;
			inst->testAppState = TA_WAIT_POLL;
		}

		inst->done = 1;

		break;

    case TA_WAIT_FINAL:

    	if(message == 0){
    		inst->done = 1;
    	}
    	else if(message == DWT_SIG_RX_OKAY){

			// parce rx_buffer
			Ranging_Frame_Final * rff = (Ranging_Frame_Final *)instance_data.rx_buffer;

			if(rff->fc==FC_FINAL && rff->dst[0]==2 && rff->dst[1]==0 && rff->src[0]==1 && rff->src[1]==0)
			{
				uint8_t buf[5];

				dwt_readtxtimestamp(buf);
				inst->resp_tx = 0;
				for(int i=0; i < 5; ++i){
					inst->resp_tx += (uint64_t)buf[i] << 8 * i;
				}
				dwt_readrxtimestamp(buf);
				inst->final_rx = 0;
				for(int i=0; i < 5; ++i){
					inst->final_rx += (uint64_t)buf[i] << 8 * i;
				}

				inst->poll_tx = 0; inst->resp_rx = 0; inst->final_tx = 0;
				for (int i = 0; i < 5; i++) {
					inst->poll_tx  |= (uint64_t)rff->timestamps[i]      << (8 * i);
					inst->resp_rx  |= (uint64_t)rff->timestamps[i + 5]  << (8 * i);
					inst->final_tx |= (uint64_t)rff->timestamps[i + 10] << (8 * i);
				}

				int64_t Ra = (int64_t)((inst->resp_rx  - inst->poll_tx) & TS_MASK);
				int64_t Da = (int64_t)((inst->final_tx - inst->resp_rx) & TS_MASK);
				int64_t Rb = (int64_t)((inst->final_rx - inst->resp_tx) & TS_MASK);
				int64_t Db = (int64_t)((inst->resp_tx  - inst->poll_rx) & TS_MASK);

				int64_t tof = (Ra * Rb - Da * Db) / (Ra + Rb + Da + Db);
				int64_t distance_mm = tof * 299702547 / 63897600; (void)distance_mm;

				rx_armed = 0;
				inst->testAppState = TA_WAIT_POLL;
				inst->done = 1;
			}
			else
			{
				inst->testAppState = TA_WAIT_POLL;
				rx_armed = false;
			}

    	}
    	else if(message == DWT_SIG_RX_TIMEOUT || message == DWT_SIG_RX_ERROR){
    		rx_armed = false;
    		inst->testAppState = TA_WAIT_POLL;
    		inst->done = 1;
    	}


    	break;

	default:

		break;

	}


	return inst->done;
}



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

    /* Read reference values from OTP for temperature and voltage compensation */
    if((ref.power == 0) && (ref.pgcnt == 0) && (ref.temp == 0) && (ref.pgdly == 0))
    {
      tvc_otp_read_txcfgref(&ref, pbss->dwt_config.chan);
    }

    dwt_entersleepaftertx(0);

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

	instance_data.poll_tx = 0;
	instance_data.poll_rx = 0;
	instance_data.resp_tx = 0;
	instance_data.resp_rx = 0;
	instance_data.final_tx = 0;
	instance_data.final_rx = 0;

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

    configTx.PGdly = ref.pgdly;
    configTx.power = ref.power;

    /* smart power is always used*/
    if(pbss->smartPowerEn == 0)
    {
        /* when smart TX power is not used then the low byte should be copied
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
		#if TWR_ROLE == TWR_INT
    	done = testapprun_int(&instance_data, message) ;
		#elif TWR_ROLE == TWR_RSP
    	done = testapprun_rsp(&instance_data, message) ;
		#endif


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


    // below code is never calling. If it will be used, low_power() function calls
    // need to be replaced, because STM32 should not sleep

    /* we have sent the message and in sleep and need to timeout (Tag needs to send another blink after some time) */
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

