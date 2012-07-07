// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011


#include "wind_sensor.h"

#include <stdio.h>
#include <string>

#include "proto/wind.h"


// Original units from Wind sensor, relative to mast.

WindSensor::WindSensor() {
  Reset();
}

void WindSensor::Reset() {
  mag_m_s = 0;  
  alpha_deg = 0;
  valid = false;
}

std::string WindSensor::ToString() const {
char line[1024];
int s = snprintf(line, sizeof line, "mag_m_s:%f alpha_deg:%f valid:%d\n",
                 mag_m_s, alpha_deg, valid ? 1 : 0);
return std::string(line, s);
}

void WindSensor::ToProto(WindProto* wind_sensor_proto) const {
  wind_sensor_proto->angle_deg = alpha_deg;
  wind_sensor_proto->speed_m_s = mag_m_s;
  wind_sensor_proto->valid = valid;
}

