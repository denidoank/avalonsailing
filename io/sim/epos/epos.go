// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Simulate an epos on a pty
//
package main

import (
	"errors"
	"flag"
	"io"
	"log"
	"os"
	"time"
)

// Epos communication layer

var (
	ErrRecv   = errors.New("epos receive error")
	ErrXmit   = errors.New("epos transmit error")
	ErrBadCRC = errors.New("epos bad frame crc")
)

func crc_ccitt(crc, data uint16) uint16 {
	for mask := uint16(0x8000); mask != 0; mask >>= 1 {
		c := crc & 0x8000
		crc <<= 1
		if data&mask != 0 {
			crc++
		}
		if c != 0 {
			crc ^= 0x1021
		}
	}
	return crc
}

func frame_crc(p []byte) uint16 {
	if len(p) < 2 || len(p)%2 != 0 {
		return 0xffff
	}

	// opcode and len are other way around
	crc := crc_ccitt(0, uint16(p[1])+(uint16(p[0])<<8))
	p = p[2:]
	for len(p) > 2 {
		crc = crc_ccitt(crc, uint16(p[0])+(uint16(p[1])<<8))
		p = p[2:]
	}

	// last two bytes in the frame are the crc values themselves,
	// must be counted as zero
	crc = crc_ccitt(crc, 0)
	return crc
}

// this is the mirror image of io/rudderd2/com.c xmit
func EposRecv(f *os.File, buf []byte) (data []byte, err error) {
	n, err := f.Read(buf[0:1]) // opcode
	if err != nil {
		return nil, err
	}
	if n != 1 {
		return nil, ErrRecv
	}

	n, err = f.Write([]byte{'O'}) // ack
	if err != nil {
		return nil, err
	}
	if n != 1 {
		return nil, ErrRecv
	}

	n, err = f.Read(buf[1:2]) // len
	if err != nil {
		return nil, err
	}
	if n != 1 {
		return nil, ErrRecv
	}

	nn := 2*(buf[1]+1) + 2
	if int(2+nn) > len(buf) {
		return nil, ErrRecv
	}
	data = buf[:2+nn]
	n, err = io.ReadFull(f, data[2:])
	if err != nil {
		return nil, err
	}
	crc := frame_crc(data)
	if uint16(buf[len(data)-2])+uint16(data[len(data)-1])<<8 != crc {
		n, err = f.Write([]byte{'F'}) // nack
		return nil, ErrBadCRC
	}

	n, err = f.Write([]byte{'O'}) // ack
	if err != nil {
		return nil, err
	}
	if n != 1 {
		return nil, ErrRecv
	}

	return data, nil
}

func EposXmit(f *os.File, buf []byte) error {
	crc := frame_crc(buf)
	buf[len(buf)-2] = byte(crc & 0xff)
	buf[len(buf)-1] = byte(crc >> 8)

	// send opcode
	n, err := f.Write(buf[0:1])
	if err != nil {
		return err
	}
	if n != 1 {
		return ErrXmit
	}

	// wait for ack
	var rbuf [1]byte
	n, err = f.Read(rbuf[:])
	if err != nil {
		return err
	}
	if n != 1 || rbuf[0] != 'O' {
		return ErrXmit
	}
	n, err = f.Write(buf[1:])
	if err != nil {
		return err
	}
	if n != len(buf)-1 {
		return ErrXmit
	}

	// wait for ack
	n, err = f.Read(rbuf[:])
	if err != nil {
		return err
	}
	if n != 1 || rbuf[0] != 'O' {
		return ErrXmit
	}
	return nil
}

func splitVal(val uint32) (v1, v2, v3, v4 byte) {
	return byte(val), byte(val >> 8), byte(val >> 16), byte(val >> 24)
}

func parseReqReadObj(buf []byte) (nodeid byte, index uint16, subindex byte) {
	index = uint16(buf[2]) + uint16(buf[3])<<8
	subindex = buf[4]
	nodeid = buf[5]
	return
}

func fmtAckReadObj(err, val uint32) []byte {
	e0, e1, e2, e3 := splitVal(err)
	v0, v1, v2, v3 := splitVal(val)
	return []byte{
		0, 3, // opcode, len
		e0, e1, e2, e3, // 32 bit error code
		v0, v1, v2, v3, // 32 bit value
		0, 0, // room for crc
	}
}

func parseReqWriteObj(buf []byte) (nodeid byte, index uint16, subindex byte, val uint32) {
	index = uint16(buf[2]) + uint16(buf[3])<<8
	subindex = buf[4]
	nodeid = buf[5]
	val = uint32(buf[6]) + uint32(buf[7])<<8 + uint32(buf[8])<<16 + uint32(buf[9])<<24
	return
}

