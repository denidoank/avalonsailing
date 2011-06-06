// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "com.h"

#ifdef DEBUG
static void
vlogf(const char* fmt, ...)
{
        va_list ap;
        char buf[1000];
        va_start(ap, fmt);
        vsnprintf(buf, 1000, fmt, ap);
        fprintf(stderr, "%s%s%s\n", buf,
                (errno) ? ": " : "",
                (errno) ? strerror(errno): "" );
        va_end(ap);
        return;
}

#define VLOGF(fmt, ...) vlogf(fmt, __VA_ARGS__)
#else
#define VLOGF(fmt, ...)
#endif

int
epos_open(const char* path_to_dev)
{
        int port;
        struct termios t;

        if ( (port = open(path_to_dev, O_RDWR | O_NOCTTY )) == -1) {
                VLOGF("open(%s, ...)", path_to_dev);
                return -1;
        }

        memset(&t, 0, sizeof(t));

        cfmakeraw(&t);
        t.c_cflag |=  CLOCAL | CREAD;
        cfsetspeed(&t, B38400);

        tcflush(port, TCIFLUSH);
        if (tcsetattr(port, TCSANOW, &t) == -1) {
                VLOGF("tcsetattr(%s)", path_to_dev);
                close(port);
                return -1;
        }

        return port;
}

static uint16_t
crc_ccitt(uint16_t crc, uint16_t data)
{
        uint16_t mask;
        for (mask = 0x8000; mask; mask >>= 1) {
                uint16_t c = crc & 0x8000;
                crc <<= 1;
                if(data & mask) crc++;
                if(c) crc ^= 0x1021;
        }
        return crc;
}

static uint16_t
frame_crc(uint8_t* p, int n)
{
        assert(n>2); assert(n%2 == 0);

        // opcode and len are other way around
        uint16_t crc = crc_ccitt(0, p[1] + (p[0]<<8));
        p += 2;
        n -= 2;
        while(n>2) {
                crc = crc_ccitt(crc, p[0] + (p[1]<<8));
                p += 2;
                n -= 2;
        }

        // last two bytes in the frame are the crc values themselves,
        // must be counted as zero
        crc = crc_ccitt(crc, 0);
        return crc;
}

static int
read_timeout(int fd, uint8_t* buf, int size)
{
        fd_set rfds;
        struct timeval tv = { 0, 500*1000 };  // 500 msec
        FD_ZERO(&rfds);  FD_SET(fd, &rfds);
        int err = select(fd+1, &rfds, NULL, NULL, &tv);
        if (err == 1)
                err = read(fd, buf, size);
        return err;
}

static uint32_t
xmit(int fd, uint8_t* data, int size)
{
        assert(size >= 6);  // 1 opcode, 1 len, at least one 16 bit word, plus 16 bit crc.

        uint16_t crc = frame_crc(data, size);
        data[size-2] = crc;
        data[size-1] = crc >> 8;

        int err = write(fd, data, 1);	// send opcode
        VLOGF("xmit: send opcode(%02x): %d\n", data[0], err);
        if (err != 1) return EPOS_ERR_XMIT;

        uint8_t ack = 0;
        err = read_timeout(fd, &ack, 1);	// receive readyAck
        VLOGF("xmit: receive readyack (%02x, '%c'): %d\n", ack, ack, err);

        if (err != 1) return EPOS_ERR_RECV;
        if (ack != 'O') return EPOS_ERR_NACK;

        uint8_t* p = data + 1;		// send len, data, crc
        int   n = size - 1;
        while (n) {
                err = write(fd, p, n);
                VLOGF("xmit: send data(%d): %d\n", n, err);
                if (err <= 0) return EPOS_ERR_XMIT;
                n -= err;
                p += err;
        }

        err = read_timeout(fd, &ack, 1);	// receive sendAck
        VLOGF("xmit: receive sendAck(%02x '%c'): %d\n", ack, ack, err);
        if (err != 1) return EPOS_ERR_RECV;
        if (ack != 'O') return EPOS_ERR_NACK;
        return 0;
}

