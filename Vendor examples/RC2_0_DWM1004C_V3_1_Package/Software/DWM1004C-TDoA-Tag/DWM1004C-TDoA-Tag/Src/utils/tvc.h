/*
 * @file       tvc.h
 *
 * @brief      temperature and voltage compensation utilities
 *
 * @author     Decawave Software
 *
 * @attention  Copyright 2019 (c) DecaWave Ltd.
 *             All rights reserved.
 */


#ifndef _TVC_H_
#define _TVC_H_ 1

#include "deca_device_api.h"

#ifdef __cplusplus
 extern "C" {
#endif


/* definitions for OTP */
#define OTP_TEMP_VBAT_ADDRESS       (0x1D)
#define OTP_TXPWR_CH2_PRF64_ADDRESS (0x13)
#define OTP_TXPWR_CH5_PRF64_ADDRESS (0x19)
#define OTP_PGCNT_ADDRESS           (0x1B)
#define OTP_XTRIM_ADDRESS           (0x1E)
#define OTP_ANT_DLY                 (0x1C)
#define OTP_VALID(x)                (((x) != 0) && ((x) != 0xffffffff))

 //New value tested for 2.2V to 3.6V range
 #define VBAT_COMP_FACTOR_CH2      (-5.16)
 #define VBAT_COMP_FACTOR_INT_CH2 (-2705326) // (INT) (-5.16 * 524288 (2^19))
 #define VBAT_COMP_FACTOR_CH5      (-6.76)
 #define VBAT_COMP_FACTOR_INT_CH5 (-27689) // (INT) (-6.76 * 4096 (2^12))

 #define SAR_VBAT_TO_VOLT_CONV_INT (48489) // (INT 1.0/173 * 8388608 (2^23))
 #define SAR_TEMP_TO_CELCIUS_CONV_INT (74711) // 1.14 = 74711 / 65536

/* definitions for temp / vbat calibration */
#define DW_BW_TXPWR_MAX_TEMP_DIFF   2     // 2 deg / SAR_TEMP_TO_CELCIUS_CONV
#define DW_BW_TXPWR_MAX_VBAT_DIFF   17    // 0.1v * MVOLT_TO_SAR_VBAT_CONV 
#define DW_VBAT_MIN                 2200  // 2200 millivolts, i.e. 2.2V
#ifndef ABS
#define ABS(x)  (((x) < 0) ? (-(x)) : (x))
#endif

/* define structure for DW1000 device reference values */
struct ref_values {
    uint8   pgdly;
    uint32  power;
    uint8   temp;
    uint16  pgcnt;
    uint8   vbat;
    uint16  antdly;

};

typedef struct ref_values ref_values_t;


/**
 * Read the tx configuration reference values from OTP.
 *
 * @param[in] ref_values_t pointer to the reference structure
 *            uint8 chan : current channel
 * @param[out] ref values
 * @return  1 if CH2/CH5, 0 for other channels
 */
uint8_t tvc_otp_read_txcfgref(ref_values_t* ref, uint8 chan);

/**
 * Run bandwidth and TX power compensation
 *
 * @param[in] reference structure
 * @param[out] tx_cfg values
 *
 * @return none
 */
void tvc_comp(dwt_txconfig_t* tx_cfg, ref_values_t * ref, uint8 channel);

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn dwt_calcpowertempvbatadj()
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
 * @param delta_voltage - int   -   the difference between current voltage (raw value units)
 *                                  and the voltage  at which reference measurements were made (raw value units)
 * output parameters: None
 *
 * returns: (uint32) The corrected TX_POWER register value
 */
 uint32 tvc_calcpowertempvbatadj(uint8 channel, uint32 ref_powerreg, int delta_temp, int delta_voltage);


#ifdef __cplusplus
}
#endif

#endif /* _TVC_H_ */
