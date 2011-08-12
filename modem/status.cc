// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "modem/status.h"
#include "common/convert.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

char CharOfIntLT64(const int x) {
  if (x < 0)
    return '?';
  if (x < 10)
    return '0' + x;
  if (x < 36)
    return 'A' + (x - 10);
  if (x < 62)
    return 'a' + (x - 36);
  // x >= 63
  return '{';
}

// SMS Status Example:
// 08184459GN451259W0011959342150912n3Bb000OtherStatusInfoInPlainText
//
// where:
// 1. GMT date and time (8 digits)
//    08184459 = day of the month (08) +
//               GMT hour (18) + GMT minute (44) + GMT second (59)
//
// 2. Position (16 chars)
//    GN451259W0021959 = G=GPS, B=Backup GPS, I=Iridium or X=Not available
//                       North 45 degrees 12 minutes 59 seconds,
//                       West 1 degrees 19 minutes 59 seconds
//
// 3. Skipper bearing (3 digits)
//    342 = 342 degrees (direction North-West-North)
//
// 4. Speed in .1 knot increments (2 digits)
//    15 = 1.5 knots
//
// 5. Wind speed average to 10 closest degrees and speed in knots:
//    0912 = Wind average direction 090 +/-5 degrees, wind speed is 12 knots
//
// 6. Battery charge (1 digit) and fuel cell runtime (1 digit)
//    Values are encoded using: 0-9 A-Z a-z = 62 values, '{' = 63 or more
//    Battery charge: decoded value / 2 volts (e.g. 'n' -> 49 -> 24.5V)
//    Fuel cell runtime: decoded value / 2 hours (e.g. '3' -> 3 -> 1.5h)
//
// 7. Number of tacks/jibes:
//    B = 11 tacks: (0-9 A-Z a-z = 62 values, '{' = 63 or more tacks)
//    b = 37 jibes: (0-9 A-Z a-z = 62 values, '{' = 63 or more jibes)
//
// 8. Hardware/Software failures
//    000 = To be defined (15 bits)
//          5 bits are encoded per character (0-9 A-Z a-z = 62 values, '{' = 63)
//          hw/sw components are: main sail engine, left ruder, right rudder,
//          inertial (imu), radar (ais), gps, modem (Iridium)
//
// === TOTAL 40 characters for basic status information ===
string BuildStatusMessage(const time_t status_time,
                          const IMUProto& imu_status,
                          const WindProto& wind_status,
                          const ModemProto& modem_status,
                          const HelmsmanCtlProto& helmsman_status,
                          const FuelcellProto& fuelcell_status,
                          const string& status_text) {
  // 1. GMT date and time (offset 0).
  char status[1024];
  struct tm broken_time;
  gmtime_r(&status_time, &broken_time);
  sprintf(status, "%02d%02d%02d%02d", broken_time.tm_mday, broken_time.tm_hour,
          broken_time.tm_min, broken_time.tm_sec);

  // 2. Position (offset 8).
  strcat(status + 8, "XXXXXXXXXXXXXXXX");
  double lat = 0.0;
  double lng = 0.0;
  if (status_time - modem_status.position_timestamp_s < 12 * 3600) {
    // Valid if modem geolocation timestamp is less than 12h old.
    status[8] = 'I';  // Iridium.
    lat = modem_status.coarse_position_lat;
    lng = modem_status.coarse_position_lng;     
  }
  // TODO: Add backup GPS location
  // IMU based GPS (main GPS).
  if (imu_status.timestamp_ms != 0) {
    status[8] = 'G';  // GPS based position
    lat = imu_status.lat_deg;
    lng = imu_status.lng_deg;
  }
  if (status[8] != 'X' && !isnan(lat) && !isnan(lng)) {
    sprintf(status + 9, "%c%02d%02d%02d%c%03d%02d%02d",
            lat >= 0.0 ? 'N' : 'S',
            static_cast<int>(floor(fabs(lat))) % 90,
            static_cast<int>(floor(fabs(lat) * 60.0)) % 60,
            static_cast<int>(floor(fabs(lat) * 3600.0)) % 60,
            lng >= 0.0 ? 'E' : 'W',
            static_cast<int>(floor(fabs(lng))) % 180,
            static_cast<int>(floor(fabs(lng) * 60.0)) % 60,
            static_cast<int>(floor(fabs(lng) * 3600.0)) % 60);
  }

  // 3. Skipper bearing (offset 24).
  strcat(status + 24, "XXX");
  if (!isnan(helmsman_status.alpha_star_deg)) {
    const double bearing = round(helmsman_status.alpha_star_deg);
    sprintf(status + 24, "%03d", (static_cast<int>(bearing) + 360) % 360);
  }

  // 4. Speed in .1 knot increments (offset 27)
  strcat(status + 27, "XX");
  double speed = NAN;
  if (!isnan(speed)) {
    if (speed >= 10.0) {
      strcat(status + 27, "++");
    } else {
      sprintf(status + 27, "%02d", static_cast<int>(floor(speed * 10.0)));
    }
  }

  // 5. Wind speed average to 10 closest degrees and speed in knots (offset 29).
  // TODO(mariusv): Update wind direction and speed from helmsmann
  strcat(status + 29, "XXXX");
  if (!isnan(wind_status.angle_deg))
    sprintf(status + 29, "%02d",
            (static_cast<int>(wind_status.angle_deg) % 360) / 10);
  if (!isnan(wind_status.speed_m_s)) {
    sprintf(status + 31, "%02d", static_cast<int>(
        MeterPerSecondToKnots(wind_status.speed_m_s)) % 100);
  }

  // 6. Battery charge (1 digit) and fuel cell supply (offset 33).
  strcat(status + 33, "??");
  if (!isnan(fuelcell_status.tension_V)) {
    status[33] =
        CharOfIntLT64(static_cast<int>(fuelcell_status.tension_V * 2.0));
  }
  if (!isnan(fuelcell_status.runtime_h)) {
    status[34] =
        CharOfIntLT64(static_cast<int>(fuelcell_status.runtime_h * 2.0) % 64);
  }

  // 7. Number of tacks/jibes (offset 35).
  strcat(status + 35, "??");

  // 8. Hardware/Software failures (offset 37).
  strcat(status + 37, "000");

  // Other information in plain text (offset 40).
  strncat(status + 40, status_text.c_str(), 160 - 40);

  return status;
}
