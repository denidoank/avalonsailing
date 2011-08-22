// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/rudder_controller.h"

#include <algorithm>
#include "common/convert.h"
#include "helmsman/controller_io.h"
#include "lib/testing/testing.h"

void Expect(double sail_deg,
            double r_left_deg,
            double r_right_deg,
            const ControllerOutput& out) {
  EXPECT_FLOAT_EQ(Deg2Rad(sail_deg), out.drives_reference.gamma_sail_star_rad);
  EXPECT_FLOAT_EQ(Deg2Rad(r_left_deg),
                  out.drives_reference.gamma_rudder_star_left_rad);
  EXPECT_FLOAT_EQ(Deg2Rad(r_right_deg),
                  out.drives_reference.gamma_rudder_star_right_rad);
}


TEST(RudderController, All) {
  RudderController c;
  double phi_star = 0;
  double omega_star = 0;
  double phi = 0;
  double omega = 0;
  double speed = 2;
  double gamma = -1;

  for (int i = 0; i < 300; ++i) {
    c.Control(phi_star, omega_star,
              phi, omega,
              speed, &gamma);
    EXPECT_FLOAT_EQ(0, gamma);  // no creeping integration error
  }

  c.Control(1, omega_star,
            phi, omega,
            speed, &gamma);
  // at limit
  EXPECT_FLOAT_EQ(-0.15708, gamma);
  c.Control(1, omega_star,
            phi, omega,
            speed, &gamma);
  EXPECT_FLOAT_EQ(-0.15708, gamma);
  c.Control(1, omega_star,
            phi, omega,
            speed, &gamma);
  EXPECT_FLOAT_EQ(-0.15708, gamma);
  c.Control(1, omega_star,
            phi, omega,
            speed, &gamma);
  EXPECT_FLOAT_EQ(-0.15708, gamma);

  c.Reset();
  c.Control(phi_star, omega_star,
            phi, omega,
            speed, &gamma);
  EXPECT_FLOAT_EQ(0, gamma);
  double p_part = -0.0175681;
  double i_part = -0.000909052;
  for (int t = 1; t < 30; ++t) {
    c.Control(0.1, omega_star,
              phi, omega,
              speed, &gamma);
    EXPECT_FLOAT_EQ(p_part + t * i_part, gamma);
  }

  c.Reset();
  c.Control(phi_star, omega_star,
            phi, omega,
            speed, &gamma);
  EXPECT_FLOAT_EQ(0, gamma);
  p_part = 3 * -0.0175681;
  i_part = 3 * -0.000909052;
  // run into limit at -9 degree
  for (int t = 1; t < 20; ++t) {
    c.Control(0.3, omega_star,
              phi, omega,
              speed, &gamma);
    EXPECT_FLOAT_EQ(std::max(Deg2Rad(-9), p_part + t * i_part), gamma);
  }
  // test of anti wind-up, control angle shall not be stuck at -9 degrees when
  // the reference value changes.
  // Stop before the integral part gets clamped!
  for (int t = 1; t < 20; ++t) {
    c.Control(-0.3, omega_star,
              phi, omega,
              speed, &gamma);
    EXPECT_LE(Deg2Rad(-9), gamma);
  }

  c.Reset();
  c.Control(0.1, 0,
            phi, omega,
            speed, &gamma);
  EXPECT_FLOAT_EQ(-0.0184771, gamma);
  c.Reset();
  c.Control(0.1, 0.1,
            phi, omega,
            speed, &gamma);
  EXPECT_FLOAT_EQ(-0.103977, gamma);  //

  c.Reset();
  // less speed, bigger angle.
  c.Control(0.1, 0.1,
            phi, omega,
            speed / 2, &gamma);
  EXPECT_FLOAT_EQ(-0.272384, gamma);  //

  c.Reset();
  // Even less speed, bigger angle at gamma_0 limit.
  c.Control(0.1, 0.1,
            phi, omega,
            0.2 * speed, &gamma);
  // -27 degrees (7 from NACA profile, 20 for gamma_0) would be 0.471238898038469
  // ?
  // TODO: Check this result.
  EXPECT_FLOAT_EQ(-0.46562, gamma);

  c.Reset();
  // Negative speed, at limit.
  c.Control(0.1, 0.1,
            phi, omega,
            -0.2 * speed, &gamma);
  EXPECT_FLOAT_EQ(0.168914, gamma);
}

int main(int argc, char* argv[]) {
  RudderController_All();
  return 0;
}
