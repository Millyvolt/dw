/*
 * @file       tvc.c
 *
 * @brief      temperature and voltage compensation utilities
 *
 * @author     Decawave Software
 *
 * @attention  Copyright 2019 (c) DecaWave Ltd.
 *             All rights reserved.
 */

#include "port_platform.h"
#include "deca_device_api.h"
#include "deca_regs.h"
#include "deca_types.h"
#include "deca_param_types.h"
#include "tvc.h"
#include "config.h"

uint32 tvc_calcpowertempvbatadj(uint8 channel, uint32 ref_powerreg, int delta_temp, int delta_voltage);

const uint8 tx_gain_lookup[62] = {
                                     // DA 0.0dB
                                     (0b110 << 5) + 0,          // gain  +0.0dB  0
                                     (0b110 << 5) + 1,          // gain  +0.5dB  1
                                     (0b110 << 5) + 2,          // gain  +1.0dB  2
                                     (0b110 << 5) + 3,          // gain  +1.5dB  3
                                     (0b110 << 5) + 4,          // gain  +2.0dB  4
                                     (0b110 << 5) + 5,          // gain  +2.5dB  5
                                     (0b110 << 5) + 6,          // gain  +3.0dB  6
                                     (0b110 << 5) + 7,          // gain  +3.5dB  7
                                     (0b110 << 5) + 8,          // gain  +4.0dB  8
                                     (0b110 << 5) + 9,          // gain  +4.5dB  9
                                     (0b110 << 5) + 10,         // gain  +5.0dB  10
                                     (0b110 << 5) + 11,         // gain  +5.5dB  11
                                     (0b110 << 5) + 12,         // gain  +6.0dB  12
                                     (0b110 << 5) + 13,         // gain  +6.5dB  13
                                     (0b110 << 5) + 14,         // gain  +7.0dB  14

                                     // DA 2.5dB
                                     (0b101 << 5) + 10,         // gain  +7.5dB  15
                                     (0b101 << 5) + 11,         // gain  +8.0dB  16
                                     (0b101 << 5) + 12,         // gain  +8.5dB  17
                                     (0b101 << 5) + 13,         // gain  +9.0dB  18
                                     (0b101 << 5) + 14,         // gain  +9.5dB  19

                                     // DA 5.0dB
                                     (0b100 << 5) + 10,         // gain +10.0dB  20
                                     (0b100 << 5) + 11,         // gain +10.5dB  21
                                     (0b100 << 5) + 12,         // gain +11.0dB  22
                                     (0b100 << 5) + 13,         // gain +11.5dB  23
                                     (0b100 << 5) + 14,         // gain +12.0dB  24

                                     // DA 7.5dB
                                     (0b011 << 5) + 10,         // gain +12.5dB  25
                                     (0b011 << 5) + 11,         // gain +13.0dB  26
                                     (0b011 << 5) + 12,         // gain +13.5dB  27
                                     (0b011 << 5) + 13,         // gain +14.0dB  28
                                     (0b011 << 5) + 14,         // gain +14.5dB  29

                                     // DA 10.0dB
                                     (0b010 << 5) + 10,         // gain +15.0dB  30
                                     (0b010 << 5) + 11,         // gain +15.5dB  31
                                     (0b010 << 5) + 12,         // gain +16.0dB  32
                                     (0b010 << 5) + 13,         // gain +16.5dB  33
                                     (0b010 << 5) + 14,         // gain +17.0dB  34

                                     // // DA 12.5dB
                                     (0b001 << 5) + 10,         // gain +17.5dB  35
                                     (0b001 << 5) + 11,         // gain +18.0dB  36
                                     (0b001 << 5) + 12,         // gain +18.5dB  37
                                     (0b001 << 5) + 13,         // gain +19.0dB  38
                                     (0b001 << 5) + 14,         // gain +19.5dB  39

                                     // DA 12.5dB
                                     (0b001 << 5) + 15,         // gain +20.0dB  40
                                     (0b001 << 5) + 16,         // gain +20.5dB  41
                                     (0b001 << 5) + 17,         // gain +21.0dB  42
                                     (0b001 << 5) + 18,         // gain +21.5dB  43
                                     (0b001 << 5) + 19,         // gain +22.0dB  44
                                     (0b001 << 5) + 20,         // gain +22.5dB  45

                                     // DA 12.5dB
                                     (0b001 << 5) + 21,         // gain +23.0dB  46
                                     (0b001 << 5) + 22,         // gain +23.5dB  47
                                     (0b001 << 5) + 23,         // gain +24.0dB  48
                                     (0b001 << 5) + 24,         // gain +24.5dB  49
                                     (0b001 << 5) + 25,         // gain +25.0dB  50
                                     (0b001 << 5) + 26,         // gain +25.5dB  51

                                     // DA 15.0dB
                                     (0b000 << 5) + 22,         // gain +26.0dB  52
                                     (0b000 << 5) + 23,         // gain +26.5dB  53
                                     (0b000 << 5) + 24,         // gain +27.0dB  54
                                     (0b000 << 5) + 25,         // gain +27.5dB  55
                                     (0b000 << 5) + 26,         // gain +28.0dB  56

                                     // DA 15.0dB
                                     (0b000 << 5) + 27,         // gain +28.5dB  57
                                     (0b000 << 5) + 28,         // gain +29.0dB  58
                                     (0b000 << 5) + 29,         // gain +29.5dB  59
                                     (0b000 << 5) + 30,         // gain +30.0dB  60
                                     (0b000 << 5) + 31 };       // gain +30.5dB  61