static uint32_t
recv(int fd, uint8_t* data, int* size)
{
        assert(*size >= 6); // 1 opcode, 1 len, at least one 16 bit word, plus 16 bit crc.

        int err = read_timeout(fd, data, 1);	// recv opcode
        VLOGF("recv: recv opcode(%02x): %d\n", data[0], err);
        if (err != 1) return EPOS_ERR_RECV;

        char ack = 'O';
        err = write(fd, &ack, 1);	// send ack
        VLOGF("recv: send readyAck (%02x, '%c'): %d\n", ack, ack, err);
        if (err != 1) return EPOS_ERR_XMIT;

        err = read_timeout(fd, data + 1, 1);	// recv len
        VLOGF("recv: recv len(%d): %d\n", data[1], err);
        if (err != 1) return EPOS_ERR_RECV;

        int n = 2 * (data[1] + 1) + 2;
        if (2 + n > *size) return EPOS_ERR_BADRESPONSE;
        *size = 2 + n;

        uint8_t* p = data + 2;
        while (n) {
                err = read_timeout(fd, p, n);
                VLOGF("recv: recv data: %d\n", err);
                if (err <= 0) return EPOS_ERR_RECV;
                n -= err;
                p += err;
        }

        n = *size;

        uint16_t crc = frame_crc(data, n);
        ack = (crc == (data[n-1]<<8) + data[n-2]) ? 'O' : 'F';

        write(fd, &ack, 1);
        VLOGF("recv: send ack (%02x, '%c'): %d\n", ack, ack, err);

        return (ack = 'O') ? 0 : EPOS_ERR_BADRESPONSE;
}


uint32_t
epos_readobject(int fd, uint16_t index, uint8_t subindex, uint8_t nodeid, uint32_t* value)
{
        uint8_t xmitframe[] = {
                0x10,		// 0: opcode
                1,              // 1: len - 1, in units of 16-bit words after this one,
				//     and excluding the crc
                index,          // 2: low byte of index
                index >> 8,	// 3: high byte of index
                subindex,       // 4: subindex
                nodeid,         // 5: nodeid
                0, 0,           // 6,7: room for crc
        };

        uint32_t r = xmit(fd, xmitframe, sizeof(xmitframe));

        if (r != 0) return r;

        uint8_t recvframe[12];
        int len = sizeof(recvframe);

        r = recv(fd, recvframe, &len);

        if (r != 0) return r;
        if (recvframe[0] != 0) return EPOS_ERR_BADRESPONSE;
        if (len != 12 || recvframe[1] != 3) return EPOS_ERR_BADRESPONSE;

        r = (recvframe[5]<<24) + (recvframe[4]<<16) + (recvframe[3]<<8) + recvframe[2];

        if (r != 0) return r;

        if (value) {
                *value = (recvframe[9]<<24) + (recvframe[8]<<16) + (recvframe[7]<<8) + recvframe[6];
        }

        return 0;
}

uint32_t
epos_writeobject(int fd, uint16_t index, uint8_t subindex, uint8_t nodeid, uint32_t value)
{
        uint8_t xmitframe[12] = {
                0x11,		// 0: opcode
                3,              // 1: len - 1
                index,          // 2: low byte of index
                index >> 8,	// 3: high byte of index
                subindex,       // 4: subindex
                nodeid,         // 5: nodeid
                value,		value >> 8,  // 6,7..
                value >> 16,	value >> 24, // 8,9.. low endian value.
                0, 0,           // 10, 11: room for crc
        };

        uint32_t r = xmit(fd, xmitframe, sizeof(xmitframe));

        if (r != 0) return r;

        uint8_t recvframe[10];
        int len = sizeof(recvframe);

        r = recv(fd, recvframe, &len);
        VLOGF("Received %d bytes:%x\n", len, r);
        if (r != 0) return r;
        if (recvframe[0] != 0) return EPOS_ERR_BADRESPONSE;
        if (len != 8 || recvframe[1] != 1) return EPOS_ERR_BADRESPONSE;

        r = (recvframe[5]<<24) + (recvframe[4]<<16) + (recvframe[3]<<8) + recvframe[2];

        return r;
}



uint32_t
epos_sendnmtservice(int fd, uint8_t nodeid, int nmt_cmd)
{
        uint8_t xmitframe[] = {
                0x0e,		// 0: opcode
                1,              // 1: len - 1, in units of 16-bit words after this one,
				//     and excluding the crc
                nodeid, 0,	// 2,3: low byte of nodeid, high byte of nodeid == 0
                nmt_cmd, 0,	// 4,5: low byte of command, high byte of command == 0
                0, 0,           // 6,7: room for crc
        };

        return xmit(fd, xmitframe, sizeof(xmitframe));
}


uint32_t
epos_sendcanframe(int fd, uint16_t cobid, int len, uint8_t data[8])
{
        // Validity of CAN message:
        assert(cobid & ~0x7FF == 0);  // 11 bits
        assert(len <= 8);

        // NOTE: section 6.3.3.1 says len-1 == 9, but i think that's wrong (lvd).
        uint8_t xmitframe[] = {
                0x20,		// 0: opcode
                5,              // 1: len - 1, in units of 16-bit words after this one,
				//     and excluding the crc
                cobid,		// 2,3: lo/hi can id (only 11 bits)
                cobid >> 8,	//
                len, 0,		// 4,5: lo/hi of len, but hi == 0
                0, 0, 0, 0, 0, 0, 0, 0,  // 6..13: data
                0, 0,           // 14,15: room for crc
        };
        memmove(xmitframe+6, data, len);
        return xmit(fd, xmitframe, sizeof(xmitframe));
}


