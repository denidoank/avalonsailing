// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011


#include "helmsman/normal_controller.h"

#include "common/convert.h"
#include "common/polar.h"
#include "helmsman/apparent.h"
#include "helmsman/controller_io.h"
#include "helmsman/rudder_controller_const.h"

#include "lib/testing/testing.h"


void SetEnv(const Polar& wind_true,
            const Polar& boat,
            ControllerInput* in,
            FilteredMeasurements* filtered,
            ControllerOutput* out) {
  in->Reset();
  in->drives.gamma_sail_rad = 0;
  filtered->Reset();
  out->Reset();
  filtered->phi_z_boat = boat.AngleRad();
  filtered->mag_boat = boat.Mag();
  filtered->omega_boat = 0;
  filtered->alpha_true = wind_true.AngleRad();
  filtered->mag_true = wind_true.Mag();

  Apparent(filtered->alpha_true, filtered->mag_true,
           filtered->phi_z_boat, filtered->mag_boat,
           filtered->phi_z_boat,
           &filtered->angle_app, &filtered->mag_app);
}

TEST(NormalController, All) {
  RudderController rudder_controller;
  // coefficients for omega_z, phi_z, int(phi_z)
  rudder_controller.SetFeedback(452.39, 563.75, 291.71, true);

  SailController sail_controller;
  NormalController c(&rudder_controller, &sail_controller);
  // This removes the rate limit for the reference value and allows
  // not sailable directions.
  c.SkipAlphaStarShaping(true);
  ControllerInput in;
  FilteredMeasurements filtered;
  ControllerOutput out;

  Polar wind_true(0, 2);       // Wind vector forward to North, with 2m/s.
  Polar boat(Deg2Rad(0), 1);  // Boat going forward as well, with 1 m/s.
                          // So the apparent wind vector is at about -20 degrees to
                          // the boats x-axis, 1m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);

  in.alpha_star_rad = Deg2Rad(0);
  c.Entry(in, filtered);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_right_rad));
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_right_rad));
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  in.alpha_star_rad = 0.001;

  c.Run(in, filtered, &out);

  EXPECT_FLOAT_EQ(-0.0423465, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  in.alpha_star_rad = 0.0;
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.00208339, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.00208339, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  in.alpha_star_rad = -0.001;
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0.0402631, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  in.alpha_star_rad = 0;
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0.0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));

  wind_true = Polar(-M_PI / 2, 2);  // wind vector to West, with 2m/s
  boat = Polar(0, 2);               // boat going North, with 2 m/s
  // so the apparent wind vector is at -135 degree to
  // the boats x-axis, sqrt(2) * 2m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);

  c.Run(in, filtered, &out);
  // sail opposing the apparent wind.
  EXPECT_FLOAT_EQ(25, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
}

TEST(NormalController, Synthetic) {
  // With the magic test speed and the amplification coefficients set right
  // the rudder angle will be equal to the negatice control error.
  RudderController rudder_controller;
  // coefficients for omega_z, phi_z, int(phi_z)
  rudder_controller.SetFeedback(0, 1000, 0, false);

  SailController sail_controller;
  NormalController c(&rudder_controller, &sail_controller);
  // This removes the rate limit for the reference value and allows
  // not sailable directions.
  c.SkipAlphaStarShaping(true);
  ControllerInput in;
  FilteredMeasurements filtered;
  ControllerOutput out;

  Polar wind_true(0, 2);       // Wind vector forward to North, with 2m/s.
  Polar boat(Deg2Rad(0), kMagicTestSpeed);  // Boat going forward as well, with about 1.1 m/s.
                          // So the apparent wind vector is at about -20 degrees to
                          // the boats x-axis, 1m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);
  // straight ahead, in the forbidden zone around the dead run.
  in.alpha_star_rad = Deg2Rad(0);
  c.Run(in, filtered, &out);
  // The reference value of  is in the forbidden jibe zone and is replaced
  // by +15 degrees. Thus the negative rudder angle.
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_right_rad));
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_right_rad));
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  in.alpha_star_rad = 0.001;

  c.Run(in, filtered, &out);

  EXPECT_FLOAT_EQ(-0.001, out.drives_reference.gamma_rudder_star_left_rad);
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  in.alpha_star_rad = 0.0;
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.0, out.drives_reference.gamma_rudder_star_left_rad);
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.0, out.drives_reference.gamma_rudder_star_left_rad);
  in.alpha_star_rad = -0.001;
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0.001, out.drives_reference.gamma_rudder_star_left_rad);
  in.alpha_star_rad = 0;
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0.0, out.drives_reference.gamma_rudder_star_left_rad);

  wind_true = Polar(-M_PI / 2, 2);  // wind vector to West, with 2m/s
  boat = Polar(0, 2);               // boat going North, with 2 m/s
  // so the apparent wind vector is at -135 degree to
  // the boats x-axis, sqrt(2) * 2m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);

  c.Run(in, filtered, &out);
  // sail opposing the apparent wind.
  EXPECT_FLOAT_EQ(25, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
}

int main(int argc, char* argv[]) {
  NormalController_All();
  NormalController_Synthetic();
  return 0;
}