/* Use "Golden values" when the DW1000 device is not calibrated during production */
/* Depends on channel. TxPower/ PGdelay values in OTP are valid only for channel 5 for DWM1001 */ 
static ref_values_t ref_goldenval[6] = {
    // Place holder
    { 
        0x0,
        0x0,
        0x0,
        0x0,
        0x0
    },
    // Place holder
    { 
        0x0,
        0x0,
        0x0,
        0x0,
        0x0
    },
    // Channel 2 
    {    
         0xc2,           /* PG_DELAY */
         0x07274767,     /* Power */
         0x81,           /* Temp */
         0x369,          /* PG_COUNT */
         0x7e            /* Vbat */
    },
    // Place holder
    { 
        0x0,
        0x0,
        0x0,
        0x0,
    },
    // Place holder
    { 
        0x0,
        0x0,
        0x0,
        0x0,
    },
    // Channel 5
    {    
         0xb5,           /* PG_DELAY */
         0x25456585,     /* Power */
         0x81,           /* Temp */
         0x369,          /* PG_COUNT */
         0x7e            /* Vbat */
    }
};

/**
 * Read the tx configuration reference values from OTP.
 *
 * @param[in] ref_values_t pointer to the reference structure
 * @param[out] ref values
 * @return  1 if CH2/CH5, 0 for other channels
 */
uint8_t tvc_otp_read_txcfgref(ref_values_t* ref, uint8 chan)
{
  /* OTP reading: transmission parameters */
  uint32 val[5];
  uint8 txp_address ;

  switch (chan)
  {
  case 2:
      txp_address = OTP_TXPWR_CH2_PRF64_ADDRESS;
      break;

  case 5:
      txp_address = OTP_TXPWR_CH5_PRF64_ADDRESS;
      break;

  default:
      return 0;
  }

  if(chan == 5 || chan == 2)
  {
      /* OTP reading: from reference registers, should be calibrated during production test */
      dwt_otpread(txp_address, val, 1);
      dwt_otpread(OTP_PGCNT_ADDRESS, val+1, 1);
      dwt_otpread(OTP_XTRIM_ADDRESS, val+2, 1);
      dwt_otpread(OTP_TEMP_VBAT_ADDRESS, val+3, 1);
      dwt_otpread(OTP_ANT_DLY, val+4, 1);

      if (OTP_VALID(val[0]) && OTP_VALID(val[1]) && OTP_VALID(val[2]) && OTP_VALID(val[3]) && OTP_VALID(val[4]) ) {
          ref->power = val[0];

          if (chan == 5)
          {
              ref->pgcnt = (val[1]) & 0xffff;
              ref->pgdly = (val[2] >> 16) & 0xff;
              ref->antdly = (val[4]) & 0xffff;
          }
          else
          {
              ref->pgcnt = (val[1] >> 16) & 0xffff;
              ref->pgdly = (val[2] >> 24) & 0xff;
              ref->antdly = (val[4] >> 16) & 0xffff;
          }
          ref->temp  = val[3] & 0xff;
          ref->vbat  = (val[3] >> 8) & 0xff;
      }
      else
      {
          /*OTP values are not valid. Use golden values instead. */
          ref->power = ref_goldenval[chan].power;
          ref->pgcnt = ref_goldenval[chan].pgcnt;
          ref->temp  = ref_goldenval[chan].temp;
          ref->pgdly = ref_goldenval[chan].pgdly;    
          ref->vbat  = ref_goldenval[chan].vbat;
      }
  }
  else
  {
      // Error channel must be either 2 or 5 with DWM1001 TDoA Tag project
	  return 0;
  }
  return 1;
}

