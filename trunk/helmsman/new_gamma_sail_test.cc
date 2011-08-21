// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/new_gamma_sail.h"

#include "common/convert.h"
#include "lib/testing/testing.h"
#include "helmsman/apparent.h"
#include "helmsman/maneuver_type.h"


TEST(NewGammaSail, Tack) {
  double alpha_true = M_PI;  // North wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(60);
  double mag_boat = 0.5;
  double angle_app;
  double mag_app;
  SailController sail_controller;
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  double old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);

  double new_alpha_boat = alpha_boat - Deg2Rad(120);
  double new_gamma_sail = -1;
  double delta_gamma_sail = -1;
  ManeuverType type = kTack;

  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(41, Rad2Deg(new_gamma_sail), 43);
  EXPECT_IN_INTERVAL(82, Rad2Deg(delta_gamma_sail), 86);
  EXPECT_EQ(old_gamma_sail, -new_gamma_sail);


  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat - Deg2Rad(140);
  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(61, Rad2Deg(new_gamma_sail), 62);
  EXPECT_IN_INTERVAL(100, Rad2Deg(delta_gamma_sail), 110);
}

TEST(NewGammaSail, Jibe) {
  double alpha_true = 0;  // South wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(50);
  double mag_boat = 2;
  double angle_app;
  double mag_app;
  SailController sail_controller;
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  double old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);


  double new_alpha_boat = alpha_boat - Deg2Rad(100);
  double new_gamma_sail = -1;
  double delta_gamma_sail = -1;
  ManeuverType type = kJibe;
  // from 80 to -80, but through 180 !
  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(-57, Rad2Deg(new_gamma_sail), -56);
  EXPECT_IN_INTERVAL(245, Rad2Deg(delta_gamma_sail), 247);

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
  type = kJibe;
  new_alpha_boat = alpha_boat - Deg2Rad(140);
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);

  
  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(-56, Rad2Deg(new_gamma_sail), -55);
  EXPECT_IN_INTERVAL(247, Rad2Deg(delta_gamma_sail), 249);

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
  type = kJibe;
  new_alpha_boat = alpha_boat - Deg2Rad(2);
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);
  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(-93, Rad2Deg(new_gamma_sail), -92);
  EXPECT_IN_INTERVAL(174, Rad2Deg(delta_gamma_sail), 176);
  

  alpha_true = M_PI;  // North wind vector
  mag_true = 5;
  alpha_boat = Deg2Rad(50);
  mag_boat = 0;

  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = Deg2Rad(-180);
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);
  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(-94, Rad2Deg(new_gamma_sail), -92);
  EXPECT_IN_INTERVAL(-57, Rad2Deg(delta_gamma_sail), -55);
  EXPECT_EQ(kChange, type);

  alpha_true = M_PI;  // North wind vector
  mag_true = 5;
  alpha_boat = Deg2Rad(50);
  mag_boat = 0;

  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kJibe;
  new_alpha_boat = Deg2Rad(-179);
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);
  
  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(92, Rad2Deg(new_gamma_sail), 93);
  EXPECT_IN_INTERVAL(-231, Rad2Deg(delta_gamma_sail), -229);
  EXPECT_EQ(kJibe, type);

  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = Deg2Rad(-181);
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);
  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(-93, Rad2Deg(new_gamma_sail), -92);
  EXPECT_IN_INTERVAL(-56, Rad2Deg(delta_gamma_sail), -55);
}

TEST(NewGammaSail, Change) {
  double alpha_true = 0;  // South wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(50);
  double mag_boat = 1;
  double angle_app;
  double mag_app;
  SailController sail_controller;
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  double old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);

  double new_alpha_boat = alpha_boat + Deg2Rad(20);
  double new_gamma_sail = -1;
  double delta_gamma_sail = -1;
  ManeuverType type = kChange;
  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(85, Rad2Deg(new_gamma_sail), 86);
  EXPECT_IN_INTERVAL(22, Rad2Deg(delta_gamma_sail), 23);
  EXPECT_EQ(kChange, type);

  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat + Deg2Rad(40);
  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(65, Rad2Deg(new_gamma_sail), 66);
  EXPECT_IN_INTERVAL(2, Rad2Deg(delta_gamma_sail), 3);
  EXPECT_EQ(kChange, type);


  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat + Deg2Rad(60);
  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(47, Rad2Deg(new_gamma_sail), 48);
  EXPECT_IN_INTERVAL(-17, Rad2Deg(delta_gamma_sail), -15);
  EXPECT_EQ(kChange, type);
}

void LogNewGammaSail() {
  double alpha_true = M_PI;  // North wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(50);
  double mag_boat = 0;
  double angle_app;
  double mag_app;

  printf("true wind: %8.2f at %8.2f degree\n", mag_true, Rad2Deg(alpha_true));
  printf("boat:      %8.2f at %8.2f degree\n", mag_boat, Rad2Deg(alpha_boat));
  printf("new_heading gamma_sail  delta  type\n");
  for (double turn = 100; turn < 260; turn += 5) {
    double new_alpha_boat = alpha_boat - Deg2Rad(turn);
    SailController sail_controller;
    double new_gamma_sail = -1;
    double delta_gamma_sail = -1;

    Apparent(alpha_true, mag_true,
             alpha_boat, mag_boat,
             alpha_boat,
             &angle_app, &mag_app);
    double old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);

    ManeuverType type = FindManeuverType(alpha_boat, new_alpha_boat, alpha_true);
    NewGammaSailWithOldGammaSail(alpha_true, mag_true,
                 alpha_boat, mag_boat,
                 new_alpha_boat,
                 old_gamma_sail,
                 type,
                 &sail_controller,
                 &new_gamma_sail,
                 &delta_gamma_sail);
    printf("%9.2f %8.2f %7.2f %s\n", Rad2Deg(new_alpha_boat), Rad2Deg(new_gamma_sail), Rad2Deg(delta_gamma_sail),
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
