// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//

#include <stdio.h>
#include <strings.h>

#include "motor.h"
#include "seq.h"
#include "com.h"

struct epos_status_str_t {
        uint32_t c;
        const char* msg;
} epos_status[] = {
        { EPOS_STS_START, "Start" },
        { EPOS_STS_NOTREADYTOSWITCHON, "Not Ready to Switch On" },
        { EPOS_STS_SWITCHONDISABLED, "Switch On Disabled" },
        { EPOS_STS_READYTOSWITCHON, "Ready to Switch On"},
        { EPOS_STS_SWITCHEDON, "Switched On"},
        { EPOS_STS_REFRESH, "Refreshing"},
        { EPOS_STS_MEASUREINIT,  "Measure Init"},
        { EPOS_STS_OPERATIONENABLE, "Operation Enable"},
        { EPOS_STS_QUICKSTOPACTIVE, "Quickstop Active"},
        { EPOS_STS_FAULTREACTIONACTIVEDISABLED, "Fault Reaction Active (Disabled)"},
        { EPOS_STS_FAULTREACTIONACTIVEENABLED,  "Fault Reaction Active (Enabled)"},
        { EPOS_STS_FAULT, "Fault"},
        {0 , NULL}
};

const char* epos_strstatus(uint32_t status) {
        status &= EPOS_STS_MASK;
        struct epos_status_str_t* s;
        for (s = epos_status; s->msg; ++s)
                if (s->c == status)
                        return s->msg;
        return "Unknown status";
}

struct epos_status_str_t epos_deverror[] = {
        { 0x0000,  "No Error" },
        { 0x1000,  "Generic Error" },
        { 0x2310,  "Over Current Error" },
        { 0x3210,  "Over Voltage Error" },
        { 0x3220,  "Under Voltage" },
        { 0x4210,  "Over Temperature" },
        { 0x5113,  "Supply Voltage (+5V) too low" },
        { 0x6100,  "Internal Software Error" },
        { 0x6320,  "Software Parameter Error" },
        { 0x7320,  "Sensor Position Error" },
        { 0x8110,  "CAN Overrun Error (Objects lost)" },
        { 0x8111,  "CAN Overrun Error" },
        { 0x8120,  "CAN Passive Mode Error" },
        { 0x8130,  "CAN Life Guard Error" },
        { 0x8150,  "CAN Transmit COB-ID collision" },
        { 0x81FD,  "CAN Bus Off" },
        { 0x81FE,  "CAN Rx Queue Overrun" },
        { 0x81FF,  "CAN Tx Queue Overrun" },
        { 0x8210,  "CAN PDO length Error" },
        { 0x8611,  "Following Error" },
        { 0xFF01,  "Hall Sensor Error" },
        { 0xFF02,  "Index Processing Error" },
        { 0xFF03,  "Encoder Resolution Error" },
        { 0xFF04,  "Hallsensor not found Error" },
        { 0xFF06,  "Negative Limit Error" },
        { 0xFF07,  "Positive Limit Error" },
        { 0xFF08,  "Hall Angle detection Error" },
        { 0xFF09,  "Software Position Limit Error" },
        { 0xFF0A,  "Position Sensor Breach" },
        { 0xFF0B,  "System Overloaded" },
};

const char* epos_strdeverror(uint32_t error) {
        struct epos_status_str_t* s;
        for (s = epos_deverror; s->msg; ++s)
                if (s->c == error)
                        return s->msg;
        return "Unknown error";
}


uint32_t
epos_get_deverrors(int fd, uint8_t nodeid, uint32_t errors[5])
{
        uint32_t err;
        int i;
        bzero(errors, sizeof(errors));

        uint32_t n = 0;
        if ((err= epos_readobject(fd, 0x1003, 0, nodeid, &n)) != 0)
                return err;

        for (i = 0; (i < 5) && (i < n); ++i)
                if ((err= epos_readobject(fd, 0x1003, i+1, nodeid, errors)) != 0)
                    return err;

        // clear error history
        if ((err= epos_writeobject(fd, 0x1003, 0, nodeid, 0)) != 0)
                return err;

        return 0;
}


