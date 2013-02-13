// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/sail_controller.h"

#include <iostream>
#include <math.h>

#include "common/angle.h"
#include "common/apparent.h"
#include "common/convert.h"
#include "common/polar.h"
#include "helmsman/sail_controller_const.h"
#include "helmsman/sampling_period.h"  // kSamplingPeriod
#include "lib/testing/testing.h"

using namespace std;

ATEST(SailController, SetGetOptimalAngleOfAttack) {
  SailController controller;
  EXPECT_LT(Deg2Rad(5), controller.GetOptimalAngleOfAttack());
  double aoa_optimal = Deg2Rad(10);
  controller.SetOptimalAngleOfAttack(aoa_optimal);
  EXPECT_FLOAT_EQ(Deg2Rad(10), controller.GetOptimalAngleOfAttack());
}

Polar app(double alpha_deg, double mag) {
  fprintf(stderr, "G apparent wind angle %lf deg\n", alpha_deg);
  Polar apparent_wind(deg(alpha_deg), mag);
  return apparent_wind;
}

double TestDeg(SailController* controller,
               double alpha_apparent_deg,
               double apparent_mag) {
  Polar apparent_wind(deg(alpha_apparent_deg), apparent_mag);
  Polar boat(deg(90), 1.5);
  Polar true_wind(0, 0);
  TruePolar(apparent_wind,
            boat,
            boat.Arg(),
            &true_wind);
  return controller->StableGammaSail(true_wind,
                                     apparent_wind,
                                     boat.Arg()).deg();
}

