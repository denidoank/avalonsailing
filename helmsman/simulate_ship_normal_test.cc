// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

// Test with runtime measurements.
// ==============================

#include "helmsman/boat_model.h"
#include "helmsman/ship_control.h"

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include "common/convert.h"
#include "common/delta_angle.h"
#include "common/polar.h"
#include "common/now.h"
#include "helmsman/sampling_period.h"
#include "lib/testing/testing.h"

extern int debug;
extern int logging_aoa;

static int print_model = 0;

void NormalControllerTest(double wind_direction_deg,
                          double expected_min_speed_m_s,
                          double wind_strength_m_s = 10) {
  logging_aoa = 0;
  debug = 0;
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
  printf("\nWind direction: %6.2lf degree, Wind speed: %4.2lf m/s\n",
          wind_direction_deg, wind_strength_m_s);
  int64_t micros_max = 0;
  int64_t micros_sum = 0;
  int rounds = 0;
  double t;
  double t_final = wind_strength_m_s < 5 ? 450 : 240; // weak winds make the slow tack fail.
                                                      // Better make jibes only for slow winds?.

  if (print_model)
    model.PrintHeader();
  for (t = 0; t < t_final; t += kSamplingPeriod) {
    model.Simulate(out.drives_reference,
                   true_wind,
                   &in);
    int64_t start = now_micros();
    ShipControl::Run(in, &out);
    int64_t micros = now_micros() - start;
    ++rounds;
    micros_sum += micros;
    micros_max = std::max(micros_max, micros);
    if ((print_model) || debug)                                 /* && t > 99.0 */
      model.Print(t);
  }
  printf("\n");
  model.PrintHeader();
  model.Print(t);
  EXPECT_LT(expected_min_speed_m_s, model.GetSpeed());
  if (fabs(model.GetPhiZ() - in.alpha_star_rad) > Deg2Rad(5))
    printf("final heading %lg for wind direction %lg\n", Rad2Deg(model.GetPhiZ()), wind_direction_deg);
  if (out.status.inits > 1) {
    printf("Extra inits: %d\n", out.status.inits);
  }
  printf("\nRuntimes/microseconds\n=================\n");
  printf("Average:    %6.4lf micros\n", micros_sum / static_cast<double>(rounds));
  printf("Max:        %lld micros\n", micros_max);
  printf("\n");
}


TEST(SimShip, Wind_1) {
  NormalControllerTest(-165,   // wind vector direction, in degrees
                       0.8);   // minimum final boat speed, m/s
  // All initial wind directions are handled correctly.
  for (double wind_direction = -180; wind_direction < 180; wind_direction += 0.3) {
    if (fabs(wind_direction - -90) < 20)  // Not sailable.
      continue;
    NormalControllerTest(wind_direction, 0.8);  // speeds vary from 0.8 to 2.1 m/s
  }
}

TEST(SimShip, Wind_0) {
  // Cannot sail with less than 2m/s wind strength.
  for (double wind_strength = 1; wind_strength < 20; wind_strength *= 1.15) {
    // All initial wind directions are handled correctly.
    for (double wind_direction = -180; wind_direction < 180; wind_direction += 0.3) {
      if (fabs(wind_direction - -90) < 20)  // Not sailable
        continue;
      // put failing cases here
      if (0 && fabs(wind_direction - 12 ) < 0.1 && fabs(wind_strength - 16.37) < 0.1) {
        debug = 1;
        print_model = 1;
      } else {
        debug = 0;
        print_model = 0;
      }
      // speeds vary due to different wind strength and headings.
      // If our model was correct, we would get the polar diagram here.
      NormalControllerTest(wind_direction, std::min(0.06 * wind_strength, 2.5), wind_strength);
    }
  }
}

TEST(SimShip, Wind_2) {
  // Cannot sail with less than 2m/s wind strength.
  for (double wind_strength = 1; wind_strength < 20; wind_strength *= 1.15)
  // All initial wind directions are handled correctly.
  for (double wind_direction = -180; wind_direction < 180; wind_direction += 0.3) {
    if (fabs(wind_direction - -90) < 20)  // Not sailable.
      continue;
    /*
    if (fabs(wind_direction - -180 ) < 0.1 && fabs(wind_strength - 2.6) < 0.1) {
      //debug = 1;
      print_model = 1;
      //continue;
    } else {
      print_model = 0;
    }
    */
    // speeds vary due to different wind strength and headings.
    // If our model was correct, we would get the polar diagram here.
    NormalControllerTest(wind_direction, std::min(0.06 * wind_strength, 2.5), wind_strength);
  }
}

TEST(SimShip, Wind_Strong) {
  for (double wind_strength = 10; wind_strength < 20; wind_strength *= 1.5)
    // All initial wind directions are handled correctly.
    for (double wind_direction = -180; wind_direction < 180; wind_direction += 0.3) {
      if (fabs(wind_direction - -90) < 20)  // Not sailable.
        continue;
      /*
      if (fabs(wind_direction - 31.8 ) < 0.1 && fabs(wind_strength - 20) < 0.1) {
        debug = 1;
        print_model = 1;
      } else {
        debug = 0;
        print_model = 0;
      }
      */
      // speeds vary due to different wind strength and headings.
      // If our model was correct, we would get the polar diagram here.
      NormalControllerTest(wind_direction, std::min(0.06 * wind_strength, 2.5), wind_strength);
      debug = 0;
    }
}

TEST(SimShip, Wind_Debug) {
  debug = 0;
  NormalControllerTest(31.8,           // wind vector direction, in degrees
                       1.5,            // minimum final boat speed, m/s
                       16.2741232583); // wind speed, m/s
  debug = 0;
}


int main(int argc, char* argv[]) {
  debug = 0;
  SimShip_Wind_0();
  SimShip_Wind_2();
  SimShip_Wind_Strong();
  SimShip_Wind_Debug();
  SimShip_Wind_1();
  return 0;
}
