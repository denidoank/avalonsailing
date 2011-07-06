// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/reference_values.h"

#include "common/convert.h"
#include "helmsman/sampling_period.h"
#include "lib/testing/testing.h"

TEST(RefeferenceValues, Tack) {
  ReferenceValues ref;
  ref.SetReferenceValues(Deg2Rad(50), Deg2Rad(-15));
  double phi_z_star;
  double omega_star;
  double gamma_sail_star;

  ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
  EXPECT_FLOAT_EQ(Deg2Rad(50), phi_z_star);
  EXPECT_EQ(0, omega_star);
  EXPECT_FLOAT_EQ(Deg2Rad(-15), gamma_sail_star);
  ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
  EXPECT_FLOAT_EQ(Deg2Rad(50), phi_z_star);
  EXPECT_EQ(0, omega_star);
  EXPECT_FLOAT_EQ(Deg2Rad(-15), gamma_sail_star);

  ref.NewPlan(Deg2Rad(-50), Deg2Rad(15 - -15), 2);
  printf("+50 to -50 degrees plan\n");
  printf("t, phi_z, omega*, gamma_sail\n");
  double t = 0;
  while (ref.RunningPlan()) {
    ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
    printf("%6.4g %6.4g %6.4g %6.4g\n",
           t, Rad2Deg(phi_z_star), Rad2Deg(omega_star),
           Rad2Deg(gamma_sail_star));
    t += kSamplingPeriod;      
    // During transition the reference values are not exact.
    EXPECT_LE(omega_star, 0.001);
    EXPECT_IN_INTERVAL(Deg2Rad(-50.001), phi_z_star, Deg2Rad( 50.001));
    //EXPECT_IN_INTERVAL(Deg2Rad(-15.1), gamma_sail_star, Deg2Rad( 15.1));
  }
  ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
  // After transition the reference values are exact.
  EXPECT_FLOAT_EQ(Deg2Rad(-50), phi_z_star);
  EXPECT_EQ(0, omega_star);
  EXPECT_FLOAT_EQ(Deg2Rad(15), gamma_sail_star);
  ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
  EXPECT_FLOAT_EQ(Deg2Rad(-50), phi_z_star);
  EXPECT_EQ(0, omega_star);
  EXPECT_FLOAT_EQ(Deg2Rad(15), gamma_sail_star);

  ref.NewPlan(Deg2Rad(50), Deg2Rad(-15 - 15), 2);
  printf("-50 to +50 degrees plan\n");
  printf("t, phi_z, omega*, gamma_sail\n");
  int i = 0;
  while (ref.RunningPlan()) {
    ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
    printf("%6.4g %6.4g %6.4g %6.4g\n",
           i * kSamplingPeriod, Rad2Deg(phi_z_star), Rad2Deg(omega_star),
           Rad2Deg(gamma_sail_star));
    ++i;
    EXPECT_GE(omega_star, -0.001);
    EXPECT_IN_INTERVAL(Deg2Rad(-50.001), phi_z_star, Deg2Rad( 50.001));
    EXPECT_IN_INTERVAL(Deg2Rad(-15.001), gamma_sail_star, Deg2Rad(15.001));
  }
  ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
  EXPECT_FLOAT_EQ(Deg2Rad(50), phi_z_star);
  EXPECT_EQ(0, omega_star);
  EXPECT_FLOAT_EQ(Deg2Rad(-15), gamma_sail_star);
  ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
  EXPECT_FLOAT_EQ(Deg2Rad(50), phi_z_star);
  EXPECT_EQ(0, omega_star);
  EXPECT_FLOAT_EQ(Deg2Rad(-15), gamma_sail_star);

  ref.NewPlan(Deg2Rad(-150), Deg2Rad(-150 - 50), 2);
  printf("+50 to -150 degrees plan\n");
  printf("t, phi_z, omega*, gamma_sail\n");
  i = 0;
  while (ref.RunningPlan()) {
    ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
    printf("%6.4g %6.4g %6.4g %6.4g\n",
           i * kSamplingPeriod, Rad2Deg(phi_z_star), Rad2Deg(omega_star),
           Rad2Deg(gamma_sail_star));
    ++i;
    EXPECT_GE(omega_star, -0.001);  // turn clockwise because it is shorter.
  }
  ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
  EXPECT_FLOAT_EQ(Deg2Rad(-150), phi_z_star);
  EXPECT_EQ(0, omega_star);
  EXPECT_FLOAT_EQ(Deg2Rad(145), gamma_sail_star);
  ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
  EXPECT_FLOAT_EQ(Deg2Rad(-150), phi_z_star);
  EXPECT_EQ(0, omega_star);
  EXPECT_FLOAT_EQ(Deg2Rad(145), gamma_sail_star);

  ref.SetReferenceValues(Deg2Rad(180), Deg2Rad(-90));
  ref.NewPlan(Deg2Rad(-180), Deg2Rad(0), 2);
  printf("+180 to -180 degrees plan\n");
  printf("t, phi_z, omega*, gamma_sail\n");
  i = 0;
  while (ref.RunningPlan()) {
    ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
    printf("%6.4g %6.4g %6.4g %6.4g\n",
           i * kSamplingPeriod, Rad2Deg(phi_z_star), Rad2Deg(omega_star),
           Rad2Deg(gamma_sail_star));
    ++i;
    EXPECT_LE(omega_star, 0.001);
  }
  ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
  EXPECT_EQ(Deg2Rad(-180), phi_z_star);
  EXPECT_EQ(0, omega_star);

  ref.SetReferenceValues(Deg2Rad(180), Deg2Rad(-90));
  ref.NewPlan(Deg2Rad(-181), Deg2Rad(0), 2);
  printf("+180 to -181 degrees plan\n");
  printf("t, phi_z, omega*, gamma_sail\n");
  i = 0;
  while (ref.RunningPlan()) {
    ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
    printf("%6.4g %6.4g %6.4g %6.4g\n",
           i * kSamplingPeriod, Rad2Deg(phi_z_star), Rad2Deg(omega_star),
           Rad2Deg(gamma_sail_star));
    ++i;
    EXPECT_LE(omega_star, 0.001);
  }
  ref.GetReferenceValues(&phi_z_star, &omega_star, &gamma_sail_star);
  EXPECT_FLOAT_EQ(Deg2Rad(179), phi_z_star);
  EXPECT_EQ(0, omega_star);
}


int main() {
  RefeferenceValues_Tack();
  //RefeferenceValues_Jibe();
  return 0;
}
