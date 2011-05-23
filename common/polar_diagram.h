// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

#ifndef COMMON_POLAR_DIAGRAM_H_
#define COMMON_POLAR_DIAGRAM_H_

// In reality the polar diagram of the boat speeds (and the zone angles) depend
// on the wind speed. We could not measure them so far, so we define a flexible
// interface and use an approximation.
// If the wind speed is unknown, give 10.0 for the wind speed to get the
// relative speed of the boat for the given angle.
// See http://www.oppedijk.com/zeilen/create-polar-diagram or
// http://www.scribd.com/doc/13811789/Segeln-gegen-den-Wind

void ReadPolarDiagram(double angle_true_wind,
                      double wind_speed,
                      bool* dead_zone_tack,
                      bool* dead_zone_jibe,
                      double* speed);


double TackZone();

double JibeZone();
#endif  // COMMON_POLAR_DIAGRAM_H_
