/*
 * @file     cmd_fn.c
 * @brief    collection of executables functions from defined known_commands[]
 *
 * @author   Decawave Software
 *
 * @attention Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#include <string.h>
#include <stdio.h>
#include "cmd.h"
#include "cmd_fn.h"
#include "config.h"
#include "translate.h"
#include "version.h"
#include "deca_version.h"
#include "port_platform.h"

//-----------------------------------------------------------------------------
const char CMD_FN_RET_OK[] = "ok\r\n";

/****************************************************************************//**
 *
 *                          f_xx "command" FUNCTIONS
 *
 * REG_FN(f_tag) macro will create a function
 *
 * const char *f_tag(char *text, param_block_t *pbss, int val)
 *
 * */

//-----------------------------------------------------------------------------
// Parameters change section

REG_FN(f_chan)
{
    int tmp = chan_to_deca(val);
    const char * ret = NULL;

    if(tmp>=0)
    {
      pbss->dwt_config.chan = tmp;
      ret = CMD_FN_RET_OK;
    }
    return (ret);
}
REG_FN(f_prf)
{
    int tmp = prf_to_deca(val);
    const char * ret = NULL;

    if(tmp>=0)
    {
      pbss->dwt_config.prf = (uint8_t)(tmp);
      ret = CMD_FN_RET_OK;
    }
    return (ret);
}
REG_FN(f_plen)
{
    int tmp = plen_to_deca(val);
    const char * ret = NULL;

    if(tmp>=0)
    {
      pbss->dwt_config.txPreambLength = (uint16_t)(tmp);
      ret = CMD_FN_RET_OK;
    }
    return (ret);
}
REG_FN(f_rxPAC)
{
    int tmp = pac_to_deca(val);
    const char * ret = NULL;

    if(tmp>=0)
    {
      pbss->dwt_config.rxPAC = (uint8_t)(tmp);
      ret = CMD_FN_RET_OK;
    }
    return (ret);
}
REG_FN(f_txCode)
{
    pbss->dwt_config.txCode = (uint8_t)(val);
    pbss->dwt_config.rxCode = (uint8_t)(val);
    return (CMD_FN_RET_OK);
}
REG_FN(f_nsSFD)
{
    pbss->dwt_config.nsSFD = (val == 0)?(0):(1);
    return (CMD_FN_RET_OK);
}     
REG_FN(f_dataRate)
{
    int tmp = bitrate_to_deca(val);
    const char * ret = NULL;

    if(tmp>=0)
    {
      pbss->dwt_config.dataRate = (uint8_t)(tmp);
      ret = CMD_FN_RET_OK;
    }
    return (ret);
}     
REG_FN(f_phrMode)
{
    pbss->dwt_config.phrMode = (val == 0)?(0):(1);
    return (CMD_FN_RET_OK);
}     
REG_FN(f_sfdTO)
{
    pbss->dwt_config.sfdTO = (uint16_t)(val);
    return (CMD_FN_RET_OK);
}     
REG_FN(f_smartPowerEn)
{
    pbss->smartPowerEn = (val == 0)?(0):(1);
    return (CMD_FN_RET_OK);
}     
REG_FN(f_interval_in_ms)
{
    pbss->blink.interval_in_ms = (uint32_t)(val);
    return (CMD_FN_RET_OK);
}     
REG_FN(f_interval_slow_in_ms)
{
    pbss->blink.interval_slow_in_ms = (uint32_t)(val);
    return (CMD_FN_RET_OK);
}     
REG_FN(f_randomness)
{
    pbss->blink.randomness = (uint8_t)(val);
    return (CMD_FN_RET_OK);
}     

//-----------------------------------------------------------------------------
// Communication /  user statistics section

/* @brief
 * */
REG_FN(f_decaTDoATag)
{
    const char *ret = NULL;
    const char ver[] = FULL_VERSION;

    char str[MAX_STR_SIZE];

    int  hlen;

    hlen = sprintf(str,"JS%04X", 0x5A5A);    // reserve space for length of JS object

    sprintf(&str[strlen(str)],"{\"Info\":{\r\n");
    sprintf(&str[strlen(str)],"\"Device\":\"DWM1004C TDoA Tag\",\r\n");
    sprintf(&str[strlen(str)],"\"Version\":\"%s\",\r\n", ver);
    sprintf(&str[strlen(str)],"\"Build\":\"%s %s\",\r\n", __DATE__, __TIME__ );
    sprintf(&str[strlen(str)],"\"Driver\":\"%s\"}}", DW1000_DEVICE_DRIVER_VER_STRING );

    sprintf(&str[2],"%04X",strlen(str)-hlen);   //add formatted 4X of length, this will erase first '{'
    str[hlen]='{';                            //restore the start bracket
    port_tx_msg(str, strlen(str));
    port_tx_msg("\r\n", 2);

    ret = CMD_FN_RET_OK;

    return (ret);
}

//-----------------------------------------------------------------------------

/*
 * @brief   show current UWB parameters in JSON format
 *
 * */