// Probe for device type, manufacturer, identity object (section 14.1, 14.5, 14.13)
// timeout hardcoded to 1sec per probe.
uint32_t
epos_motor_probe(int fd, uint8_t nodeid, uint32_t* serialnumber)
{
        uint32_t err;

        // device type Index 0x1000 Sub-index 0x00 Type UNSIGNED32
        // Default Value 0x00020192  -> actually 20092
        uint32_t device_type = 0x92;
        err = epos_waitobject(fd, 1000, 0x1000,  0, nodeid, 0x000000FF, &device_type);
        if (err) fprintf(stderr, "Error probing for device type:%s", epos_strerror(err));
        if (err) return err;

        // manufacturer device name Index 0x1008 Sub-index 0x00 Type
        // VISIBLE_STRING Access CONST Default Value “EPOS”
        uint32_t manufacturer = 0x534f5045;
        err = epos_waitobject(fd, 1000, 0x1008, 0, nodeid, 0xFFFFFFFF, &manufacturer);
        if (err) fprintf(stderr, "Error probing for manufacturer:%s", epos_strerror(err));
        if (err) return err;

        // identity object Index 0x1018 vendor id Index 0x1018
        // Sub-index 0x01 Type UNSIGNED32 Default Value 0x000000FB
        uint32_t vendorid = 0xFB;
        err = epos_waitobject(fd, 1000, 0x1018, 1, nodeid, 0xFF, &vendorid);
        if (err) fprintf(stderr, "Error probing for vendorid:%s", epos_strerror(err));
        if (err) return err;

        // identity object serial number
        *serialnumber = 0;
        err = epos_waitobject(fd, 1000, 0x1018, 4, nodeid, 0, serialnumber);
        if (err) fprintf(stderr, "Probing for serial number:%s", epos_strerror(err));
        if (err) return err;
        fprintf(stderr, "EPOS Serial: %x\n", *serialnumber);
/*
        fprintf(stderr, "EPOS: type:%x manufacturer:%c%c%c%c vendor:%x serial:%x\n",
                device_type,
                ((char*)manufacturer)[3], ((char*)manufacturer)[2], ((char*)manufacturer)[1], ((char*)manufacturer)[0],
                vendorid, *serialnumber);
*/
        return 0;
}

// subindex = 0
static uint32_t probe(int fd, uint8_t nodeid, uint16_t index, uint32_t* val) {
        if (!val) return 0;
        *val = 0;
        return epos_waitobject(fd, 1000, index, 0, nodeid, 0, val);
}

uint32_t epos_motor_status(int fd, uint8_t nodeid,
                           uint32_t* status,
                           uint32_t* error,
                           uint32_t* position,
                           uint32_t* velocity,
                           uint32_t* current) {
        uint32_t err;
        if ((err = probe(fd, nodeid, 0x6041, status)) != 0) return err;
        if ((err = probe(fd, nodeid, 0x1001, error)) != 0) return err;
        if ((err = probe(fd, nodeid, 0x6064, position)) != 0) return err;
        if ((err = probe(fd, nodeid, 0x2028, velocity)) != 0) return err;
        if ((err = probe(fd, nodeid, 0x6078, current)) != 0) return err;
        return 0;
}

uint32_t
epos_motor_ppm_init(int fd, uint8_t nodeid, struct EposPPMConfig* cfg)
{
        struct EposCmd cmds[] = {
                { 0x6040, 0, 0x80 }, // clear fault
                { 0x6060, 0, 0x01 }, // mode of operation = PPM
                { 0x6065, 0, cfg->max_following_error },     // User specific [2000 qc]
                { 0x607D, 1, cfg->min_position_limit },      // User specific [-2147483648 qc]
                { 0x607D, 2, cfg->max_position_limit },      // User specific [2147483647 qc]
                { 0x607F, 0, cfg->max_profile_velocity },    // Motor specific [25000 rpm]
                { 0x6081, 0, cfg->profile_velocity },        // Desired Velocity [1000 rpm]
                { 0x6083, 0, cfg->profile_acceleration},     // User specific [10000 rpm/s]
                { 0x6084, 0, cfg->profile_deceleration},     // User specific [10000 rpm/s]
                { 0x6085, 0, cfg->quickstop_deceleration},   // User specific [10000 rpm/s]
                { 0x6086, 0, cfg->motion_profile_type},      // User specific [0]
                { 0x6040, 0, 0x6 }, // shutdown
                { 0x6040, 0, 0xF }, // switch on
                { 0, 0, 0 },
        };
        struct EposCmd* c = cmds;
        uint32_t err = epos_sequence(fd, nodeid, &c);
        return err;
}


