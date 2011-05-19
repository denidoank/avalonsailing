// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
#ifndef IO_EPOS_MOTOR_H
#define IO_EPOS_MOTOR_H

#include <stdint.h>

/* Methods for controlling Maxon motors over EPOS according to
 *  the "Maxon Positioning Controller; Application Note "Device Programming"
 *
 * http://test.maxonmotor.com/docsx/Download/Product/Pdf/EPOS_Application_Note_Device_Programming_E.pdf
 *
 *  All functions return the epos_read/writeobject return value that
 *  causes it to exit or 0 on success.
 *
 *  All functions take the port filedescriptor and can node id as
 *  first and second parameters.
 *
 *  // means  NOT IMPLEMENTED
 *
 *  section 1, 10: First Step, Utilities
 *  //  epos_motor_configure(...)
 *  //  epos_motor_savecfg()
 *  //  epos_motor_factory_reset()
 *  //  epos_motor_cobid_reset()
 *
 *  section 8, 9:
 *      epos_motor_clearfault()
 *      epos_motor_status(uint32* status, position, velocity, current)
 *
 *  section 2: Profile Position Mode
 *      epos_motor_ppm_init(struct EposPPMConfig*)
 *      epos_motor_ppm_execabs(uint32 target);  // absolute target position
 *      epos_motor_ppm_execrel(uint32 target);  // relative target position
 *      epos_motor_wait_target(int timeout_ms);  // bit 10
 *
 *   section 3: Homing Mode
 *      epos_motor_homing(struct EposHomingConfig*)
 *   // epos_motor_wait_homed(int timeout_ms);  // bit 10 or 12  -> USE wait_target
 *
 *   section 4 Profile Velocity Mode
 *      epos_motor_ppm_init(struct EposPVMConfig*)
 *      epos_motor_ppm_exec(uint32 target);   // target velocity
 *      epos_motor_wait_target(int timeout_ms);  // bit 10
 *
 *   section 5/6 Position Mode / Velocity Mode
 *      epos_motor_position(uint32 min, max, maxerr)
 *   // epos_motor_velocity(uint32 min, max, maxerr)
 *   // epos_motor_current(uint32 maxcurrent, maxspeed, ttcw)
 *      epos_motor_quickstop()
 *
 *   Home Grown:
 *      epos_motor_probe(uint32* serialnumber)
 *
 */

uint32_t epos_motor_probe(int fd, uint8_t nodeid, uint32_t* serialnumber);

//  epos_motor_configure(...)
//  epos_motor_savecfg()
//  epos_motor_factory_reset()
//  epos_motor_cobid_reset()

// See Programmers reference for status register (0x6041[0])

enum {  // The Bits, programmers reference sec
        EPOS_STS_BIT_READY = (1<<0),    // "Ready to switch on"
        EPOS_STS_BIT_ON    = (1<<1),    // "Switched on"
        EPOS_STS_BIT_ENABLED = (1<<2),  // "Operation enable",
        EPOS_STS_BIT_FAULT = (1<<3),    // "Fault",
        EPOS_STS_BIT_POWER = (1<<4),    // "Voltage enabled (power stage on)",
        EPOS_STS_BIT_STOPPED = (1<<5),  // "Quick stop",
        EPOS_STS_BIT_DISABLED = (1<<6), // "Switch on disable",
        EPOS_STS_BIT_MYSTERYBIT = (1<<7), //  "not used (Warning)",
        EPOS_STS_BIT_OFFSETCURRENT = (1<<8), // "Offset current measured",
        EPOS_STS_BIT_NMTOPERATIONAL = (1<<9), // "Remote (NMT Slave State Operational)",
        EPOS_STS_BIT_TARGETREACHED = (1<<10), // "Target reached",
        EPOS_STS_BIT_LIMITED = (1<<11),       // "Internal limit active",
        EPOS_STS_BIT_OPACK = (1<<12),         // "Set-point/ ack Speed/ Homing attained",
        EPOS_STS_BIT_OPERR = (1<<13),         // "Following error/ Not used/ Homing error",
        EPOS_STS_BIT_REFRESHING = (1<<14),    // "Refresh cycle of power stage",
        EPOS_STS_BIT_REFERENCED = (1<<15),    // "Position referenced to home position",
};

