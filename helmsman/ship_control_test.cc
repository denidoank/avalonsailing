// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/ship_control.h"

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


TEST(ShipControl, All) {

  ControllerInput in;
  in.Reset();

  ControllerOutput out;
  out.Reset();

  ShipControl::Run(in, &out);

  Expect(0, 0, 0, out);

  ShipControl::Brake();

  ShipControl::Run(in, &out);
  Expect(0, 80, -80, out);

  ShipControl::Brake();

  ShipControl::Run(in, &out);
  Expect(0, 80, -80, out);

  ShipControl::Docking();

  ShipControl::Run(in, &out);
  Expect(0, 0, 0, out);
}

int main(int argc, char* argv[]) {
  ShipControl_All();
  return 0;
}