ATEST(SailController, StableGammaSailFromApparent) {
  SailController controller;
  double aoa_optimal = Deg2Rad(10); // TODO Angle
  controller.SetOptimalAngleOfAttack(aoa_optimal);
  controller.SetAppSign(1);
  EXPECT_FLOAT_EQ(-93, controller.StableGammaSailFromApparent(app( 0, 5)).deg());
  EXPECT_FLOAT_EQ(-91, controller.StableGammaSailFromApparent(app( 4, 5)).deg());
  controller.SetAppSign(-1);
  EXPECT_FLOAT_EQ( 91, controller.StableGammaSailFromApparent(app(-4, 5)).deg());

  controller.SetAppSign(1);
  EXPECT_FLOAT_EQ(-70, controller.StableGammaSailFromApparent(app(100, 5)).deg());
  EXPECT_FLOAT_EQ(-80, controller.StableGammaSailFromApparent(app( 90, 5)).deg());
  EXPECT_FLOAT_EQ(-84, controller.StableGammaSailFromApparent(app( 86, 5)).deg());
  EXPECT_FLOAT_EQ(-90, controller.StableGammaSailFromApparent(app( 80, 5)).deg());

  // Switchpoint at 72.5 degrees, Hysteresis width: 2*5deg and a delay of 200 calls.
  // But if the difference to the switchpoint is more than 10 degrees then we
  // switch immediately.
  Angle expected = rad(Deg2Rad(5.1) + kSwitchpoint + M_PI + aoa_optimal);
  expected.print("expected");
  EXPECT_ANGLE_EQ(rad(Deg2Rad(5.1) + kSwitchpoint + M_PI + aoa_optimal),
                  controller.StableGammaSailFromApparent(app(5.1 + Rad2Deg(kSwitchpoint), 5)));
  EXPECT_ANGLE_EQ(rad(Deg2Rad(0) + kSwitchpoint + M_PI + aoa_optimal),
                  controller.StableGammaSailFromApparent(app(0 + Rad2Deg(kSwitchpoint), 5)));
  EXPECT_ANGLE_EQ(rad(Deg2Rad(-4.9) + kSwitchpoint + M_PI + aoa_optimal),
                  controller.StableGammaSailFromApparent(app(-4.9 + Rad2Deg(kSwitchpoint), 5)));
  // Now we would switch to spinakker mode but te delay prevents it.
  EXPECT_ANGLE_EQ(rad(Deg2Rad(-5.1) + kSwitchpoint + M_PI + aoa_optimal),
                  controller.StableGammaSailFromApparent(app(-5.1 + Rad2Deg(kSwitchpoint), 5)));
  EXPECT_ANGLE_EQ(rad(Deg2Rad(-5.1) + kSwitchpoint + M_PI + aoa_optimal),
                  controller.StableGammaSailFromApparent(app(-5.1 + Rad2Deg(kSwitchpoint), 5)));
  EXPECT_ANGLE_EQ(rad(Deg2Rad(-5.1) + kSwitchpoint + M_PI + aoa_optimal),
                  controller.StableGammaSailFromApparent(app(-5.1 + Rad2Deg(kSwitchpoint), 5)));
  // Now we switch to spinakker mode.
  EXPECT_ANGLE_EQ(deg(-61.8),
                  controller.StableGammaSailFromApparent(app(-10.1 + Rad2Deg(kSwitchpoint), 5)));
  EXPECT_ANGLE_EQ(deg(0.05 - Rad2Deg(kDragMax)),
                  controller.StableGammaSailFromApparent(app(0.1, 5)));
  EXPECT_ANGLE_EQ(deg(1 - Rad2Deg(kDragMax)),
                  controller.StableGammaSailFromApparent(app(2, 5)));

  // Switch back to wing mode
  controller.StableGammaSailFromApparent(app(150, 5));  // To set the sail mode
  // assume mode_ was WING
  EXPECT_ANGLE_EQ(rad(Deg2Rad(-5.1) + kSwitchpoint + M_PI + aoa_optimal),
                  controller.StableGammaSailFromApparent(app(-5.1 + Rad2Deg(kSwitchpoint), 5)));
  double alpha_wind_rad = kSwitchpoint;

  controller.SetAppSign(1);

  EXPECT_FLOAT_EQ(-50, controller.StableGammaSailFromApparent(app(120, 5)).deg());
  alpha_wind_rad = kSwitchpoint - 1.1 * kHalfHysteresis;  // in SPINAKER region now
  int delay = static_cast<int>(kSpinakkerSwitchDelay / kSamplingPeriod + 0.5);
  for (int i = 0; i < delay - 1; ++i) {
    EXPECT_FLOAT_EQ(alpha_wind_rad - M_PI + aoa_optimal,
                    controller.StableGammaSailFromApparent(app( Rad2Deg(alpha_wind_rad), 5)).rad());
    fprintf(stderr, "cnt %d\n", i);
  }
  EXPECT_FLOAT_EQ(0.5 * alpha_wind_rad - kDragMax,
                  controller.StableGammaSailFromApparent(app( Rad2Deg(alpha_wind_rad), 5)).rad());

  // and switch back.
  alpha_wind_rad = kSwitchpoint + 1.1 * kHalfHysteresis;  // in WING region now
  for (int i = 0; i < delay; ++i)
    EXPECT_FLOAT_EQ(0.5 * alpha_wind_rad - kDragMax,
                    controller.StableGammaSailFromApparent(app( Rad2Deg(alpha_wind_rad), 5)).rad());
  EXPECT_FLOAT_EQ(alpha_wind_rad - M_PI + aoa_optimal,
                  controller.StableGammaSailFromApparent(app( Rad2Deg(alpha_wind_rad), 5)).rad());

  // Now the wind changes drastically and we switch immediately.
  alpha_wind_rad = kSwitchpoint - 2.1 * kHalfHysteresis;  // far in the SPINAKER region now
  EXPECT_FLOAT_EQ(0.5 * alpha_wind_rad - kDragMax,
                  controller.StableGammaSailFromApparent(app( Rad2Deg(alpha_wind_rad), 5)).rad());

  // and switch back.
  alpha_wind_rad = kSwitchpoint + 2.1 * kHalfHysteresis;  // far in the WING region now
  EXPECT_FLOAT_EQ(alpha_wind_rad - M_PI + aoa_optimal,
                  controller.StableGammaSailFromApparent(app( Rad2Deg(alpha_wind_rad), 5)).rad());

  controller.SetAppSign(1);
  EXPECT_FLOAT_EQ(-100, controller.StableGammaSailFromApparent(app( 70, 5)).deg());
  EXPECT_FLOAT_EQ(-90, controller.StableGammaSailFromApparent(app( 80, 5)).deg());
  EXPECT_FLOAT_EQ(-80, controller.StableGammaSailFromApparent(app( 90, 5)).deg());
  EXPECT_FLOAT_EQ(-70, controller.StableGammaSailFromApparent(app(100, 5)).deg());
  EXPECT_FLOAT_EQ(-60, controller.StableGammaSailFromApparent(app(110, 5)).deg());
  EXPECT_FLOAT_EQ(-50, controller.StableGammaSailFromApparent(app(120, 5)).deg());
  EXPECT_FLOAT_EQ(-40, controller.StableGammaSailFromApparent(app(130, 5)).deg());
  EXPECT_FLOAT_EQ(-30, controller.StableGammaSailFromApparent(app(140, 5)).deg());
  EXPECT_FLOAT_EQ(-25, controller.StableGammaSailFromApparent(app(145, 5)).deg());
  EXPECT_FLOAT_EQ(-22, controller.StableGammaSailFromApparent(app(148, 5)).deg());
  EXPECT_FLOAT_EQ(-20, controller.StableGammaSailFromApparent(app(150, 5)).deg());
  EXPECT_FLOAT_EQ(-15, controller.StableGammaSailFromApparent(app(155, 5)).deg());
  EXPECT_FLOAT_EQ(-13, controller.StableGammaSailFromApparent(app(157, 5)).deg());
  EXPECT_FLOAT_EQ(-13, controller.StableGammaSailFromApparent(app(160, 5)).deg());
  EXPECT_FLOAT_EQ(  0, controller.StableGammaSailFromApparent(app(170, 5)).deg());
  EXPECT_FLOAT_EQ(  0, controller.StableGammaSailFromApparent(app(180, 5)).deg());
  controller.SetAppSign(-1);
  EXPECT_FLOAT_EQ(100, controller.StableGammaSailFromApparent(app( -70, 5)).deg());
  EXPECT_FLOAT_EQ( 90, controller.StableGammaSailFromApparent(app( -80, 5)).deg());
  EXPECT_FLOAT_EQ( 80, controller.StableGammaSailFromApparent(app( -90, 5)).deg());
  EXPECT_FLOAT_EQ( 70, controller.StableGammaSailFromApparent(app(-100, 5)).deg());
  EXPECT_FLOAT_EQ( 60, controller.StableGammaSailFromApparent(app(-110, 5)).deg());
  EXPECT_FLOAT_EQ( 50, controller.StableGammaSailFromApparent(app(-120, 5)).deg());
  EXPECT_FLOAT_EQ( 40, controller.StableGammaSailFromApparent(app(-130, 5)).deg());
  EXPECT_FLOAT_EQ( 30, controller.StableGammaSailFromApparent(app(-140, 5)).deg());
  EXPECT_FLOAT_EQ( 25, controller.StableGammaSailFromApparent(app(-145, 5)).deg());
  EXPECT_FLOAT_EQ( 22, controller.StableGammaSailFromApparent(app(-148, 5)).deg());
  EXPECT_FLOAT_EQ( 20, controller.StableGammaSailFromApparent(app(-150, 5)).deg());
  EXPECT_FLOAT_EQ( 15, controller.StableGammaSailFromApparent(app(-155, 5)).deg());
  EXPECT_FLOAT_EQ( 13, controller.StableGammaSailFromApparent(app(-157, 5)).deg());
  EXPECT_FLOAT_EQ( 13, controller.StableGammaSailFromApparent(app(-160, 5)).deg());
  EXPECT_FLOAT_EQ(  0, controller.StableGammaSailFromApparent(app(-170, 5)).deg());
  EXPECT_FLOAT_EQ(  0, controller.StableGammaSailFromApparent(app(-180, 5)).deg());
  for (int n = 0; n < 300; ++n)  // Still below the switch point.
    EXPECT_FLOAT_EQ(100, controller.StableGammaSailFromApparent(app( -70, 5)).deg());
  // switched to spinakker
  EXPECT_FLOAT_EQ( 63, controller.StableGammaSailFromApparent(app( -60, 5)).deg());
  EXPECT_FLOAT_EQ( 68, controller.StableGammaSailFromApparent(app( -50, 5)).deg());
  EXPECT_FLOAT_EQ( 73, controller.StableGammaSailFromApparent(app( -40, 5)).deg());
  EXPECT_FLOAT_EQ( 78, controller.StableGammaSailFromApparent(app( -30, 5)).deg());
  EXPECT_FLOAT_EQ( 83, controller.StableGammaSailFromApparent(app( -20, 5)).deg());
  EXPECT_FLOAT_EQ( 88, controller.StableGammaSailFromApparent(app( -10, 5)).deg());
  EXPECT_FLOAT_EQ( 93, controller.StableGammaSailFromApparent(app(   0, 5)).deg());
  controller.SetAppSign(1);
  EXPECT_FLOAT_EQ(-93, controller.StableGammaSailFromApparent(app(   0, 5)).deg());
  EXPECT_FLOAT_EQ(-88, controller.StableGammaSailFromApparent(app(  10, 5)).deg());
  EXPECT_FLOAT_EQ(-83, controller.StableGammaSailFromApparent(app(  20, 5)).deg());
  EXPECT_FLOAT_EQ(-78, controller.StableGammaSailFromApparent(app(  30, 5)).deg());
  EXPECT_FLOAT_EQ(-73, controller.StableGammaSailFromApparent(app(  40, 5)).deg());
  EXPECT_FLOAT_EQ(-68, controller.StableGammaSailFromApparent(app(  50, 5)).deg());
  EXPECT_FLOAT_EQ(-63, controller.StableGammaSailFromApparent(app(  60, 5)).deg());
  // delayed switch, still spinakker.
  EXPECT_FLOAT_EQ(-58, controller.StableGammaSailFromApparent(app(  70, 5)).deg());
  for (int n = 0; n < 300; ++n)  // Still below the switch point.
    EXPECT_FLOAT_EQ(-58, controller.StableGammaSailFromApparent(app(  70, 5)).deg());
  for (int n = 0; n < 200; ++n) {
    printf("%d\n", n);
    EXPECT_FLOAT_EQ(-53, controller.StableGammaSailFromApparent(app(  80, 5)).deg());
  }
  // Switched to wing mode.
  for (int n = 0; n < 300; ++n) {
    printf("%d\n", n);
    EXPECT_FLOAT_EQ(-90, controller.StableGammaSailFromApparent(app(  80, 5)).deg());
  }
  EXPECT_FLOAT_EQ(-90, controller.StableGammaSailFromApparent(app(  80, 5)).deg());
  EXPECT_FLOAT_EQ(-80, controller.StableGammaSailFromApparent(app(  90, 5)).deg());
  EXPECT_FLOAT_EQ(-70, controller.StableGammaSailFromApparent(app( 100, 5)).deg());
  EXPECT_FLOAT_EQ(-60, controller.StableGammaSailFromApparent(app( 110, 5)).deg());
  EXPECT_FLOAT_EQ(-50, controller.StableGammaSailFromApparent(app( 120, 5)).deg());
  EXPECT_FLOAT_EQ(-40, controller.StableGammaSailFromApparent(app( 130, 5)).deg());
  EXPECT_FLOAT_EQ(-30, controller.StableGammaSailFromApparent(app( 140, 5)).deg());
  EXPECT_FLOAT_EQ(-20, controller.StableGammaSailFromApparent(app( 150, 5)).deg());
  EXPECT_FLOAT_EQ(-13, controller.StableGammaSailFromApparent(app( 160, 5)).deg());
  EXPECT_FLOAT_EQ(  0, controller.StableGammaSailFromApparent(app( 170, 5)).deg());
  EXPECT_FLOAT_EQ(  0, controller.StableGammaSailFromApparent(app( 180, 5)).deg());
  EXPECT_FLOAT_EQ(  0, controller.StableGammaSailFromApparent(app( 190, 5)).deg());
}

