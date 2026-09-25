/*! ----------------------------------------------------------------------------
 * @file    deca_spi.c
 * @brief   SPI access functions
 *
 * @attention
 *
 * Copyright 2015 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */

#include <deca_device_api.h>
#include "deca_spi.h"
#include "port_platform.h"

#define DW_NSS_GPIO_Port DW_CS_GPIO_Port
#define DW_NSS_Pin DW_CS_Pin
/****************************************************************************//**
 *
 *                              DW1000 SPI section
 *
 *******************************************************************************/
/*! ------------------------------------------------------------------------------------------------------------------
 * Function: openspi()
 *
 * Low level abstract function to open and initialise access to the SPI device.
 * returns 0 for success, or -1 for error
 */
int openspi(/*SPI_TypeDef* SPIx*/)
{
    return 0;
} // end openspi()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: closespi()
 *
 * Low level abstract function to close the the SPI device.
 * returns 0 for success, or -1 for error
 */
int closespi(void)
{
    return 0;
} // end closespi()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: writetospi()
 *
 * Low level abstract function to write to the SPI
 * Takes two separate byte buffers for write header and write data
 * returns 0 for success, or -1 for error
 */
int writetospi(uint16       headerLength,
               const uint8  *headerBuffer,
               uint32       bodyLength,
               const uint8  *bodyBuffer)
{

    decaIrqStatus_t  stat ;
    stat = decamutexon() ;

    LL_GPIO_ResetOutputPin(DW_NSS_GPIO_Port, DW_NSS_Pin ); /**< Put chip select line low */

    while(headerLength--){
        while ( LL_SPI_IsActiveFlag_TXE(SPI1) == 0 )
        {
        }
        LL_SPI_TransmitData8(SPI1, *headerBuffer++);
    }

    while(bodyLength--){
        while ( LL_SPI_IsActiveFlag_TXE(SPI1) == 0 )
        {
        }
        LL_SPI_TransmitData8(SPI1, *bodyBuffer++);
    }

    // wait last transaction to complete
    while ( LL_SPI_IsActiveFlag_BSY(SPI1) != 0 )
    {
    }

    LL_GPIO_SetOutputPin(DW_NSS_GPIO_Port, DW_NSS_Pin); /**< Put chip select line high */

    decamutexoff(stat);
    return 0;
} // end writetospi()


/*! ------------------------------------------------------------------------------------------------------------------
 * @fn readfromspi()
 * @brief
 * Low level abstract function to read from the SPI
 * Takes two separate byte buffers for write header and read data
 * returns the offset into read buffer where first byte of read data may be found,
 * or returns -1 if there was an error
 */
//#pragma GCC optimize ("O3")
int readfromspi(uint16      headerLength,
                const uint8 *headerBuffer,
                uint32      readlength,
                uint8       *readBuffer)
{
    /* Blocking: Check whether previous transfer has been finished */

    // wait last transaction to complete
    while ( LL_SPI_IsActiveFlag_BSY(SPI1) != 0 )
    {
    }

    /* Process Locked */
    decaIrqStatus_t  stat ;
    stat = decamutexon() ;

    LL_GPIO_ResetOutputPin(DW_NSS_GPIO_Port, DW_NSS_Pin); /**< Put chip select line low */

    /* Send header */
    while(headerLength--){
        while ( LL_SPI_IsActiveFlag_TXE(SPI1) == 0 )
        {
        }
        LL_SPI_TransmitData8(SPI1, *headerBuffer++);
    }
    while ( LL_SPI_IsActiveFlag_BSY(SPI1) != 0 )
    {
    }
    LL_SPI_ReceiveData8(SPI1);  // dummy read

    /* for the data buffer use LL functions directly as the HAL SPI read function
     * has issue reading single bytes */
    while(readlength-- > 0)
    {
        /* Wait until TXE flag is set to send data */
        while ( LL_SPI_IsActiveFlag_TXE(SPI1) == 0 )
        {
        }

        LL_SPI_TransmitData8(SPI1, 0);   /* set output to 0 (MOSI), this is necessary for
                                            e.g. when waking up DW1000 from DEEPSLEEP via dwt_spicswakeup() function.
                                         */

        /* Wait until RXNE flag is set to read data */
        while ( LL_SPI_IsActiveFlag_RXNE(SPI1) == 0 )
        {
        }

        (*readBuffer++) = LL_SPI_ReceiveData8(SPI1);
    }

    LL_GPIO_SetOutputPin(DW_NSS_GPIO_Port, DW_NSS_Pin ); /**< Put chip select line high */

    /* Process Unlocked */
    decamutexoff(stat);
    return 0;
} // end readfromspi()

/****************************************************************************//**
 *
 *                              END OF DW1000 SPI section
 *
 *******************************************************************************/


