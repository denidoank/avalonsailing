// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2012

#ifndef HELMSMAN_GPS_H
#define HELMSMAN_GPS_H

#include <string>
#include "proto/gps.h"

// Original units from GPS.
struct Gps {
  Gps();
  void Reset();
  std::string ToStringZZZ() const;
  void ToProto(GPSProto* gps_proto) const;
  void FromProto(const GPSProto& gps_proto);

  // GPS-Data
  double longitude_deg;
  double latitude_deg;
  // COG velocity
  double speed_m_s;  // boat forward speed over ground, in m/s
  // Possible additional compass input (but this is COG, not phi_z)
  // and at standstill both are very different. So we could use
  // cog_rad at higher speeds only, but how reliable is that speed
  // information?
  double cog_rad;
  bool valid;
};

#endif  // HELMSMAN_GPS_H