// The deviations result from the unknown boat speed used in TestDeg.
// Also expect this test to chenge when the PolarDiagram is updated.
ATEST(SailController, StableGammaSail) {
  SailController controller;
  double aoa_optimal = Deg2Rad(10); // TODO Angle
  controller.SetOptimalAngleOfAttack(aoa_optimal);
  controller.SetAppSign(1);
  EXPECT_FLOAT_EQ(-93, TestDeg(&controller, 0, 5));
  EXPECT_FLOAT_EQ(-90.89534251824536, TestDeg(&controller,  4, 5));
  controller.SetAppSign(-1);
  EXPECT_FLOAT_EQ( 90.89534251824536, TestDeg(&controller, -4, 5));  // theory: 91deg

  controller.SetAppSign(1);
  EXPECT_FLOAT_EQ(-65.97539015580288, TestDeg(&controller, 100, 5));  // -70deg
  EXPECT_FLOAT_EQ(-83.67618027206301, TestDeg(&controller,  80, 5));  // -90deg

  controller.SetAppSign(1);
  EXPECT_FLOAT_EQ(-93.4841174738404,  TestDeg(&controller,  70, 5));  // -100deg
  EXPECT_FLOAT_EQ(-28.29559694243281, TestDeg(&controller, 148, 5));  // -22deg
  EXPECT_FLOAT_EQ(-14.94992290769891, TestDeg(&controller, 160, 5));  // -10deg
  EXPECT_FLOAT_EQ(0 , TestDeg(&controller, 170, 5));  //  0deg
  EXPECT_FLOAT_EQ(0, TestDeg(&controller, 180, 5));  //  0deg
  controller.SetAppSign(-1);
  EXPECT_FLOAT_EQ( 93.4841174738404,  TestDeg(&controller,  -70, 5));  // 100deg
  EXPECT_FLOAT_EQ( 28.29559694243281, TestDeg(&controller, -148, 5));  // 22deg
  EXPECT_FLOAT_EQ( 14.94992290769891, TestDeg(&controller, -160, 5));  // 10deg
  EXPECT_FLOAT_EQ(  0, TestDeg(&controller, -170, 5));  // 0deg
  EXPECT_FLOAT_EQ(  0, TestDeg(&controller, -180, 5));  // 0deg
  EXPECT_FLOAT_EQ(  0, TestDeg(&controller, 170, 5));  // 0deg

  for (int n = 0; n < 300; ++n)  // Still below the switch point.
    EXPECT_FLOAT_EQ(93.4841174738404, TestDeg(&controller,  -70, 5));  // 100deg
  // switched to spinakker
  EXPECT_FLOAT_EQ( 87.72654923534098, TestDeg(&controller,  -10, 5));  // 88deg
  EXPECT_FLOAT_EQ( 93, TestDeg(&controller,    0, 5));  // 93deg
  controller.SetAppSign(1);
  EXPECT_FLOAT_EQ(-93, TestDeg(&controller,    0, 5));  // -93deg
  EXPECT_FLOAT_EQ(-59.95496884210372, TestDeg(&controller,   60, 5));  // -63deg
  // Switched to wing mode.
  for (int n = 0; n < 300; ++n) {
    printf("%d\n", n);
    EXPECT_FLOAT_EQ(-83.67618027206301, TestDeg(&controller,   80, 5));  // -90deg
  }
  EXPECT_FLOAT_EQ(-83.67618027206301,  TestDeg(&controller,   80, 5));  // -90deg
  EXPECT_FLOAT_EQ(-74.49734780804562,  TestDeg(&controller,   90, 5));  // -80deg
  EXPECT_FLOAT_EQ(  0, TestDeg(&controller,  170, 5));  // 0deg
  EXPECT_FLOAT_EQ(  0, TestDeg(&controller,  180, 5));  // 0deg
  EXPECT_FLOAT_EQ(  0, TestDeg(&controller,  190, 5));  // 0deg
}