uint32_t
epos_requestcanframe(int fd, uint16_t cobid, int len, uint8_t data[8])
{
        // Validity of CAN message:
        assert(cobid & ~0x7FF == 0);  // 11 bits

        // NOTE: section 6.3.3.1 says len-1 == 9, but i think that's wrong (lvd).
        uint8_t xmitframe[] = {
                0x20,		// 0: opcode
                1,              // 1: len - 1, in units of 16-bit words after this one,
				//     and excluding the crc
                cobid, cobid >> 8, // 2,3: lo/hi can id
                len, 0,		// 4,5: lo/hi of len, but hi == 0
                0, 0,           // 6,7: room for crc
        };
        uint32_t r = xmit(fd, xmitframe, sizeof(xmitframe));
        if (r != 0) return r;

        uint8_t recvframe[16];
        int rawlen = sizeof(recvframe);

        r = recv(fd, recvframe, &rawlen);

        if (r != 0) return r;
        if (recvframe[0] != 0) return EPOS_ERR_BADRESPONSE;
        if (rawlen != 16 || recvframe[1] != 5) return EPOS_ERR_BADRESPONSE;

        r = (recvframe[5]<<24) + (recvframe[4]<<16) + (recvframe[3]<<8) + recvframe[2];
        if (r == 0) {
                memmove(recvframe+6, data, len);
                memset(data+len, 0, sizeof(data)-len);
        }
        return r;
}


static struct epos_error_str_t {
        uint32_t code;
        const char* msg;
} epos_error_str[] = {
  { 0x00000000, "No error." },
  { 0x05030000, "Toggle bit not alternated." },
  { 0x05040000, "SDO protocol timed out." },
  { 0x05040001, "Client/server command specifier not valid or unknown." },
  { 0x05040005, "Out of memory" },
  { 0x06010000, "Unsupported access to an object." },
  { 0x06010001, "Attempt to read a write only object."  },
  { 0x06010002, "Attempt to write a read only object."  },
  { 0x06020000, "Object does not exist in the object dictionary." },
  { 0x06040041, "Object cannot be mapped to the PDO." },
  { 0x06040042, "The number and length of the objects to be mapped would exceed PDO length." },
  { 0x06040043, "General parameter incompatibility reason." },
  { 0x06040047, "General internal incompatibility reason." },
  { 0x06060000, "Access failed due to an hardware error." },
  { 0x06070010, "Data type does not match, length of service parameter does not match." },
  { 0x06070012, "Data type does not match, length of service parameter too high." },
  { 0x06070013, "Data type does not match, length of service parameter too low." },
  { 0x06090011, "Sub-index does not exist." },
  { 0x06090030, "Value range of parameter exceeded (only for write access)." },
  { 0x06090031, "Value of parameter written too high." },
  { 0x06090032, "Value of parameter written too low." },
  { 0x06090036, "Maximum value is less than minimum value." },
  { 0x08000000, "General error." },
  { 0x08000020, "Data cannot be transferred or stored to the application." },
  { 0x08000021, "Data cannot be transferred or stored to the application because of local control." },
  { 0x08000022, "Data cannot be transferred or stored to the application because of the present device state."},
  { 0x0F00FFC0, "The device is in wrong NMT state." },
  { 0x0F00FFBF, "The RS232 command is illegal." },
  { 0x0F00FFBE, "The password is not correct." },
  { 0x0F00FFBC, "The device is not in service mode." },
  { 0x0F00FFB9, "Error Node-ID." },
// Our own
  { EPOS_ERR_BADRESPONSE, "RS232/EPOS: Bad response frame." },
  { EPOS_ERR_NACK,  "RS232/EPOS: Non-ACk." },
  { EPOS_ERR_RECV,  "RS232/EPOS: Receive error." },
  { EPOS_ERR_XMIT,  "RS232/EPOS: Transmit error." },
  { EPOS_ERR_BADCRC,  "RS232/EPOS: Received bad CRC." },
  { EPOS_ERR_TIMEOUT, "RS232/EPOS: Timeout waiting for value." },
  { 0, NULL }
};

const char*
epos_strerror(uint32_t e)
{
        struct epos_error_str_t* p;
        for (p = epos_error_str; p->msg; ++p)
                if (p->code == e)
                        return p->msg;
        return "Unknown error code";
}
