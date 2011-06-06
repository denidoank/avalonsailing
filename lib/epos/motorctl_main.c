// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Tool to control generic EPOS motors
//   mode: home
//

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "com.h"
#include "motor.h"

const char* version = "$Id: $";
int verbose=0;
const char* argv0;
const char default_path_to_port[]  = "/dev/ttyUSB0";

static void
crash(const char* fmt, ...)
{
        va_list ap;
        char buf[1000];
        va_start(ap, fmt);
        vsnprintf(buf, 1000, fmt, ap);
        fprintf(stderr, "%s:%s%s%s\n", argv0, buf,
                (errno) ? ": " : "",
                (errno) ? strerror(errno):"" );
        exit(1);
        va_end(ap);
        return;
}

static void usage(void)
{
        fprintf(stderr,
                "usage: %s [-dvh] [-s /path/to/port] [-i nodeid 1..16] [command ...]\n"
                "command:\n"
                "\thome\n"
                "\t\t-e max_following_Error\n"
                "\t\t-o home_Offset\n"
                "\t\t-v max_profile_Velocity\n"
                "\t\t-q Quickstop_deceleration\n"
                "\t\t-s Switch_search_speed\n"
                "\t\t-z Zero_search_speed\n"
                "\t\t-a homing_Acceleration\n"
                "\t\t-c Current_threshold\n"
                "\t\t-p home_Position\n"
                "\t\t-m homing_Method\n"
                "\tppm\n"
                "\t\t-e max_following_Error\n"
                , argv0);
        exit(1);
}



static const char* status_bits[] = {
        "Ready to switch on",
        "Switched on",
        "Operation enable",
        "Fault",
        "Voltage enabled (power stage on)",
        "Quick stop",
        "Switch on disable",
        "not used (Warning)",
        "Offset current measured",
        "Remote (NMT Slave State Operational)",
        "Target reached",
        "Internal limit active",
        "Set-point/ ack Speed/ Homing attained",
        "Following error/ Not used/ Homing error",
        "Refresh cycle of power stage",
        "Position referenced to home position",
        NULL
};

static const char* error_bits[] = {
        "GENERIC", "CURRENT", "VOLTAGE", "TEMPERATURE",
        "COMMUNICATION", "PROFILE", "RESERVED", "MOTION",
        NULL
};

uint32_t print_status(int port, uint8_t nodeid) {
        uint32_t status;
        uint32_t error;
        uint32_t position_qc;
        uint32_t velocity_rpm;
        uint32_t current_ma;
        uint32_t err =  epos_motor_status(port, nodeid,
                                          &status,
                                          &error,
                                          &position_qc,
                                          &velocity_rpm,
                                          &current_ma);
        if (err) return err;
        fprintf(stderr, "Status: 0x%04x error:0x%02x position: %d, velocity: %d current: %dmA\n",
                status, error, position_qc, velocity_rpm, (int16_t)current_ma);

        fprintf(stderr, "Status bits: 0x%04x = %s\n", status, epos_strstatus(status));
        const char** m;
        for (m = status_bits; *m; ++m, status>>=1)
                if (status & 1)
                        fprintf(stderr, "\t%s\n", *m);

        fprintf(stderr, "Error bits: 0x%02x = (", error);
        for (m = error_bits; *m; ++m, error>>=1)
                if (error & 1)
                        fprintf(stderr, " %s", *m);
        fprintf(stderr, " )\nError History:\n");

        uint32_t errors[5];
        err = epos_get_deverrors(port, nodeid, 5, errors);
        if (err) return err;
        int i;
        for (i = 0; i < 5; i++) {
                fprintf(stderr, "\t%d: %s\n", i, epos_strdeverror(errors[i]));
                if (!errors[i]) break;  // Stop at first NoError
        }


        return 0;
}