REG_FN(f_jstat)
{
    char str[MAX_STR_SIZE];

    int  hlen;

    hlen = sprintf(str,"JS%04X", 0x5A5A);    // reserve space for length of JS object
    sprintf(&str[strlen(str)],"{\"UWB PARAM\":{\r\n");

    sprintf(&str[strlen(str)],"\"CHAN\":%d,\r\n",deca_to_chan(pbss->dwt_config.chan));
    sprintf(&str[strlen(str)],"\"PRF\":%d,\r\n", deca_to_prf (pbss->dwt_config.prf));
    sprintf(&str[strlen(str)],"\"PLEN\":%d,\r\n",deca_to_plen(pbss->dwt_config.txPreambLength));
    sprintf(&str[strlen(str)],"\"DATARATE\":%d,\r\n",deca_to_bitrate(pbss->dwt_config.dataRate));
    sprintf(&str[strlen(str)],"\"TXCODE\":%d,\r\n",pbss->dwt_config.txCode);
    sprintf(&str[strlen(str)],"\"PAC\":%d,\r\n", deca_to_pac (pbss->dwt_config.rxPAC));
    sprintf(&str[strlen(str)],"\"NSSFD\":%d,\r\n",pbss->dwt_config.nsSFD);
    sprintf(&str[strlen(str)],"\"PHRMODE\":%d,\r\n",pbss->dwt_config.phrMode);
    sprintf(&str[strlen(str)],"\"SMARTPOWER\":%d,\r\n",pbss->smartPowerEn);
    sprintf(&str[strlen(str)],"\"BLINKFAST\":%lu,\r\n",pbss->blink.interval_in_ms);
    sprintf(&str[strlen(str)],"\"BLINKSLOW\":%lu,\r\n",pbss->blink.interval_slow_in_ms);
    sprintf(&str[strlen(str)],"\"RANDOMNESS\":%d,\r\n",pbss->blink.randomness);
    sprintf(&str[strlen(str)],"\"TAGIDSET\":%d,\r\n",pbss->tagIDset);
    sprintf(&str[strlen(str)],"\"TAGID\":0x%02x%02x%02x%02x%02x%02x%02x%02x}}",
                                               pbss->tagID[7], pbss->tagID[6], pbss->tagID[5], pbss->tagID[4],
                                               pbss->tagID[3], pbss->tagID[2], pbss->tagID[1], pbss->tagID[0]);

    sprintf(&str[2],"%04X",strlen(str)-hlen);//add formatted 4X of length, this will erase first '{'
    str[hlen]='{';                            //restore the start bracket
    sprintf(&str[strlen(str)],"\r\n");
    port_tx_msg(str, strlen(str));

    return (CMD_FN_RET_OK);
}

/*
 * @brief show current mode of operation,
 *           version, and the configuration
 *
 * */
REG_FN(f_stat)
{
    const char * ret = CMD_FN_RET_OK;

    f_decaTDoATag(text, pbss, val);
    f_jstat(text, pbss, val);

    ret = CMD_FN_RET_OK;
    return (ret);
}


REG_FN(f_help_app)
{
    int        indx = 0;
    const char * ret = NULL;

    char str[MAX_STR_SIZE];
    
    while (known_commands[indx].name != NULL)
    {
        sprintf(str,"%s \r\n", known_commands[indx].name);

        port_tx_msg((char*)str, strlen(str));

        indx++;
    }

    ret = CMD_FN_RET_OK;
    return (ret);
}

REG_FN(f_tcwm)
{
    return (CMD_FN_RET_OK);
}

REG_FN(f_tcfm)
{
    return (CMD_FN_RET_OK);
}
//-----------------------------------------------------------------------------
// Communication change section

/*
 * @brief restore NV configuration from defaults
 *
 * */
REG_FN(f_restore)
{
	restore_nvm_fconfig();

	return (CMD_FN_RET_OK);
}

/*
 * @brief restart the FW
 *
 * */
REG_FN(f_restart)
{
	Restart();
	return (CMD_FN_RET_OK);
}

/*
 * @brief save configuration
 *
 * */
REG_FN(f_save)
{
    save_bssConfig(pbss);

    return (CMD_FN_RET_OK);
}

//-----------------------------------------------------------------------------



/* end f_xx command functions */

//-----------------------------------------------------------------------------
/* list of known commands:
 * NAME, allowed_MODE,     REG_FN(fn_name)
 * */
const command_t known_commands []= {
    /* CMDNAME   MODE   fn     */
    {"STAT",    mANY,   f_stat},
    {"HELP",    mANY,   f_help_app},
    {"SAVE",    mANY,   f_save},
    {"RESTORE",    mANY,   f_restore},
    {"RESTART",    mANY,   f_restart},

    /* Service Commands */
   
    {"CHAN", mANY, f_chan},             //!< channel number {1, 2, 3, 4, 5, 7 }
    {"PRF",  mANY, f_prf},              //!< Pulse Repetition Frequency {DWT_PRF_16M or DWT_PRF_64M}
    {"PLEN", mANY, f_plen},             //!< DWT_PLEN_64..DWT_PLEN_4096
    {"PAC", mANY, f_rxPAC},             //!< Acquisition Chunk Size (Relates to RX preamble length)
    {"TXCODE", mANY, f_txCode},         //!< TX preamble code
    {"NSSFD", mANY, f_nsSFD},           //!< Boolean should we use non-standard SFD for better performance
    {"DATARATE", mANY, f_dataRate},     //!< Data Rate {DWT_BR_110K, DWT_BR_850K or DWT_BR_6M8}
    {"PHRMODE", mANY, f_phrMode},       //!< PHR mode {0x0 - standard DWT_PHRMODE_STD, 0x3 - extended frames DWT_PHRMODE_EXT
    {"SFDTO", mANY, f_sfdTO},               //!< SFD timeout value (in symbols)
    {"SMARTPOWEREN", mANY, f_smartPowerEn}, //!< Smart Power enable / disable};

    {"TCWM",    mANY,  f_tcwm},
    {"TCFM",    mANY,  f_tcfm},

    {"BLINKFAST", mANY, f_interval_in_ms},      //!< Blink interval in ms
    {"BLINKSLOW", mANY, f_interval_slow_in_ms}, //!< Blink interval in ms
    {"RANDOMNESS", mANY, f_randomness},         //!< Randomness in %

    {NULL,      mANY,   NULL}
};
