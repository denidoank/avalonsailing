// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, March 2012

#include "helmsman/compass.h"

#include "common/convert.h"
#include "lib/testing/testing.h"


ATEST(CompassTests, All) {
  double pitch_rad = -1E6;
  double roll_rad = -1E6;

  bool valid = GravityVectorToPitchAndRoll(0, 0 , -10,
                                           &pitch_rad, &roll_rad);
  EXPECT_EQ(true, valid);
  EXPECT_FLOAT_EQ(0, Rad2Deg(pitch_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(roll_rad));
  valid = GravityVectorToPitchAndRoll(0, 0, -7,
                                      &pitch_rad, &roll_rad);
  EXPECT_EQ(false, valid);
  valid = GravityVectorToPitchAndRoll(0, 0, 10,
                                      &pitch_rad, &roll_rad);
  EXPECT_EQ(false, valid);

  valid = GravityVectorToPitchAndRoll(-0.01, 0, -10,
                                      &pitch_rad, &roll_rad);
  EXPECT_EQ(true, valid);
  EXPECT_FLOAT_EQ(-0.01/10, pitch_rad);
  EXPECT_FLOAT_EQ(0, Rad2Deg(roll_rad));

  valid = GravityVectorToPitchAndRoll(0, -0.01, -10,
                                      &pitch_rad, &roll_rad);
  EXPECT_EQ(true, valid);
  EXPECT_FLOAT_EQ(0, pitch_rad);
  EXPECT_FLOAT_EQ(0.01/10, roll_rad);

  valid = GravityVectorToPitchAndRoll(-7.07, 0, -7.07,
                                      &pitch_rad, &roll_rad);
  EXPECT_EQ(false, valid);
  EXPECT_FLOAT_EQ(-45, Rad2Deg(pitch_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(roll_rad));

  valid = GravityVectorToPitchAndRoll(0, -7.07, -7.07,
                                      &pitch_rad, &roll_rad);
  EXPECT_EQ(false, valid);
  EXPECT_FLOAT_EQ(0, Rad2Deg(pitch_rad));
  EXPECT_FLOAT_EQ(45, Rad2Deg(roll_rad));

  double bearing = -1;
  valid = VectorsToBearing(0, 0, -9.81,
                           1, 0, 0,
                           &bearing);
  EXPECT_EQ(true, valid);
  EXPECT_FLOAT_EQ(0, Rad2Deg(bearing));      // heading north

  valid = VectorsToBearing(0, 0, -9.81,
                           0, 1, 0,          // Magnet vector points starboard
                           &bearing);
  EXPECT_EQ(true, valid);
  EXPECT_FLOAT_EQ(-90, Rad2Deg(bearing));    // so we are heading west.

  valid = VectorsToBearing(0, 0, -9.81,
                           -1, 0, 0,         // Magnet vector points behind
                           &bearing);
  EXPECT_EQ(true, valid);
  EXPECT_FLOAT_EQ(-180, Rad2Deg(bearing));   // so we are heading south.

  valid = VectorsToBearing(0, 0, -9.81,
                           0, -1, 0,         // Magnet vector points portside
                           &bearing);
  EXPECT_EQ(true, valid);
  EXPECT_FLOAT_EQ(90, Rad2Deg(bearing));     // so we are heading east.

  // a vertical component does not hurt.
  valid = VectorsToBearing(0, 0, -9.81,
                           0, -1, 0.5,       // Magnet vector points portside
                           &bearing);
  EXPECT_EQ(true, valid);
  EXPECT_FLOAT_EQ(90, Rad2Deg(bearing));     // so we are heading east.

  // a vertical magnetic component does not hurt. roll doesn't matter much.
  valid = VectorsToBearing(0, 1, -9.81,
                           0, -1, 0.5,       // Magnet vector points portside
                           &bearing);
  EXPECT_EQ(true, valid);
  EXPECT_FLOAT_EQ(90, Rad2Deg(bearing));     // so we are heading east.

  // A vertical magnetic component does not hurt. roll and pitch don't matter much.
  valid = VectorsToBearing(-2, 1, -9.81,
                           0, -1, 0.5,       // Magnet vector points portside
                           &bearing);
  EXPECT_EQ(true, valid);
  EXPECT_FLOAT_EQ(85.6731, Rad2Deg(bearing)); // so we are still heading east.

  // BearingToMagnetic is used in the model to create magnetic vectors.
  for (double bearing_in = 0.44; bearing_in < M_PI; bearing_in += 0.182194) {
    double mag_x, mag_y, mag_z;
    BearingToMagnetic(bearing_in, &mag_x, &mag_y, &mag_z);
    valid = VectorsToBearing(0, 0, -9.81,
                             mag_x, mag_y, mag_z,       // Magnet vector points portside
                             &bearing);
    EXPECT_EQ(true, valid);
    EXPECT_FLOAT_EQ(Rad2Deg(bearing_in), Rad2Deg(bearing));
  }

}


int main(int argc, char* argv[]) {
  testing::RunAllTests();
  return 0;
}
