// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011


#include "helmsman/boat_model.h"
#include "helmsman/ship_control.h"

#include <math.h>
#include <stdio.h>
#include "common/convert.h"
#include "common/delta_angle.h"
#include "common/polar.h"


#include "lib/testing/testing.h"


namespace {
const double kSamplingPeriod = 0.1;
}

extern int debug;
static int print_model = 0;
void NormalControllerTest(double wind_direction_deg,
                          double expected_min_speed_m_s,
                          double wind_strength_m_s = 10) {
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed
                  -M_PI / 2);       // gamma_sail_ / rad, e.g. -90 degrees here
  DriveReferenceValuesRad ref;
  Polar true_wind(Deg2Rad(wind_direction_deg), wind_strength_m_s);  // forward wind, 10m/s
  ControllerInput in;
  ControllerOutput out;
  in.alpha_star_rad = Deg2Rad(90);         // want to go east

  ShipControl::Reset();
  ShipControl::Docking();              // The consequential Entry into the
                                       // Initial state allows to reset the
                                       // Initial controller.
  ShipControl::Run(in, &out);
  ShipControl::Normal();
  printf("\nWind direction: %g degree, Wind speed: %g m/s\n",
          wind_direction_deg, wind_strength_m_s);
  double t;
  double t_final = wind_strength_m_s < 5 ? 250 : 240;

  for (t = 0; t < t_final; t += kSamplingPeriod) {
    model.Simulate(out.drives_reference,
                   true_wind,
                   &in);
    ShipControl::Run(in, &out);
    if ((print_model && t > 99.0) || debug)
      model.Print(t);
  }
  printf("\n");
  model.PrintHeader();
  model.Print(t);
  EXPECT_LT(expected_min_speed_m_s, model.GetSpeed());
  if (fabs(model.GetPhiZ() - in.alpha_star_rad) > Deg2Rad(5))
    printf("final heading %g for wind direction %g\n", Rad2Deg(model.GetPhiZ()), wind_direction_deg);
}


TEST(SimShip, Wind_1) {
  NormalControllerTest(-165,   // wind vector direction, in degrees
                       1.0);   // minimum final boat speed, m/s
  // All initial wind directions are handled correctly.
  for (double wind_direction = -180; wind_direction < 180; wind_direction += 0.3) {
    if (fabs(wind_direction - -90) < 20)  // Not sailable.
      continue;
    NormalControllerTest(wind_direction, 1.0);  // speeds vary from 1.0 to 2.1 m/s
  }
}

TEST(SimShip, Wind_2) {
  // Cannot sail with less than 0.65m/s wind strength.
  for (double wind_strength = 2; wind_strength < 20; wind_strength *= 1.15)
  // All initial wind directions are handled correctly.
  for (double wind_direction = -180; wind_direction < 180; wind_direction += 0.3) {
    if (fabs(wind_direction - -90) < 20)  // Not sailable.
      continue;
    // speeds vary due to different wind strength and headings.
    // If our model was correct, we would get the polar diagram here.
    NormalControllerTest(wind_direction, 0.08 * wind_strength, wind_strength);
  }
}

TEST(SimShip, Wind_Strong) {
  for (double wind_strength = 10; wind_strength < 20; wind_strength *= 1.5)
    // All initial wind directions are handled correctly.
    for (double wind_direction = -180; wind_direction < 180; wind_direction += 0.3) {
      if (fabs(wind_direction - -90) < 20)  // Not sailable.
        continue;
      if (fabs(wind_direction - 31.8 ) < 0.1 && fabs(wind_strength - 20) < 0.1) {
        continue;
        debug = 1;
        print_model = 1;
      }
      // speeds vary due to different wind strength and headings.
      // If our model was correct, we would get the polar diagram here.
      NormalControllerTest(wind_direction, 0.08 * wind_strength, wind_strength);
      debug = 0;
    }
}

TEST(SimShip, Wind_Debug) {
  debug = 0;
  NormalControllerTest(31.8,     // wind vector direction, in degrees
                       2,        // minimum final boat speed, m/s
                       16.2741232583); // wind speed, m/s
  debug = 0;
}


int main(int argc, char* argv[]) {
  debug = 0;
  SimShip_Wind_Strong();
  SimShip_Wind_Debug();
  SimShip_Wind_1();
  SimShip_Wind_2();
  return 0;
}

