// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2012
#include "gps.h"

#include <stdio.h>
#include "common/convert.h"
#include "common/normalize.h"

Gps::Gps() {
  Reset();
}

void Gps::Reset() {
  latitude_deg = 0;
  longitude_deg = 0;
  speed_m_s = 0;
  cog_rad = 0;
  valid = false;
}

std::string Gps::ToStringZZZ() const {
  char line[20*25];
  int s = snprintf(line, sizeof line, "latitude_deg:%lf longitude_deg:%lf "
      "cog_deg:%.4lf speed_m_s:%.4lf valid:%d\n",
      longitude_deg, latitude_deg,
      Rad2Deg(cog_rad), speed_m_s, valid);
  return std::string(line, s);
}

void Gps::ToProto(GPSProto* gps_proto) const {
  if (!valid) return;
  gps_proto->cog_deg = NormalizeDeg(Rad2Deg(cog_rad));
  gps_proto->lat_deg = latitude_deg;
  gps_proto->lng_deg = longitude_deg;
  gps_proto->speed_m_s = speed_m_s;
}

void Gps::FromProto(const GPSProto& gps_proto) {
  latitude_deg  = gps_proto.lat_deg;
  longitude_deg = gps_proto.lng_deg;

  // x-Speed is COG speed and always positive.
  speed_m_s = gps_proto.speed_m_s;
  cog_rad = NormalizeRad(Deg2Rad(gps_proto.cog_deg));
  valid = true;
}
