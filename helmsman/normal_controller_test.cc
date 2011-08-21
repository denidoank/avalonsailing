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
  SailController sail_controller;
  NormalController c(&rudder_controller, &sail_controller);
  ControllerInput in;
  FilteredMeasurements filtered;
  ControllerOutput out;

  Polar wind_true(0, 2);  // wind vector forward to North, with 2m/s
  Polar boat(0, 1);       // boat going forward as well, with 1 m/s.
                          // So the apparent wind vector is at 0 degree to
                          // the boats x-axis, 1m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);
  // straight ahead
  in.alpha_star_rad = 0;
  c.Run(in, filtered, &out);

  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_right_rad));
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  c.Run(in, filtered, &out);
  c.Run(in, filtered, &out);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_right_rad));
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  in.alpha_star_rad = 0.01;

  c.Run(in, filtered, &out);
  // Values depend on the control amplification coefficients, so we have to
  // relax the test conditions later.
  EXPECT_FLOAT_EQ(452.39, kStateFeedback1);
  EXPECT_FLOAT_EQ(563.75, kStateFeedback2);
  EXPECT_FLOAT_EQ(291.71, kStateFeedback3);
  

  EXPECT_FLOAT_EQ(-0.295634, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  in.alpha_star_rad = 0.0;
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.0145448, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.0145448, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  in.alpha_star_rad = -0.01;
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0.281089, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  in.alpha_star_rad = 0;
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0.0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));

 
  
  /*
  wind_true = Polar(-M_PI / 2, 2);  // wind vector to West, with 2m/s
  boat = Polar(0, 2);               // boat going North, with 2 m/s
  // so the apparent wind vector is at -135 degree to
  // the boats x-axis, sqrt(2) * 2m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);

  c.Run(in, filtered, &out);
  // sail opposing the apparent wind.
  EXPECT_FLOAT_EQ(45, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  */
}

int main(int argc, char* argv[]) {
  NormalController_All();
  return 0;
}

