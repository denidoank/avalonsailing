// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include <stdio.h>
#include <string>

#include "helmsman/compass_sensor.h"

#include "common/convert.h"
#include "proto/compass.h"


CompassSensor::CompassSensor() {
  Reset();
}

void CompassSensor::Reset() {
  phi_z_rad = 0;
}

std::string CompassSensor::ToString() const {
char line[1024];
int s = snprintf(line, sizeof line,
    "timestamp_ms:0 roll_deg:0 pitch_deg:0 yaw_deg:%.3lf temp_c:22\n",
    Rad2Deg(phi_z_rad));
return std::string(line, s);
}

void CompassSensor::ToProto(CompassProto* compass_proto) const {
  compass_proto->roll_deg = -0.01234;
  compass_proto->pitch_deg = 0;
  compass_proto->yaw_deg = Rad2Deg(phi_z_rad);
  compass_proto->temp_c = 22;
}

