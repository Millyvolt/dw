/*
 * @file       tdoa_tag_main.c
 *
 * @brief      TDoA Tag Application code
 *
 * @author     Decawave
 *
 * @attention  Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *             All rights reserved.
 */

#include "cmd_uart_rx.h"

extern void port_tx_msg(char *ptr, int len);

#define COM_RX_BUF_SIZE   (64)

static uint8_t local_buff[COM_RX_BUF_SIZE];
static uint16_t local_buff_length=0;

typedef enum {
     NO_DATA = 0,
     COMMAND_READY,
}uart_data_e;


/*
 * @brief    Waits only commands from incoming stream.
 *             The binary interface (deca_usb2spi stream) is not allowed.
 *
 * @return  COMMAND_READY : the data for future processing can be found in app.local_buff : app.local_buff_len
 *          NO_DATA : no command yet
 */
uart_data_e waitForCommand(uint8_t *pBuf, uint16_t len)
{
    uart_data_e ret;
    static uint8_t cmdLen = 0;
    static uint8_t cmdBuf[COM_RX_BUF_SIZE]; /**< slow command buffer : small size */

    ret = NO_DATA;

    if (len <= 2)
    {/* "slow" command mode: Human interface. Wait until '\r' or '\n' */
        if (cmdLen == 0)
        {
            memset(cmdBuf, 0, sizeof(cmdBuf));
        }

        if (cmdLen < (sizeof(local_buff) - 1))
        {
            /*FIXME: YB current fix to avoid double print due to port_tx uart hack/current implementation*/
            //port_tx_msg((char *)pBuf, len);    //ECHO

            if (*pBuf == '\n' || *pBuf == '\r')
            {
                if (cmdLen > 0)
                {
                    memcpy(local_buff, cmdBuf, cmdLen);

                    local_buff_length = cmdLen;
                    local_buff[cmdLen] = 0;

                    ret = COMMAND_READY;
                    cmdLen = 0;
                }
            }
            else if (*pBuf == '\b') //erase of a char in the terminal
            {
                if (cmdLen > 0)
                {
                    --cmdLen;
                    cmdBuf[cmdLen] = 0;
                    port_tx_msg("\033[K", 3);
                }

            }
            else
            {
                cmdBuf[cmdLen] = *pBuf;
                cmdLen++;
            }
        }
        else
        {
            /* error in command protocol : flush everything */
            port_tx_msg("\r\n", 2);
            cmdLen = 0;
        }
    }
    else
    {/* "fast" command mode : assume every data buffer is "COMMAND_READY" */

        if (len < (sizeof(local_buff) - 1))
        {
            memcpy(local_buff, pBuf, len);

            local_buff_length = len;
            local_buff[len] = 0;
            cmdLen = 0;

            ret = COMMAND_READY;
        }
        else
        { /* overflow in protocol : flush everything */
            port_tx_msg( "Error: \r\n", 2);
            cmdLen = 0;
        }
    }

    return (ret);
}


/* @fn  process_uartmsg
 *
 * @brief Function is used for processing UARY msg.
 *        If the UART msg is dwUWB command then enter
 *        into UART_COMMAND mode and perform operation
 *        based on uart input.
 * @param[in] void
 * */
void process_uartmsg(void)
{
    uint8_t rx_buf[COM_RX_BUF_SIZE];
    int uartLen, res;

    memset(rx_buf,0,sizeof(rx_buf));
    uartLen = deca_uart_receive( rx_buf, COM_RX_BUF_SIZE );
    
    if(uartLen > 0)
    {
      res = waitForCommand(rx_buf, uartLen);
    
      if (res == COMMAND_READY)
      {
          int len = MIN((local_buff_length-1), (sizeof(local_buff)-1));
          local_buff[len+1] = 0;
          command_parser((char *)local_buff);            //parse and execute the command
      }
    }
}

