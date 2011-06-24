// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011


#include "helmsman/brake_controller.h"

#include "common/unknown.h"

#include "common/convert.h"
#include "common/polar.h"
#include "helmsman/apparent.h"
#include "helmsman/controller_io.h"

#include "lib/testing/testing.h"


void SetEnv(const Polar& wind_true,
            const Polar& boat,
            ControllerInput* in,
            FilteredMeasurements* filtered,
            ControllerOutput* out) {
  in->Reset();
  filtered->Reset();
  out->Reset();
  filtered->phi_z_boat = boat.AngleRad();
  filtered->mag_boat = boat.Mag();
  filtered->omega_boat = 0;
  filtered->alpha_true = wind_true.AngleRad();
  filtered->mag_true = wind_true.Mag();

  Apparent(filtered->alpha_true, filtered->mag_true,
           filtered->phi_z_boat, filtered->mag_boat,
           &filtered->angle_app, &filtered->mag_app);
}

TEST(BrakeController, All) {
  BrakeController c;
  ControllerInput in;
  FilteredMeasurements filtered;
  ControllerOutput out;

  Polar wind_true(0, 2);  // wind vector forward to North, with 2m/s
  Polar boat(0, 1);       // boat going forward as well, with 1 m/s.
                          // So the apparent wind vector is at 0 degree to
                          // the boats x-axis, 1m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);

  c.Run(in, filtered, &out);

  EXPECT_FLOAT_EQ(80, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_FLOAT_EQ(-80, Rad2Deg(out.drives_reference.gamma_rudder_star_right_rad));
  // sail kept in flag position, opposing the apparent wind.
  EXPECT_FLOAT_EQ(-180, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  wind_true = Polar(-M_PI / 2, 2);  // wind vector to West, with 2m/s
  boat = Polar(0, 2);               // boat going North, with 2 m/s
  // so the apparent wind vector is at -135 degree to
  // the boats x-axis, sqrt(2) * 2m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);

  c.Run(in, filtered, &out);
  // sail opposing the apparent wind.
  EXPECT_FLOAT_EQ(45, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
}

int main(int argc, char* argv[]) {
  BrakeController_All();
  return 0;
}

