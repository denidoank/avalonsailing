// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Communication layer public functions of the EPOS library, to talk
// to a Maxxon EPOS 24/5 motor controller over RS232, cf section 6 of
// the EPOS Communication Guide.
//
// http://shop.maxonmotor.com/maxon/assets_external/Katalog_neu/eshop/Downloads/maxon_motor_control/Positionierung/Common_EPOS/Communication_Guide/EPOS_Communication_Guide_E.pdf
//
//  Note. The segmented read/writeobject methods are not implemented.
//
#ifndef IO_EPOS_COM_H
#define IO_EPOS_COM_H

#include <stdint.h>

/*
 * epos_open: Open and initialize serial device for use with the
 * EPOS protocol.
 *
 * path_to_dev must contain the filename of a serial device.
 *
 * The port will be set to use 8N1 format.  If an error occurs during
 * initialisation, prints a message on stderr and returns
 * -1. Otherwise the proper termios flags will be set so that EPOS
 * messages can be sent and received with , and the filedescriptor
 * will be returned.  The returned filedescriptor may be closed with
 * close(2).
 *
 * This is a convenience function.  The epos_* functions below can
 * be used with any functional serial port filedescriptor.
 */
int epos_open(const char* path_to_dev);


/*
 * epos_readobject:  Issue a ReadObject command and receive the reply.
 *
 * See section 6.3.1.1.
 *
 * Issues the command with given index, subindex and nodeid. And waits
 * for the reply.  The reply is checked on crc. If there is any error,
 * returns a non-zero error code, otherwise fills value with the (host
 * ordered) response.
 *
 * Error codes are those defined in section 6.4 plus the ones defined below.
 *
 */

uint32_t epos_readobject(int fd, uint16_t index, uint8_t subindex, uint8_t nodeid, uint32_t* value);


/*
 * epos_writeobject:  Issue a WriteObject command and receive the reply.
 *
 * See section 6.3.2.1
 *
 * Arguments and return value are like those of epos_readobject().
 */
uint32_t epos_writeobject(int fd, uint16_t index, uint8_t subindex, uint8_t nodeid, uint32_t value);


/*
 * epos_sendnmtservice: Issue a CAN Network ManagementTsomething message.
 *
 * See section 6.3.2.3
 *
 */

enum {
        EPOS_NMT_CMD_STARTREMOTENODE = 1,
        EPOS_NMT_CMD_STOPREMOTENODE = 2,
        EPOS_NMT_CMD_ENTERPREOPERATIONAL = 128,
        EPOS_NMT_CMD_RESETNODE = 129,
        EPOS_NMT_CMD_RESETCOMMUNICATION = 130,
};

uint32_t epos_sendnmtservice(int fd, uint8_t nodeid, int nmt_cmd);


/*
 * epos_sendcanframe: Send a generic CAN frame.
 *
 * CAN messages may be up to 8 bytes in length.
 *
 * See section 6.3.3.1
 *
 */

uint32_t epos_sendcanframe(int fd, uint16_t cobid, int len, uint8_t data[8]);


/*
 * epos_requestcanframe: Issue a CAN Remote Transmission Request (RTR)
 *                       for a generic CAN frame and receive the response.
 *
 * Note: len must be set on call.
 *
 * See section 6.3.3.2
 *
 */
uint32_t epos_requestcanframe(int fd, uint16_t cobid, int len, uint8_t data[8]);


/*
 * turn epos error code e into a human readable string.
 */
const char* epos_strerror(uint32_t e);

enum {
        EPOS_ERR_BADRESPONSE	= 0x08100010,
        EPOS_ERR_NACK		= 0x08100020,
        EPOS_ERR_RECV		= 0x08100030,
        EPOS_ERR_XMIT		= 0x08100040,
        EPOS_ERR_BADCRC		= 0x08100050,
        EPOS_ERR_TIMEOUT	= 0x08100060,  // used by seq, epos_waitobject
};

#endif // IO_EPOS_COM_H
