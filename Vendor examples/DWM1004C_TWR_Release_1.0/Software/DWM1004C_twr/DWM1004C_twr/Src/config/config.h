/*
 * @file       cmd_console.h
 *
 * @brief      Port headers file to Nordic nRF52382 Tag Board for DW1001
 *
 * @author     Decawave Software
 *
 * @attention  Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *             All rights reserved.
 */

#ifndef CMD_CONSOLE_H_
#define CMD_CONSOLE_H_

#ifdef __cplusplus
 extern "C" {
#endif

/* module DEFINITIONS */
#define MAX_TEXT_LEN_SIZE   64
#define MAX_COMPARM_SIZE    16
#define MAX_STR_SIZE        512

#define CMD_MALLOC_ENABLE   1

/* Exported function prototypes */
void load_bssConfig(void);
param_block_t *get_pbssConfig(void);
void save_bssConfig( const param_block_t * pbuf);

void restore_nvm_fconfig(void);


#ifdef __cplusplus
}
#endif

#endif /* CMD_CONSOLE_H_ */