/**
 * Run bandwidth and TX power compensation
 *
 * @return none
 */
void tvc_comp(dwt_txconfig_t* tx_cfg, ref_values_t * ref, uint8 channel)
{
  uint8 curr_temp;
  uint8 curr_vbat;
  int delta_temp;
  int delta_voltage;
  uint16_t tempvbat;

  /* Only do compensation when the reference registers are set during production */
  if ((ref->power) && (ref->pgcnt) && (ref->temp))
  {
    /* Set SPI clock to 2MHz */
    port_set_dw1000_slowrate();         

    /* Read DW1000 IC temperature and supply volgate for compensation procedure. */
    tempvbat = dwt_readtempvbat(0);    
    curr_vbat = tempvbat & 0xff;
    curr_temp = (tempvbat >> 8) & 0xff;

    /* Calculate temperature and voltage difference in raw value unit */
    delta_temp = (int) (curr_temp - ref->temp);
    delta_voltage = (int) (curr_vbat - ref->vbat);

    /* Compensate bandwidth and power settings for temperature */
    tx_cfg->PGdly = dwt_calcbandwidthtempadj(ref->pgcnt);
    tx_cfg->power = tvc_calcpowertempvbatadj(channel, ref->power, delta_temp, delta_voltage);

    //dwt_configuretxrf(tx_cfg);

    /* Set SPI clock to 8MHz */
    port_set_dw1000_fastrate();
  }
}


/*! ------------------------------------------------------------------------------------------------------------------
 * @fn tvc_calcpowertempadj()
 *
 * @brief this function determines the corrected power setting (TX_POWER setting) for the
 * DW1000 which changes over temperature.
 *
 * Note: only ch2 or ch5 are supported, if other channel is used - the COMP factor should be calculated and adjusted
 *
 * input parameters:
 * @param channel - uint8 - the channel at which compensation of power level will be applied: 2 or 5
 * @param delta_temp - int - the difference between current ambient temperature (raw value units)
 *                                  and the temperature at which reference measurements were made (raw value units)

 * output parameters: None
 *
 * returns: (uint32) The corrected TX_POWER register value
 */
