/*
 * @file cmd_fn.h
 *
 * @brief  header file for cmd_fn.c
 *
 * @author Decawave Software
 *
 * @attention Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */
#ifndef INC_CMD_FN_H_
#define INC_CMD_FN_H_    1

#ifdef __cplusplus
 extern "C" {
#endif

#include "default_config.h"
#include "port_platform.h"

//-----------------------------------------------------------------------------
/* module DEFINITIONS */

#ifndef mANY
#define mANY (0)
#endif

//-----------------------------------------------------------------------------
/* All cmd_fn functions have unified input: (char *text, param_block_t *pbss, int val)
 * Will use REG_FN macro to declare unified functions.
 * */
#define REG_FN(x) const char *x(char *text, param_block_t *pbss, int val)

 /* command table structure definition */
 typedef struct {
     const char  *name;            /**< Command name string */
     const int  mode;            /**< allowed execution operation mode */
     REG_FN       ((*fn));         /**< function() */
 }command_t;

 extern const command_t known_commands[];

 void command_stop_received(void);



#ifdef __cplusplus
}
#endif


#endif /* INC_CMD_FN_H_ */