ATEST(SailController, Reverse) {
  SailController controller;
  controller.SetAppSign(1);
  double optimal_deg = 10;
  controller.SetOptimalAngleOfAttack(Deg2Rad(optimal_deg));
  EXPECT_FLOAT_EQ(-90 - optimal_deg, controller.ReverseGammaSailFromApparent(app( 90, 5)).deg());
  EXPECT_FLOAT_EQ(-80 - optimal_deg, controller.ReverseGammaSailFromApparent(app(100, 5)).deg());
  EXPECT_FLOAT_EQ(-70 - optimal_deg, controller.ReverseGammaSailFromApparent(app(110, 5)).deg());
  EXPECT_FLOAT_EQ(-117, controller.ReverseGammaSailFromApparent(app(120, 5)).deg());
  EXPECT_FLOAT_EQ(-112, controller.ReverseGammaSailFromApparent(app(130, 5)).deg());
  EXPECT_FLOAT_EQ(-107, controller.ReverseGammaSailFromApparent(app(140, 5)).deg());
  EXPECT_FLOAT_EQ( -97, controller.ReverseGammaSailFromApparent(app(160, 5)).deg());
  EXPECT_FLOAT_EQ( -87, controller.ReverseGammaSailFromApparent(app(180, 5)).deg());
  // Reduce AOA and forces for stronger wind. Stay in wing mode.
  EXPECT_FLOAT_EQ( -10, controller.ReverseGammaSailFromApparent(app(180, 7.5)).deg());
  EXPECT_FLOAT_EQ( -10, controller.ReverseGammaSailFromApparent(app(180, 10)).deg());
  EXPECT_FLOAT_EQ( -4.9, controller.ReverseGammaSailFromApparent(app(180, 20)).deg());
  EXPECT_FLOAT_EQ(-90 - 4.9, controller.ReverseGammaSailFromApparent(app( 90, 20)).deg());
  // weak wind cases
  EXPECT_FLOAT_EQ(-120, controller.ReverseGammaSailFromApparent(app(120, 0.4)).deg());
  EXPECT_FLOAT_EQ(-120, controller.ReverseGammaSailFromApparent(app( 90, 0.4)).deg());
  controller.SetAppSign(-1);
  EXPECT_FLOAT_EQ(120, controller.ReverseGammaSailFromApparent(app(120, 0.4)).deg());
  EXPECT_FLOAT_EQ(120, controller.ReverseGammaSailFromApparent(app(180, 0.4)).deg());
  EXPECT_FLOAT_EQ(120, controller.ReverseGammaSailFromApparent(app(170, 0.4)).deg());
  EXPECT_FLOAT_EQ(120, controller.ReverseGammaSailFromApparent(app(-170, 0.4)).deg());
}

ATEST(SailController, AngleStorm) {
  SailController controller;
  controller.SetAppSign(1);
  double optimal_deg = 10;
  controller.SetOptimalAngleOfAttack(Deg2Rad(optimal_deg));

  EXPECT_FLOAT_EQ(-80, controller.StableGammaSailFromApparent(app(  90, 5)).deg());
  EXPECT_FLOAT_EQ(-90 + optimal_deg, controller.StableGammaSailFromApparent(app(  90, 5)).deg());
  EXPECT_FLOAT_EQ(-90 + optimal_deg, controller.StableGammaSailFromApparent(app(  90, kAngleReductionLimit)).deg());
  EXPECT_FLOAT_EQ(-90 + optimal_deg / 4, controller.StableGammaSailFromApparent(app(  90, 2 * kAngleReductionLimit)).deg());
}


int main(int argc, char* argv[]) {
  return testing::RunAllTests();
}
