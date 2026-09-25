/*
 * @file       config.c
 *
 * @author     Decawave Software
 *
 * @attention  Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *             All rights reserved.
 *
 */
#include <string.h>
#include <stm32l0xx.h>

#include "instance.h"
//#include "stm32l0xx_hal.h"
#include "main.h"
//#include "stm32l0xx_hal.h"
#include "default_config.h"

/* The location of FConfig and defaultConfig are defined in Linker script file and project configuration options */

/* Changeble block of parameters in the EEPROM */
CRCprotected_param_t FConfig __attribute__((section(".fConfig"))) \
                                   __attribute__((aligned(FCONFIG_PAGE_SIZE))) ;

/* Application default constant parameters block in the NFLASH. Never changes. Used for Restore ONLY (RESTORE command or bad CRC). */
const param_block_t defaultFConfig __attribute__((section(".default_config"))) \
                                   __attribute__((aligned(FCONFIG_PAGE_SIZE))) = DEFAULT_CONFIG;

/* Run-Time config parameters. */
static param_block_t tmpConfig __attribute__((aligned(FCONFIG_PAGE_SIZE)));


/* IMPLEMENTATION */

/* Static functions */

/* Lock the EEPROM: */
static void LockEeprom(void)
{
   while ((FLASH->SR & FLASH_SR_BSY) != 0) /* Wait for FLASH to be free */
   {
   }

   FLASH->PECR = FLASH->PECR & ~(FLASH_PECR_ERRIE | FLASH_PECR_EOPIE); /* disable flash interrupts */
   FLASH->PECR = FLASH->PECR | FLASH_PECR_PELOCK; /* Lock memory with PELOCK */
}


/* Unlock the EEPROM: */
#define FLASH_PEKEY1               ((uint32_t)0x89ABCDEFU) /*!< Flash program erase key1 */
#define FLASH_PEKEY2               ((uint32_t)0x02030405U) /*!< Flash program erase key: used with FLASH_PEKEY2
                                                               to unlock the write access to the FLASH_PECR register and
                                                               data EEPROM */

#define FLASH_PRGKEY1              ((uint32_t)0x8C9DAEBFU) /*!< Flash program memory key1 */
#define FLASH_PRGKEY2              ((uint32_t)0x13141516U) /*!< Flash program memory key2: used with FLASH_PRGKEY2
                                                               to unlock the program memory */

#define FLASH_OPTKEY1              ((uint32_t)0xFBEAD9C8U) /*!< Flash option key1 */
#define FLASH_OPTKEY2              ((uint32_t)0x24252627U) /*!< Flash option key2: used with FLASH_OPTKEY1 to
                                                                unlock the write access to the option byte block */

static void UnlockEeprom(void)
{
   while ((FLASH->SR & FLASH_SR_BSY) != 0) /* Wait for FLASH to be free */
   {
   }

   if ((FLASH->PECR & FLASH_PECR_PELOCK) != 0) /* If PELOCK is locked */
   {
      FLASH->PEKEYR = FLASH_PEKEY1; /* Unlock PELOCK */
      FLASH->PEKEYR = FLASH_PEKEY2;
   }
   FLASH->PECR = FLASH->PECR | (FLASH_PECR_ERRIE | FLASH_PECR_EOPIE); /* enable flash interrupts */
}

/**
* Brief This function programs a word of data EEPROM.
* The ERASE bit and DATA bit are cleared in PECR at the beginning
* words are automatically erased if required before programming
* Param addr is the 32-bit EEPROM address to program, data is the 32 bit word to program
* Retval None
*/
static void EepromProgram(uint32_t * addr, const uint32_t ee_data)
{
/* NOTE: The EEPROM must be unlocked and the flash interrupts must have been enabled prior to calling this function.*/
   *addr = ee_data; /* write data to EEPROM */
   __WFI();
   if (*addr != ee_data)
   {
   }
}

static uint8_t CRC8_Calculate( uint8_t * pdata, uint32_t size)
{

    LL_CRC_ResetCRCCalculationUnit(CRC);
    while ( size-- > 0)
    {
        LL_CRC_FeedData8(CRC, *pdata++);
    }
    return LL_CRC_ReadData8(CRC);
}
/* Exported functions  */

/* @brief    Writes buffer to the nonvolatile config location &FConfig
 * assumes data fold to page
 *
 */
void save_bssConfig( const param_block_t * pbuf)
{
    CRCprotected_param_t temp_protected_config;
    memset(&temp_protected_config.free, 0xFF, sizeof(temp_protected_config.free));
    memcpy(&temp_protected_config.params, pbuf, sizeof(temp_protected_config.params));

    temp_protected_config.CRC8 = CRC8_Calculate( (uint8_t *) &temp_protected_config.params, sizeof(temp_protected_config.params));

    uint32_t * FConfig_dword_pointer  = (uint32_t *) &FConfig;
    const uint32_t * current_dword_pointer = (uint32_t *) &temp_protected_config;

    uint32_t num_dwords =  sizeof(CRCprotected_param_t) / sizeof(uint32_t);
    if ( sizeof(CRCprotected_param_t) % sizeof(uint32_t) ) {
        // extra dword to fit the rest of
        num_dwords++;
    }

    __disable_irq();
    UnlockEeprom();
    for ( uint32_t i = 0; i<num_dwords; i++ )
    {
        EepromProgram(FConfig_dword_pointer + i, current_dword_pointer[i] );
    }
    LockEeprom();
    __enable_irq();
}

void load_bssConfig(void)
{
    uint8_t tempCRC = CRC8_Calculate( (uint8_t *) &FConfig.params, sizeof(FConfig.params));

    if ( tempCRC != FConfig.CRC8 ) {
        save_bssConfig( &defaultFConfig);
    }

    memcpy(&tmpConfig, &FConfig.params, sizeof(tmpConfig));
    app.pConfig = &tmpConfig;
}


param_block_t *get_pbssConfig(void)
{
  return app.pConfig;
}




/* @fn       restore_nvm_fconfig
 * @brief    init main program run-time configuration parameters from NVM
 *           assumes that memory model .text and .bss the same
 * */
void restore_nvm_fconfig(void)
{    
    save_bssConfig( &defaultFConfig);
    load_bssConfig();
}

/* end of config.c */
