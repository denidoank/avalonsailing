// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

#ifndef COMMON_POLAR_DIAGRAM_H_
#define COMMON_POLAR_DIAGRAM_H_

// After tests in wind speed ranges of 2-10m/s I defined
// this approximation. Our speed limit is around 2.5m/s.

// If the wind speed is unknown, give 10.0 for the wind speed to get the
// relative speed of the boat for the given angle.
// See http://www.oppedijk.com/zeilen/create-polar-diagram or
// http://www.scribd.com/doc/13811789/Segeln-gegen-den-Wind

// angle_true_wind_deg is the angle to to where the wind is coming from,
// i.e. 180 degrees is running or sailing into the same direction
// as the wind blows.
void ReadPolarDiagram(double angle_true_wind_deg,
                      double wind_speed_m_s,
                      bool* dead_zone_tack,
                      bool* dead_zone_jibe,
                      double* speed_m_s);

// in degrees
double TackZoneDeg();
double JibeZoneDeg();
// in radians
double TackZoneRad();
double JibeZoneRad();  // about 160 in radians
double JibeZoneHalfWidthRad();  // Returns the more plausible 20 degrees in radians.

double BestSailableHeading(double alpha_star, double alpha_true);
double BestSailableHeadingDeg(double alpha_star_deg, double alpha_true_deg);
double BestStableSailableHeading(double alpha_star, double alpha_true, double previous_output);

// Calculate the sailable heading under the given circumstances.
typedef enum Sector {
  TackPort = 1,
  TackStar,
  ReachStar,
  JibeStar,
  JibePort,
  ReachPort
} SectorT;



// target is not written for sector == ReachStar or target == ReachPort
double SailableHeading(double alpha_star,    // desired heading alpha*
                       double alpha_true,    // true wind vector direction
                       double alpha_app,     // apparent wind vector direction
                       double mag_app,       // apparent wind vector magnitude
                       double phi_z,         // boat direction
                       double previous_output,  // previous output direction, needed to implement hysteresis
                       SectorT* sector,      // sector codes for state handling and maneuver
                       double* target);      // target angle for maneuver
#endif  // COMMON_POLAR_DIAGRAM_H_
