// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/new_gamma_sail.h"

#include "common/convert.h"
#include "lib/testing/testing.h"
#include "helmsman/maneuver_type.h"


TEST(NewGammaSail, Tack) {
  double alpha_true = M_PI;  // North wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(60);
  double mag_boat = 0.5;
  double new_alpha_boat = alpha_boat - Deg2Rad(120);
  SailController sail_controller;
  double new_gamma_sail = -1;
  double delta_gamma_sail = -1;
  ManeuverType type = kChange;

  NewGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail,
               &type);
  EXPECT_IN_INTERVAL(45, Rad2Deg(new_gamma_sail), 46);
  EXPECT_IN_INTERVAL(90, Rad2Deg(delta_gamma_sail), 92);
  EXPECT_EQ(kTack, type);

  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat - Deg2Rad(140);
  NewGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail,
               &type);
  EXPECT_IN_INTERVAL(64, Rad2Deg(new_gamma_sail), 65);
  EXPECT_IN_INTERVAL(109, Rad2Deg(delta_gamma_sail), 110);
  EXPECT_EQ(kTack, type);
}

TEST(NewGammaSail, Jibe) {
  double alpha_true = 0;  // South wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(50);
  double mag_boat = 2;
  double new_alpha_boat = alpha_boat - Deg2Rad(100);
  SailController sail_controller;
  double new_gamma_sail = -1;
  double delta_gamma_sail = -1;
  ManeuverType type = kChange;
  // from 80 to -80, but through 180 !
  NewGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail,
               &type);
  EXPECT_IN_INTERVAL(-57, Rad2Deg(new_gamma_sail), -56);
  EXPECT_IN_INTERVAL(245, Rad2Deg(delta_gamma_sail), 247);
  EXPECT_EQ(kJibe, type);

  /*   50               -90

                        /
         A             /
     ----            <====
       H
      H


      A
      |
      | Wind

  */
  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat - Deg2Rad(140);
  NewGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail,
               &type);
  EXPECT_IN_INTERVAL(-59, Rad2Deg(new_gamma_sail), -58);
  EXPECT_IN_INTERVAL(-115, Rad2Deg(delta_gamma_sail), -114);
  EXPECT_EQ(kTack, type); // Wrong!! TODO(grundmann)

  /* Correct jibe, sail turns over the bow, delta gamma is positive and
     has a magnitude of more than 180 degrees.

                |
    A           |          A
 ----    ==>    |    ==>   ----
    H           H          H
    H           H          H
  */

  alpha_boat = Deg2Rad(1);
  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat - Deg2Rad(2);
  NewGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail,
               &type);
  EXPECT_IN_INTERVAL(-93, Rad2Deg(new_gamma_sail), -92);
  EXPECT_IN_INTERVAL(174, Rad2Deg(delta_gamma_sail), 176);
  EXPECT_EQ(kJibe, type);
  

  alpha_true = M_PI;  // North wind vector
  mag_true = 5;
  alpha_boat = Deg2Rad(50);
  mag_boat = 0;

  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = Deg2Rad(-180);
  NewGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail,
               &type);
  EXPECT_IN_INTERVAL(-94, Rad2Deg(new_gamma_sail), -92);
  EXPECT_IN_INTERVAL(-54, Rad2Deg(delta_gamma_sail), -52);
  EXPECT_EQ(kChange, type);

  alpha_true = M_PI;  // North wind vector
  mag_true = 5;
  alpha_boat = Deg2Rad(50);
  mag_boat = 0;

  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = Deg2Rad(-179);
  NewGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail,
               &type);
  EXPECT_IN_INTERVAL(92, Rad2Deg(new_gamma_sail), 93);
  EXPECT_IN_INTERVAL(-228, Rad2Deg(delta_gamma_sail), -226);
  EXPECT_EQ(kJibe, type);

  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = Deg2Rad(-181);
  NewGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail,
               &type);
  EXPECT_IN_INTERVAL(-93, Rad2Deg(new_gamma_sail), -92);
  EXPECT_IN_INTERVAL(-53, Rad2Deg(delta_gamma_sail), -52);
  EXPECT_EQ(kChange, type);


}

TEST(NewGammaSail, Change) {
  double alpha_true = 0;  // South wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(50);
  double mag_boat = 1;
  double new_alpha_boat = alpha_boat + Deg2Rad(20);
  SailController sail_controller;
  double new_gamma_sail = -1;
  double delta_gamma_sail = -1;
  ManeuverType type = kChange;
  NewGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail,
               &type);
  EXPECT_IN_INTERVAL(87, Rad2Deg(new_gamma_sail), 89);
  EXPECT_IN_INTERVAL(25, Rad2Deg(delta_gamma_sail), 26);
  EXPECT_EQ(kChange, type);

  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat + Deg2Rad(40);
  NewGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail,
               &type);
  EXPECT_IN_INTERVAL(67, Rad2Deg(new_gamma_sail), 69);
  EXPECT_IN_INTERVAL(4, Rad2Deg(delta_gamma_sail), 6);
  EXPECT_EQ(kChange, type);


  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat + Deg2Rad(60);
  NewGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail,
               &type);
  EXPECT_IN_INTERVAL(49, Rad2Deg(new_gamma_sail), 51);
  EXPECT_IN_INTERVAL(-13, Rad2Deg(delta_gamma_sail), -11);
  EXPECT_EQ(kChange, type);
}

void LogNewGammaSail() {
  double alpha_true = M_PI;  // North wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(50);
  double mag_boat = 0;
  printf("true wind: %g at %g degree\n", mag_true, Rad2Deg(alpha_true));
  printf("boat:      %g at %g degree\n", mag_boat, Rad2Deg(alpha_boat));
  printf("new_heading gamma_sail  delta  type\n");
  for (double turn = 100; turn < 260; turn += 5) {
    double new_alpha_boat = alpha_boat - Deg2Rad(turn);
    SailController sail_controller;
    double new_gamma_sail = -1;
    double delta_gamma_sail = -1;
    ManeuverType type = kChange;
    NewGammaSail(alpha_true, mag_true,
                 alpha_boat, mag_boat,
                 new_alpha_boat,
                 &sail_controller,
                 &new_gamma_sail,
                 &delta_gamma_sail,
                 &type);
    printf("%6.2f %6.2f %6.2f %s\n", Rad2Deg(new_alpha_boat), Rad2Deg(new_gamma_sail), Rad2Deg(delta_gamma_sail),
           type == kChange ? "Change" : type == kJibe ? "Jibe" : "Tack");
  }
}


int main() {
  NewGammaSail_Tack();
  NewGammaSail_Jibe();
  NewGammaSail_Change();
  LogNewGammaSail();
  return 0;
}
