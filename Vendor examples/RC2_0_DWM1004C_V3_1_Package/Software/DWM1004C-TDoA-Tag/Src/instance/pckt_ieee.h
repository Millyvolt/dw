/*
 * @file       pckt_ieee.h
 *
 * @brief      DecaWave definition of TAG messages
 *
 * @author     Decawave Software
 *
 * @attention  Copyright 2018 (c) DecaWave Ltd, Dublin, Ireland.
 *             All rights reserved.
 */

#ifndef IEEE_EUI_64_TAG
#define IEEE_EUI_64_TAG

//IEEE EUI-64 tag ID (ISO/IEC 24730-62)
#define FRAME_CONTROL_BYTES       1
#define FRAME_SEQ_NUM_BYTES       1
#define FRAME_CRC                 2
#define FRAME_SOURCE_ADDRESS      8
#define FRAME_CTRLP              (FRAME_CONTROL_BYTES + FRAME_SEQ_NUM_BYTES) //2
#define FRAME_CRTL_AND_ADDRESS   (FRAME_SOURCE_ADDRESS + FRAME_CTRLP) //10 bytes

#define ADDR_BYTE_SIZE              EUI64_ADDR_SIZE

/* Frame Control */
#define FCS_EUI_64            0xC5
#define EUI64_ADDR_SIZE        (8)
/* size of IEEE_EUI_64 blink message including FCS with zero payload size */
#define EUI64_CONTROL_SIZE    (16)

/* 12 octets for Minimum IEEE ID blink */
typedef struct
{
    uint8 frameCtrl;                         //  frame control bytes 00
    uint8 seqNum;                            //  sequence_number 01
    uint8 tagID[EUI64_ADDR_SIZE];            //  02-09 64 bit addresses
    /*  10-11  we allow space for the CRC as it is logically part of the
        message. However DW1000 TX calculates and adds these bytes */
    uint8 fcs[2] ;
} iso_IEEE_EUI64_blink_msg ;


typedef enum {
	FC_POLL = 0xE0,
	FC_RESPONSE,
	FC_FINAL
} Function_Code;

typedef struct{
	const uint8_t ctrl1;	// 0x41
	const uint8_t ctrl2;	// 0x88
	uint8_t seq_num;
	const uint8_t PAN_id[2];	// 0xABCD
	uint8_t dst[2];		// address
	uint8_t src[2];		// address
	uint8_t fc;
	uint8_t fcs[2];
} Ranging_Frame;

typedef struct{
	const uint8_t ctrl1;	// 0x41
	const uint8_t ctrl2;	// 0x88
	uint8_t seq_num;
	const uint8_t PAN_id[2];	// 0xABCD
	uint8_t dst[2];		// address
	uint8_t src[2];		// address
	uint8_t fc;
	uint8_t timestamps[15];		// difference from Ranging_Frame
	uint8_t fcs[2];
} Ranging_Frame_Final;
//where addresses is: INITIATOR - 0x0001, RESPONDER - 0x0002



#endif //IEEE_EUI_64_TAG
