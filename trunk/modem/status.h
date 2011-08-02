// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef MODEM_STATUS_H_
#define MODEM_STATUS_H_

#include "proto/imu.h"
#include "proto/modem.h"
#include "proto/wind.h"

#include <string>

using namespace std;

// Build a SMS status message based on latest status from IMU, wind and modem.
string BuildStatusMessage(const time_t status_time,
                          const IMUProto& imu_status,
                          const WindProto& wind_status,
                          const ModemProto& modem_status,
                          const string& status_text);

#endif  // MODEM_STATUS_H_