int main(int argc, char* argv[]) {

        int ch;
        const char* path_to_port = default_path_to_port;
        int nodeid = 1;
        int debug = 0;
        int target = 0;
        int homing = 0;

        argv0 = argv[0];

        while ((ch = getopt(argc, argv, "NPdhi:s:t:v")) != -1){
                switch (ch) {
                case 'N': homing = 1; break;
                case 'P': homing = 2; break;
                case 'd': ++debug; break;
                case 'i': nodeid = atoi(optarg); break;
                case 's': path_to_port = optarg; break;
                case 'v': ++verbose; break;
                case 't': target = atoi(optarg); break;
                case 'h':
                default:
                        usage();
                }
        }

        if (nodeid < 1 || nodeid > 16) usage();

        if(debug) verbose=3;

        // Open the epos rs232
        int port = epos_open(path_to_port);
        if (port < 0) crash("epos_open(%s)", path_to_port);

        fprintf(stderr, "Probing: ");
        fflush(stderr);
        uint32_t serialnumber = 0;
        uint32_t err = epos_motor_probe(port, nodeid, &serialnumber);
        if (err) crash("Error probing EPOS:%s", epos_strerror(err));

        err = print_status(port, nodeid);
        if (err) crash("Error getting status: %s", epos_strerror(err));

        if (homing) {

                fprintf(stderr, "Configuring Homing Mode: ");
                fflush(stderr);
                struct EposHomingConfig hcfg = {
                         2000,  // max_following_error;
                            0,  // home_offset;
                        10000,  // max_profile_velocity;
                        10000,  // quickstop_deceleration;
                         1000,  // switch_search_speed;
                          500,  // zero_search_speed;
                         1000,  // homing_acceleration;
                         1000,  // current_threshold;
                            0,  // home_position;
                         homing,// homing_method;
                };

                err = epos_motor_homing_init(port, nodeid, &hcfg);
                if (err) crash("Error: %s", epos_strerror(err));
                fprintf(stderr, "Ok\n");

                fprintf(stderr, "Executing Homing Mode: ");
                fflush(stderr);
                err = epos_motor_homing_exec(port, nodeid);
                if (err) crash("Error: %s", epos_strerror(err));
                fprintf(stderr, "Ok\n");


                err = print_status(port, nodeid);
                if (err) crash("Error getting status: %s", epos_strerror(err));

                err = epos_motor_wait_target(port, nodeid, 10000);
                if (err) crash("Error waiting for target reached: %s", epos_strerror(err));

        } else { // move

                fprintf(stderr, "Initializing rudder: ");
                fflush(stderr);
                struct EposPPMConfig cfg = {
                        10000, // max_following_error;     // 0x6065-00  User specific [2000 qc]
                        -1000000,   // min_position_limit;      // 0x607D-01  User specific [-2147483648 qc]
                        +1000000,  // max_position_limit;      // 0x607D-02  User specific [2147483647 qc]
                        1000, // max_profile_velocity;    // 0x607F-00  Motor specific [25000 rpm]
                        5000, // profile_velocity;        // 0x6081-00  Desired Velocity [1000 rpm]
                        1000, // profile_acceleration;    // 0x6083-00  User specific [10000 rpm/s]
                        1000, // profile_deceleration;    // 0x6084-00  User specific [10000 rpm/s]
                        1000, // quickstop_deceleration;  // 0x6085-00  User specific [10000 rpm/s]
                           0, // motion_profile_type;     // 0x6086-00  User specific [0]
                };

                err = epos_motor_ppm_init(port, nodeid, &cfg);
                if (err) crash("Error Initializing PPM: %s", epos_strerror(err));
                fprintf(stderr, "Ok\n");

                err = print_status(port, nodeid);
                if (err) crash("Error getting status: %s", epos_strerror(err));

                fprintf(stderr, "Moving to %d: ", target);
                fflush(stderr);
                err = epos_motor_ppm_execabs(port, nodeid, target);
                if (err) crash("Error executing move: %s", epos_strerror(err));
                fprintf(stderr, "Ok\n");

                err = print_status(port, nodeid);
                if (err) crash("Error getting status: %s", epos_strerror(err));

                err = epos_motor_wait_target(port, nodeid, 10000);
                if (err) crash("Error waiting for target reached: %s", epos_strerror(err));

        }

        return 0;
}