enum {  // The Combinations, programmers reference sec 8.1.1
        EPOS_STS_MASK  = 0x416F,
        EPOS_STS_START = 0x0,
        EPOS_STS_NOTREADYTOSWITCHON = 0x0100,
        EPOS_STS_SWITCHONDISABLED   = 0x0140,
        EPOS_STS_READYTOSWITCHON    = 0x0121,
        EPOS_STS_SWITCHEDON         = 0x0123,
        EPOS_STS_REFRESH            = 0x4123,
        EPOS_STS_MEASUREINIT        = 0x4133,
        EPOS_STS_OPERATIONENABLE    = 0x0137,
        EPOS_STS_QUICKSTOPACTIVE    = 0x0117,
        EPOS_STS_FAULTREACTIONACTIVEDISABLED = 0x010F,
        EPOS_STS_FAULTREACTIONACTIVEENABLED = 0x011F,
        EPOS_STS_FAULT              = 0x0108,
};

enum {  // Bits of the Error register 0x1001
        EPOS_ERR_BIT_GENERIC    = (1<<0),
        EPOS_ERR_BIT_CURRENT    = (1<<1),
        EPOS_ERR_BIT_VOLTAGE    = (1<<2),
        EPOS_ERR_BIT_TEMPERATURE = (1<<3),
        EPOS_ERR_BIT_COMMUNICATION = (1<<4),
        EPOS_ERR_BIT_PROFILE    = (1<<5),
        EPOS_ERR_BIT_RESERVED   = (1<<6),
        EPOS_ERR_BIT_MOTION     = (1<<7),
};

// Queries corresponding registers for non-null arguments only.
uint32_t epos_motor_status(int fd, uint8_t nodeid,
                           uint32_t* status,
                           uint32_t* error,
                           uint32_t* position_qc,
                           uint32_t* velocity_rpm,
                           uint32_t* current_ma);

const char* epos_strstatus(uint32_t status);

// Get current and last 5 device errors, not to be confused with
// communication errors.  top one is current
uint32_t epos_get_deverrors(int fd, uint8_t nodeid, uint32_t errors[5]);
const char* epos_strdeverror(uint32_t error);

struct EposPPMConfig {
        uint32_t max_following_error;     // 0x6065-00  User specific [2000 qc]
        uint32_t min_position_limit;      // 0x607D-01  User specific [-2147483648 qc]
        uint32_t max_position_limit;      // 0x607D-02  User specific [2147483647 qc]
        uint32_t max_profile_velocity;    // 0x607F-00  Motor specific [25000 rpm]
        uint32_t profile_velocity;        // 0x6081-00  Desired Velocity [1000 rpm]
        uint32_t profile_acceleration;    // 0x6083-00  User specific [10000 rpm/s]
        uint32_t profile_deceleration;    // 0x6084-00  User specific [10000 rpm/s]
        uint32_t quickstop_deceleration;  // 0x6085-00  User specific [10000 rpm/s]
        uint32_t motion_profile_type;     // 0x6086-00  User specific [0]
};

struct EposPVMConfig {
        uint32_t max_profile_velocity;
        uint32_t profile_acceleration;
        uint32_t profile_deceleration;
        uint32_t quickstop_deceleration;
        uint32_t motion_profile_type;
};

uint32_t epos_motor_ppm_init(int fd, uint8_t nodeid, struct EposPPMConfig* cfg);
uint32_t epos_motor_ppm_execabs(int fd, uint8_t nodeid, uint32_t target); // absolute target position
uint32_t epos_motor_ppm_execrel(int fd, uint8_t nodeid, uint32_t target); // relative target position
uint32_t epos_motor_pvm_init(int fd, uint8_t nodeid, struct EposPVMConfig* cfg);
uint32_t epos_motor_pvm_exec(int fd, uint8_t nodeid, uint32_t target);   // target velocity
uint32_t epos_motor_wait_target(int fd, uint8_t nodeid, int timeout_ms);

struct EposHomingConfig {
        uint32_t max_following_error;
        uint32_t home_offset;
        uint32_t max_profile_velocity;
        uint32_t quickstop_deceleration;
        uint32_t switch_search_speed;
        uint32_t zero_search_speed;
        uint32_t homing_acceleration;
        uint32_t current_threshold;
        uint32_t home_position;
        uint32_t homing_method;
};

uint32_t epos_motor_homing(int fd, uint8_t nodeid, struct EposHomingConfig* cfg);
// uint32_t epos_motor_wait_homed(int fd, uint8_t nodeid, int timeout_ms);  // bit 10 or 12 USE wait_target instead

// 3 more modes, TBD
// epos_motor_position(int fd, uint8_t nodeid, uint32 min, max, maxerr)
// epos_motor_velocity(uint32 min, max, maxerr)
// epos_motor_current(uint32 maxcurrent, maxspeed, ttcw)

uint32_t epos_motor_quickstop(int fd, uint8_t nodeid);
uint32_t epos_motor_clearfault(int fd, uint8_t nodeid);

#endif // IO_EPOS_MOTOR_H
