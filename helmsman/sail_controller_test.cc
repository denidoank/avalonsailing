// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/sail_controller.h"

#include <iostream>
#include <math.h>

#include "common/convert.h"
#include "helmsman/sail_controller_const.h"
#include "helmsman/sampling_period.h"  // kSamplingPeriod
#include "lib/testing/testing.h"

using namespace std;

TEST(SailController, Basic) {
  SailController controller;
  EXPECT_LT(Deg2Rad(5), controller.GetOptimalAngleOfAttack());


  double aoa_optimal = Deg2Rad(10);
  controller.SetOptimalAngleOfAttack(aoa_optimal);
  EXPECT_FLOAT_EQ(Deg2Rad(10), controller.GetOptimalAngleOfAttack());

  EXPECT_EQ(Deg2Rad(-93), controller.BestGammaSail(Deg2Rad(0), 5));
  EXPECT_FLOAT_EQ(Deg2Rad(-91), controller.BestGammaSail(Deg2Rad( 4), 5));
  EXPECT_FLOAT_EQ(Deg2Rad( 91), controller.BestGammaSail(Deg2Rad(-4), 5));

  EXPECT_FLOAT_EQ(Deg2Rad(-70), controller.BestGammaSail(Deg2Rad(100), 5));
  EXPECT_FLOAT_EQ(Deg2Rad(-80), controller.BestGammaSail(Deg2Rad(90), 5));
  EXPECT_FLOAT_EQ(Deg2Rad(-90), controller.BestGammaSail(Deg2Rad(80), 5));

  EXPECT_FLOAT_EQ(Deg2Rad(84), controller.BestGammaSail(Deg2Rad(-86), 10));


  double alpha_wind = kSwitchpoint;
  EXPECT_FLOAT_EQ(kSwitchpoint - M_PI + aoa_optimal,
                  controller.BestGammaSail(kSwitchpoint + 1E-6, 5));
  EXPECT_FLOAT_EQ(0.5 * kSwitchpoint - kDragMax,
                  controller.BestGammaSail(kSwitchpoint - 1E-6, 5));

  EXPECT_FLOAT_EQ(0.05 - kDragMax, controller.BestGammaSail(0.1, 5));

  EXPECT_FLOAT_EQ(alpha_wind - M_PI + aoa_optimal,
                  controller.BestStabilizedGammaSail( alpha_wind, 5));

  EXPECT_FLOAT_EQ(-(alpha_wind - M_PI + aoa_optimal),
                  controller.BestStabilizedGammaSail(-alpha_wind, 5));

  // No wind case.
  EXPECT_FLOAT_EQ(0, controller.BestStabilizedGammaSail(-alpha_wind, 0));
  // Switch to spinnaker mode is delayed.
  EXPECT_FLOAT_EQ(alpha_wind - M_PI + aoa_optimal,
                  controller.BestStabilizedGammaSail( alpha_wind, 5));

  alpha_wind = kSwitchpoint - 1.1 * kHalfHysteresis;  // in SPINAKER region now
  int delay = static_cast<int>(kSwitchBackDelay / kSamplingPeriod + 0.5);
  for(int i = 0; i < delay; ++i)
    EXPECT_FLOAT_EQ(alpha_wind - M_PI + aoa_optimal,
                    controller.BestStabilizedGammaSail( alpha_wind, 5));
  EXPECT_FLOAT_EQ(0.5 * alpha_wind - kDragMax,
                  controller.BestStabilizedGammaSail( alpha_wind, 5));

  // and switch back.
  alpha_wind = kSwitchpoint + 1.1 * kHalfHysteresis;  // in WING region now
  for(int i = 0; i < delay; ++i)
    EXPECT_FLOAT_EQ(0.5 * alpha_wind - kDragMax,
                    controller.BestStabilizedGammaSail( alpha_wind, 5));
  EXPECT_FLOAT_EQ(alpha_wind - M_PI + aoa_optimal,
                  controller.BestStabilizedGammaSail( alpha_wind, 5));

  // Now the wind changes drastically and we switch immediately.
  alpha_wind = kSwitchpoint - 2.1 * kHalfHysteresis;  // far in the SPINAKER region now
  EXPECT_FLOAT_EQ(0.5 * alpha_wind - kDragMax,
                  controller.BestStabilizedGammaSail( alpha_wind, 5));

  // and switch back.
  alpha_wind = kSwitchpoint + 2.1 * kHalfHysteresis;  // far in the WING region now
  EXPECT_FLOAT_EQ(alpha_wind - M_PI + aoa_optimal,
                  controller.BestStabilizedGammaSail( alpha_wind, 5));
}


TEST(SailController, Reverse) {
  SailController controller;
  double aoa_optimal = Deg2Rad(10);
  controller.SetOptimalAngleOfAttack(aoa_optimal);

  EXPECT_FLOAT_EQ(Deg2Rad( 90), controller.BestGammaSailForReverseMotion(Deg2Rad(180 + 4), 5));
  EXPECT_FLOAT_EQ(Deg2Rad(-90), controller.BestGammaSailForReverseMotion(Deg2Rad(180 - 4), 5));

  EXPECT_FLOAT_EQ(Deg2Rad(-100), controller.BestGammaSailForReverseMotion(Deg2Rad(90), 5));
  EXPECT_FLOAT_EQ(Deg2Rad(-90), controller.BestGammaSailForReverseMotion(Deg2Rad(100), 5));
  // spinaker
  EXPECT_FLOAT_EQ(Deg2Rad(-90), controller.BestGammaSailForReverseMotion(Deg2Rad(110), 5));
  EXPECT_FLOAT_EQ(Deg2Rad(-90), controller.BestGammaSailForReverseMotion(Deg2Rad(120), 5));

  EXPECT_FLOAT_EQ(1.81514, controller.BestGammaSailForReverseMotion(Deg2Rad(-86), 10));
  EXPECT_FLOAT_EQ(-1.81514, controller.BestGammaSailForReverseMotion(Deg2Rad(86), 10));
}

TEST(SailController, Storm) {
  SailController controller;
  double optimal_deg = 10;
  controller.SetOptimalAngleOfAttack(Deg2Rad(optimal_deg));

  EXPECT_FLOAT_EQ(Deg2Rad(-80), controller.BestGammaSail(Deg2Rad(90), 5));
  EXPECT_FLOAT_EQ(Deg2Rad(-90 + optimal_deg), controller.BestGammaSail(Deg2Rad(90), 5));
  EXPECT_FLOAT_EQ(Deg2Rad(-90 + optimal_deg), controller.BestGammaSail(Deg2Rad(90), kAngleReductionLimit));
  EXPECT_FLOAT_EQ(Deg2Rad(-90 + optimal_deg / 4), controller.BestGammaSail(Deg2Rad(90), 2 * kAngleReductionLimit));
}

int main(int argc, char* argv[]) {
  SailController_Basic();
  SailController_Reverse();
  SailController_Storm();

  return 0;
}