uint32_t
epos_motor_ppm_execabs(int fd, uint8_t nodeid, uint32_t target)
{
        struct EposCmd cmds[] = {
                { 0x607A, 0, target }, // desired position
                { 0x6040, 0, 0x3f   }, // absolute target position, start immediately
                { 0, 0, 0 },
        };
        struct EposCmd* c = cmds;
        uint32_t err = epos_sequence(fd, nodeid, &c);
        return err;
}

uint32_t epos_motor_ppm_execrel(int fd, uint8_t nodeid, uint32_t target)
{
        struct EposCmd cmds[] = {
                { 0x607A, 0, target }, // desired position
                { 0x6040, 0, 0x7f   }, // relative target position, start immediately
                { 0, 0, 0 },
        };
        struct EposCmd* c = cmds;
        uint32_t err = epos_sequence(fd, nodeid, &c);
        return err;
}

uint32_t
epos_motor_pvm_init(int fd, uint8_t nodeid, struct EposPVMConfig* cfg)
{
        struct EposCmd cmds[] = {
                { 0x6040, 0, 0x80 }, // clear fault
                { 0x6060, 0, 0x03 }, // mode of operation = PVM
                { 0x607F, 0, cfg->max_profile_velocity },     //Motor specific [25000 rpm]
                { 0x6083, 0, cfg->profile_acceleration },     //User specific [10000 rpm/s]
                { 0x6084, 0, cfg->profile_deceleration },     //User specific [10000 rpm/s]
                { 0x6085, 0, cfg->quickstop_deceleration },   //User specific [10000 rpm/s]
                { 0x6086, 0, cfg->motion_profile_type },      //User Specific [0]
                { 0x6040, 0, 0x6 }, // shutdown
                { 0x6040, 0, 0xF }, // switch on
                { 0, 0, 0 },
        };
        struct EposCmd* c = cmds;
        uint32_t err = epos_sequence(fd, nodeid, &c);
        return err;
}


uint32_t epos_motor_pvm_exec(int fd, uint8_t nodeid, uint32_t target) {
        struct EposCmd cmds[] = {
                { 0x60FF, 0, target }, // target velocity
                { 0x6040, 0, 0xF   }, // start immediately
                { 0, 0, 0 },
        };
        struct EposCmd* c = cmds;
        uint32_t err = epos_sequence(fd, nodeid, &c);
        return err;
}

uint32_t epos_motor_homing(int fd, uint8_t nodeid, struct EposHomingConfig* cfg)
{
        struct EposCmd cmds[] = {
                { 0x6040, 0, 0x80 }, // clear fault
                { 0x6060, 0, 0x06 }, // mode of operation = homing mode
                { 0x6065, 0, cfg->max_following_error }, // User specific [2000 qc]
                { 0x607C, 0, cfg->home_offset},          // User specific [0 qc]
                { 0x607F, 0, cfg->max_profile_velocity},  // Motor specific [25000 rpm]
                { 0x6085, 0, cfg->quickstop_deceleration }, //User specific [10000 rpm/s]
                { 0x6099, 1, cfg->switch_search_speed},   //User specific [100 rpm]
                { 0x6099, 2, cfg->zero_search_speed},     //User specific [10 rpm]
                { 0x609A, 0, cfg->homing_acceleration},   //User specific [1000 rpm/s]
                { 0x2080, 0, cfg->current_threshold},     //User specific [500 mA]
                { 0x2081, 0, cfg->home_position },        //User specific [0 qc]
                { 0x6098, 0, cfg->homing_method },        // see firmware doc
                { 0x6040, 0, 0x6 }, // shutdown
                { 0x6040, 0, 0xF }, // switch on
                { 0x6040, 0, 0xF }, // switch on
                { 0x6040, 0, 0x1F }, // start homing
                { 0, 0, 0 },
        };
        struct EposCmd* c = cmds;
        uint32_t err = epos_sequence(fd, nodeid, &c);
        return err;
}


uint32_t epos_motor_wait_target(int fd, uint8_t nodeid, int timeout_ms) {
        uint32_t status = EPOS_STS_BIT_TARGETREACHED;
        return epos_waitobject(fd, timeout_ms,
                               0x6041, 0, nodeid, EPOS_STS_BIT_TARGETREACHED, &status);
}

uint32_t epos_motor_quickstop(int fd, uint8_t nodeid) {
        return epos_writeobject(fd, 0x6040, 0, nodeid, 0xB);
}

uint32_t epos_motor_clearfault(int fd, uint8_t nodeid) {
        return epos_writeobject(fd, 0x6040, 0, nodeid, 0x80);
}