func fmtAckWriteObj(err uint32) []byte {
	e0, e1, e2, e3 := splitVal(err)
	return []byte{
		0, 1, // opcode, len
		e0, e1, e2, e3, // 32 bit error code
		0, 0, // room for crc
	}
}

// -----------------------------------------------------------------------------

const (
	regSerial  = (0x1018 << 8) | 4

	regControl = 0x6040 << 8
	CONTROL_CLEARFAULT = 0x80
	CONTROL_SHUTDOWN = 6
	CONTROL_START = 0x3F
	CONTROL_SWITCHON = 0xF

	regStatus  = 0x6041<<8 
        STATUS_FAULT = (1<<3)
        STATUS_HOMEREF = (1<<15)
        STATUS_HOMINGERROR = (1<<13)
        STATUS_TARGETREACHED = (1<<10)

        regOpmode  = 0x6060 << 8
        OPMODE_HOMING = 6
        OPMODE_PPM = 1

	regError = 0x1001<<8
	regErrHistCount = 0x1003<<8
	regTargPos = 0x607A<<8
        regCurrPos = 0x6064<<8
        regBMMHPos = 0x6004<<8
)


type EposController struct {
	Registers map[uint32]uint32
}

func (ep *EposController) ReadObj(reg uint32) (val, r uint32) {
	return ep.Registers[reg], 0
}

func (ep *EposController) WriteObj(reg, val uint32) uint32 {
	ep.Registers[reg] = val
	return 0
}


// -----------------------------------------------------------------------------

var (
	port     = flag.String("port", "/dev/ptywf", "Master PTY to attach to")
	delay_ms = flag.Int("delay", 50, "Delay between request and response")
	be_sail  = flag.Bool("s", false, "be the sail epos + bmmh")
	be_left  = flag.Bool("l", false, "be the left rudder epos")
	be_right = flag.Bool("r", false, "be the left rudder epos")
)


func main() {
	flag.Parse()
	f, err := os.OpenFile(*port, os.O_RDWR, 0)
	if err != nil {
		log.Fatal(err)
	}

	var nodes [16]*EposController

 	// nodeid:1 serial:0x9010537  // right
	// nodeid:2 serial:0x9010506  // sail
	// nodeid:3 serial:0x9011145  // left
	// nodeid:8 serial:0x1227     // bmmh

	if *be_right {
		nodes[1] = &EposController{ map[uint32]uint32{
				regSerial:0x9010537,
		}}
	}
	if *be_left {
		nodes[3] = &EposController{ map[uint32]uint32{
				regSerial:0x9011145,
		}}
	}
	if *be_sail {
		nodes[2] = &EposController{ map[uint32]uint32{
				regSerial:0x9010506,
		}}
		nodes[8] = &EposController{ map[uint32]uint32{
				regSerial:0x1227,
		}}
	}

	var abuf [256]byte
	for {
		buf := abuf[:]
		buf, err = EposRecv(f, buf)
		if err != nil {
			break
		}

		switch buf[0] { // opcode
		case 0x10: // read object
			if len(buf) < 8 {
				log.Fatalf("read object short buffer: %x", buf)
			}
			node, index, subindex := parseReqReadObj(buf)
			log.Printf("read object %d:%x[%d]", node, index, subindex)
			var r, val uint32
			if int(node) < len(nodes) && nodes[node] != nil {
				val, r = nodes[node].ReadObj(uint32(index)<<8 + uint32(subindex))
			} else {
				val, r = 0, 0x0F00FFB9  // err bad nodeid
			}
			time.Sleep(time.Duration(*delay_ms) * time.Millisecond)
			err = EposXmit(f, fmtAckReadObj(r, val))
			if err != nil {
				log.Fatal(err)
			}

		case 0x11: // write object
			if len(buf) < 12 {
				log.Fatalf("write object short buffer: %x", buf)
			}
			node, index, subindex, val := parseReqWriteObj(buf)
			log.Printf("write object %d:%x[%d] = %x", node, index, subindex, val)
			var r uint32
			if int(node) < len(nodes) && nodes[node] != nil {
				r = nodes[node].WriteObj(uint32(index)<<8 + uint32(subindex), val)
			} else {
				r = 0x0F00FFB9  // err bad nodeid
			}
			time.Sleep(time.Duration(*delay_ms) * time.Millisecond)
			err = EposXmit(f, fmtAckWriteObj(r))
			if err != nil {
				log.Fatal(err)
			}

		default:
			log.Fatalf("opcode %x not supported", buf[0])
		}
	}

	if err != nil {
		log.Fatal(err)
	}
}