int8 tvc_calcpowertempadj(uint8 channel, int delta_temp)
{
    int8 delta_power;
    int negative = 0;

    if(delta_temp < 0)
    {
        negative = 1;
        delta_temp = -delta_temp; //make (-)ve into (+)ve number
    }

    // Calculate the expected power differential at the current temperature
    if(channel == 5)
    {
        delta_power = ((((delta_temp * SAR_TEMP_TO_CELCIUS_CONV_INT) / 65536) * TEMP_COMP_FACTOR_CH5) >> 12) ; //>>12 is same as /4096
    }
    else if(channel == 2)
    {
        delta_power = ((((delta_temp * SAR_TEMP_TO_CELCIUS_CONV_INT) / 65536) * TEMP_COMP_FACTOR_CH2) >> 12) ; //>>12 is same as /4096
    }
    else
    {
        delta_power = 0;
    }

    if(negative == 1)
    {
        delta_power = -delta_power; //restore the sign
    }
    return delta_power;
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn tvc_calcpowervbatadj()
 *
 * @brief this function determines the power change for the DW1000 which changes over voltage.
 *
 * input parameters:
 * @param delta_voltage - int  - the difference between current voltage and reference voltage from OTP (raw value units)

 * output parameters: None
 *
 * returns: (int8) The TX_POWER need to change (in 0.5dB steps)
 */

 int8 tvc_calcpowervbatadj(uint8 channel , int delta_voltage)
{
     int8 delta_power;

     /* Using /2^n and not >> n to avoid incorrect results when delta_power is negative */
     /* 2^12 = 4096
      * 2^17 = 131072
      * 2^19 = 524288
      * 2^23 = 8388608     */

     if(channel == 5)
     {
         delta_power = ((((delta_voltage * VBAT_COMP_FACTOR_INT_CH5 ) / 4096) * SAR_VBAT_TO_VOLT_CONV_INT) / 8388608);
     }
     else if(channel == 2)
     {
         delta_power = ((((delta_voltage * VBAT_COMP_FACTOR_INT_CH2 ) / 524288) * SAR_VBAT_TO_VOLT_CONV_INT) / 8388608);
     }
     else
     {
         delta_power = 0;
     }

     return delta_power;
}

 /*! ------------------------------------------------------------------------------------------------------------------
  * @fn tvc_computetxpowersetting()
  *
  * @brief this function calculates the appropriate change to the TX_POWER register to compensate
  * the TX power output at different temperatures.
  *
  * input parameters:
  * @param ref_powerreg - uint32 - the TX_POWER register value recorded when reference measurements were made
  * @param power_adj - uint32 - the adjustment in power level to be made, in 0.5dB steps
  *
  * output parameters:
  *
  * returns: (uint32) The setting to be programmed into the TX_POWER register
  */
 uint32 tvc_computetxpowersetting_lut(uint32 ref_powerreg, int32 power_adj)
 {
    uint8 current_da_attn = 0 ;
    uint8 current_mixer_gain = 0 ;
    uint32 new_regval = 0;
    int32 new_gain = 0 ;
    int i;

    for(i = 0; i < 4; i++)
    {
     current_da_attn = ((ref_powerreg >> (i*8)) & 0xE0) >> 5;
     current_mixer_gain = (ref_powerreg >> (i*8)) & 0x1F;

     // assuming da_attn will never be 111 (no output)
     current_da_attn = 0b110 - current_da_attn;
     new_gain = current_da_attn * MIX_DA_FACTOR + current_mixer_gain + power_adj ;
     if ( new_gain < 0 ) {
         new_gain = 0;
     }
     if ( new_gain >= (int32) (sizeof(tx_gain_lookup) / (int32) sizeof(tx_gain_lookup[0])) ) {
         new_gain = sizeof(tx_gain_lookup) / sizeof(tx_gain_lookup[0]) - 1;
     }

     new_regval |= (uint32) (tx_gain_lookup[ new_gain ] ) << (i * 8) ;
    }

    return (uint32)new_regval;
 }


/*! ------------------------------------------------------------------------------------------------------------------
 * @fn tvc_calcpowertempvbatadj()
 *
 * @brief this function determines the corrected power setting (TX_POWER setting) for the
 * DW1000 which changes over temperature and voltage.
 *
 * Note: only ch2 or ch5 are supported, if other channel is used - the COMP factor should be calculated and adjusted
 *
 * input parameters:
 * @param channel       - uint8   - the channel at which compensation of power level will be applied: 2 or 5
 * @param ref_powerreg  - uint32  - the TX_POWER register value recorded when reference measurements were made
 * @param delta_temp    - int     - the difference between current ambient temperature (raw value units)
 *                                  and the temperature at which reference measurements were made (raw value units)
 * @param raw_voltage   - uint8   - the current supply voltage (raw value units)
 * output parameters: None
 *
 * returns: (uint32) The corrected TX_POWER register value
 */
 uint32 tvc_calcpowertempvbatadj(uint8 channel, uint32 ref_powerreg, int delta_temp, int delta_voltage)
{
    int8 delta_power;

    delta_power =  tvc_calcpowervbatadj(channel, delta_voltage);
    delta_power += tvc_calcpowertempadj(channel, delta_temp);

    if(delta_power == 0)
    {
        return ref_powerreg ; //no change to power register
    }

    // Adjust the TX_POWER register value
    return tvc_computetxpowersetting_lut(ref_powerreg, delta_power);
}


