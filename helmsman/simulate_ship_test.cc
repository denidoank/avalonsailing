// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

// This test just checks that we are able to pick up speed
// within 40 s. The initial controller is tested.


#include "helmsman/boat_model.h"
#include "helmsman/ship_control.h"

#include <math.h>
#include <stdio.h>
#include "common/convert.h"
#include "common/delta_angle.h"
#include "common/polar.h"
#include "helmsman/sampling_period.h"
#include "lib/testing/testing.h"

extern int logging_aoa;
extern int debug;

void InitialControllerTest(double wind_direction_deg,
                           double expected_min_speed_m_s) {
  logging_aoa = 0;
  debug = 0;
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed
                  -M_PI / 2);       // gamma_sail_ / rad, e.g. -90 degrees here
  DriveReferenceValuesRad ref;
  Polar true_wind(Deg2Rad(wind_direction_deg), 10);  // forward wind, 10m/s
  ControllerInput in;
  ControllerOutput out;
  in.alpha_star_rad = Deg2Rad(90);         // want to go east

  ShipControl::Reset();
  ShipControl::Docking();              // The consequential Entry into the
                                       // Initial state allows to reset the
                                       // Initial controller.
  ShipControl::Run(in, &out);
  ShipControl::Normal();
  printf("\nWind direction: %lg degree\n", wind_direction_deg);
  double t;
  for (t = 0; t < 50; t += kSamplingPeriod) {
    model.Simulate(out.drives_reference,
                   true_wind,
                   &in);
    ShipControl::Run(in, &out);
    // model.Print(t);
  }
  printf("\n");
  model.PrintHeader();
  model.Print(t);
  EXPECT_LT(expected_min_speed_m_s, model.GetSpeed());
}


TEST(SimShip, Wind_0) {
  InitialControllerTest(-179,  // wind vector direction, in degrees
                        0.7);  // minimum speed, m/s

  // all initial wind directions are handled correctly.
  for (double wind_direction = -180; wind_direction < 180; wind_direction += 1)
    InitialControllerTest(wind_direction, 0.6);  // speeds vary around 0.9 m/s.
}

int main(int argc, char* argv[]) {
  SimShip_Wind_0();
  return 0;
}

