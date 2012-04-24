// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#include "helmsman/new_gamma_sail.h"

#include "common/convert.h"
#include "common/normalize.h"
#include "common/polar_diagram.h"
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
  double aoa_optimal = Deg2Rad(10);
  sail_controller.SetOptimalAngleOfAttack(aoa_optimal);

  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  double old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);

  double new_alpha_boat = alpha_boat - Deg2Rad(120);  // to -60deg
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
  EXPECT_IN_INTERVAL(41, Rad2Deg(new_gamma_sail), 60 - aoa_optimal);
  EXPECT_IN_INTERVAL(82, Rad2Deg(delta_gamma_sail), 98);
  EXPECT_FLOAT_EQ(old_gamma_sail, -new_gamma_sail);


  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat - Deg2Rad(140);  // to -80deg
  NewGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat, mag_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(61, Rad2Deg(new_gamma_sail), 80 - aoa_optimal);  // expect 20 deg more than before
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
  double aoa_optimal = Deg2Rad(10);
  sail_controller.SetOptimalAngleOfAttack(aoa_optimal);
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
  EXPECT_IN_INTERVAL(-60, Rad2Deg(new_gamma_sail), -50);
  EXPECT_IN_INTERVAL(240, Rad2Deg(delta_gamma_sail), 260);

  /* Correct jibe, sail turns over the bow, delta gamma is positive and
     has a magnitude of about 180 degrees.

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
  EXPECT_IN_INTERVAL(-65, Rad2Deg(delta_gamma_sail), -50);
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
  EXPECT_IN_INTERVAL(-240, Rad2Deg(delta_gamma_sail), -225);
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
  EXPECT_IN_INTERVAL(-65, Rad2Deg(delta_gamma_sail), -51);
}

TEST(NewGammaSail, Change) {
  double alpha_true = 0;  // South wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(50);
  double mag_boat = 1;
  double angle_app;
  double mag_app;
  SailController sail_controller;
  double aoa_optimal = Deg2Rad(10);
  sail_controller.SetOptimalAngleOfAttack(aoa_optimal);
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
  EXPECT_IN_INTERVAL(76, Rad2Deg(new_gamma_sail), 90);
  EXPECT_IN_INTERVAL(12, Rad2Deg(delta_gamma_sail), 27);
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
  EXPECT_IN_INTERVAL(57, Rad2Deg(new_gamma_sail), 70);
  EXPECT_IN_INTERVAL(2, Rad2Deg(delta_gamma_sail), 6);
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
  EXPECT_IN_INTERVAL(39, Rad2Deg(new_gamma_sail), 52);
  EXPECT_IN_INTERVAL(-17, Rad2Deg(delta_gamma_sail), -12);
  EXPECT_EQ(kChange, type);
}

// Tests of NextGammaSailWithOldGammaSail.
// They all pass without modifications of the expectations.
TEST(NewGammaSail, TackNext) {
  double alpha_true = M_PI;  // North wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(60);
  double mag_boat = 0.5;
  double angle_app;
  double mag_app;
  SailController sail_controller;
  double aoa_optimal = Deg2Rad(10);
  sail_controller.SetOptimalAngleOfAttack(aoa_optimal);

  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  double old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);

  double new_alpha_boat = alpha_boat - Deg2Rad(120);  // to -60deg
  double new_gamma_sail = -1;
  double delta_gamma_sail = -1;
  ManeuverType type = kTack;

  NextGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(41, Rad2Deg(new_gamma_sail), 60 - aoa_optimal);
  EXPECT_IN_INTERVAL(82, Rad2Deg(delta_gamma_sail), 98);
  EXPECT_FLOAT_EQ(old_gamma_sail, -new_gamma_sail);


  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat - Deg2Rad(140);  // to -80deg
  NextGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(61, Rad2Deg(new_gamma_sail), 80 - aoa_optimal);  // expect 20 deg more than before
  EXPECT_IN_INTERVAL(100, Rad2Deg(delta_gamma_sail), 110);
}

TEST(NewGammaSail, JibeNext) {
  double alpha_true = 0;  // South wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(50);
  double mag_boat = 2;
  double angle_app;
  double mag_app;
  SailController sail_controller;
  double aoa_optimal = Deg2Rad(10);
  sail_controller.SetOptimalAngleOfAttack(aoa_optimal);
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
  NextGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat,
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


  NextGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(-60, Rad2Deg(new_gamma_sail), -50);
  EXPECT_IN_INTERVAL(240, Rad2Deg(delta_gamma_sail), 260);

  /* Correct jibe, sail turns over the bow, delta gamma is positive and
     has a magnitude of about 180 degrees.

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
  NextGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat,
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
  NextGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(-94, Rad2Deg(new_gamma_sail), -92);
  EXPECT_IN_INTERVAL(-65, Rad2Deg(delta_gamma_sail), -50);
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

  NextGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(92, Rad2Deg(new_gamma_sail), 93);
  EXPECT_IN_INTERVAL(-240, Rad2Deg(delta_gamma_sail), -225);
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
  NextGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(-93, Rad2Deg(new_gamma_sail), -92);
  EXPECT_IN_INTERVAL(-65, Rad2Deg(delta_gamma_sail), -51);
}

TEST(NewGammaSail, ChangeNext) {
  double alpha_true = 0;  // South wind vector
  double mag_true = 5;
  double alpha_boat = Deg2Rad(50);
  double mag_boat = 1;
  double angle_app;
  double mag_app;
  SailController sail_controller;
  double aoa_optimal = Deg2Rad(10);
  sail_controller.SetOptimalAngleOfAttack(aoa_optimal);
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           alpha_boat,
           &angle_app, &mag_app);
  double old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);

  double new_alpha_boat = alpha_boat + Deg2Rad(20);
  double new_gamma_sail = -1;
  double delta_gamma_sail = -1;
  ManeuverType type = kChange;
  NextGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(76, Rad2Deg(new_gamma_sail), 90);
  EXPECT_IN_INTERVAL(12, Rad2Deg(delta_gamma_sail), 27);
  EXPECT_EQ(kChange, type);

  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat + Deg2Rad(40);
  NextGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(57, Rad2Deg(new_gamma_sail), 70);
  EXPECT_IN_INTERVAL(2, Rad2Deg(delta_gamma_sail), 6);
  EXPECT_EQ(kChange, type);


  new_gamma_sail = -1;
  delta_gamma_sail = -1;
  type = kChange;
  new_alpha_boat = alpha_boat + Deg2Rad(60);
  NextGammaSailWithOldGammaSail(alpha_true, mag_true,
               alpha_boat,
               new_alpha_boat,
               old_gamma_sail,
               type,
               &sail_controller,
               &new_gamma_sail,
               &delta_gamma_sail);
  EXPECT_IN_INTERVAL(39, Rad2Deg(new_gamma_sail), 52);
  EXPECT_IN_INTERVAL(-17, Rad2Deg(delta_gamma_sail), -12);
  EXPECT_EQ(kChange, type);
}

static int tolerance_exceeded = 0;

void LogNewGammaSail(double mag_true_m_s,
                     double alpha_boat = M_PI) {
  double alpha_true = M_PI;  // North wind vector
  //double alpha_boat = M_PI;
  double angle_app;
  double mag_app;
  double mag_boat;
  bool dead_zone_tack;
  bool dead_zone_jibe;
  ReadPolarDiagram(Rad2Deg(SymmetricRad(alpha_boat - alpha_true)),
                   mag_true_m_s,
                   &dead_zone_tack,
                   &dead_zone_jibe,
                   &mag_boat);

  printf("\nfrom 180 to +\ntrue wind: %8.2lf at %8.2lf degree\n", mag_true_m_s, Rad2Deg(alpha_true));
  printf("boat:      %8.2lf at %8.2lf degree\n", mag_boat, Rad2Deg(alpha_boat));
  printf("new_heading  gamma_sail   gamma_sail \n");
  printf("             exact        simplified \n");
  bool app_printed = false;
  for (double turn = 0; turn > -180; turn -= 2.5) {
    double new_alpha_boat = SymmetricRad(alpha_boat + Deg2Rad(turn));
    SailController sail_controller;
    double aoa_optimal = Deg2Rad(10);
    sail_controller.SetOptimalAngleOfAttack(aoa_optimal);
    double new_gamma_sail = -1;
    double delta_gamma_sail = -1;

    Apparent(alpha_true, mag_true_m_s,
             alpha_boat, mag_boat,
             alpha_boat,
             &angle_app, &mag_app);

    double old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);
    if (!app_printed) {
      app_printed = true;
      fprintf(stderr, "old apparent: %7.2lf deg old gamma sail %7.2lf\n", Rad2Deg(angle_app), Rad2Deg(old_gamma_sail));
    }
    ManeuverType maneuver_type = FindManeuverType(alpha_boat, new_alpha_boat, alpha_true);
    NewGammaSailWithOldGammaSail(alpha_true, mag_true_m_s,
                 alpha_boat, mag_boat,
                 new_alpha_boat,
                 old_gamma_sail,
                 maneuver_type,
                 &sail_controller,
                 &new_gamma_sail,
                 &delta_gamma_sail);
    double next_gamma_sail;
    NextGammaSailWithOldGammaSail(angle_app, mag_app,
                 alpha_boat,
                 new_alpha_boat,
                 old_gamma_sail,
                 maneuver_type,
                 &sail_controller,
                 &next_gamma_sail,
                 &delta_gamma_sail);
    fprintf(stderr, "\n");
    if (fabs(Rad2Deg(next_gamma_sail - new_gamma_sail) > 10)) {
      printf("For new alpha boat: %9.2lf >10deg error  exact: %9.2lf <> est %9.2lf next-new: %7.2lf  %-10s\n",
             Rad2Deg(new_alpha_boat), Rad2Deg(new_gamma_sail), Rad2Deg(next_gamma_sail),
             Rad2Deg(next_gamma_sail - new_gamma_sail),
             ManeuverToString(maneuver_type));
      ++tolerance_exceeded;
    }
  }
}

void LogNewGammaSailReversed(double mag_true_m_s) {
  double alpha_true = M_PI - 0.02;  // North wind vector
  double alpha_boat = M_PI - 0.02;
  double angle_app;
  double mag_app;
  double mag_boat;
  bool dead_zone_tack;
  bool dead_zone_jibe;
  ReadPolarDiagram(Rad2Deg(SymmetricRad(alpha_boat - alpha_true)),
                   mag_true_m_s,
                   &dead_zone_tack,
                   &dead_zone_jibe,
                   &mag_boat);

  printf("REVERSE\nfrom 180 to +\ntrue wind: %8.2lf at %8.2lf degree\n", mag_true_m_s, Rad2Deg(alpha_true));
  printf("boat:      %8.2lf at %8.2lf degree\n", mag_boat, Rad2Deg(alpha_boat));
  printf("new_heading  gamma_sail   gamma_sail  maneuver_type\n");
  printf("             exact        simplified                           \n");
  bool app_printed = false;
  for (double turn = 0; turn < 180; turn += 2.5) {
    double new_alpha_boat = 0;
    alpha_boat = SymmetricRad(Deg2Rad(turn));

    SailController sail_controller;
    double aoa_optimal = Deg2Rad(10);
    sail_controller.SetOptimalAngleOfAttack(aoa_optimal);
    double new_gamma_sail = -1;
    double delta_gamma_sail = -1;

    Apparent(alpha_true, mag_true_m_s,
             alpha_boat, mag_boat,
             alpha_boat,
             &angle_app, &mag_app);

    double old_gamma_sail = sail_controller.BestGammaSail(angle_app, mag_app);
    if (!app_printed) {
      app_printed = true;
      fprintf(stderr, "old apparent: %7.2lf deg old gamma sail %7.2lf\n", Rad2Deg(angle_app), Rad2Deg(old_gamma_sail));
    }
    ManeuverType maneuver_type = FindManeuverType(alpha_boat, new_alpha_boat, alpha_true);
    NewGammaSailWithOldGammaSail(alpha_true, mag_true_m_s,
                 alpha_boat, mag_boat,
                 new_alpha_boat,
                 old_gamma_sail,
                 maneuver_type,
                 &sail_controller,
                 &new_gamma_sail,
                 &delta_gamma_sail);
    double next_gamma_sail;
    NextGammaSailWithOldGammaSail(angle_app, mag_app,
                 alpha_boat,
                 new_alpha_boat,
                 old_gamma_sail,
                 maneuver_type,
                 &sail_controller,
                 &next_gamma_sail,
                 &delta_gamma_sail);
    fprintf(stderr, "\n");
    if (fabs(Rad2Deg(next_gamma_sail - new_gamma_sail) > 10)) {
      printf("DEVIATION: new boat %9.2lf  sail exact/estimated: %9.2lf / %9.2lf (delta: %7.2lf) est. maneuver %-10s\n",
             Rad2Deg(new_alpha_boat), Rad2Deg(new_gamma_sail), Rad2Deg(next_gamma_sail),
             Rad2Deg(next_gamma_sail - new_gamma_sail),
             ManeuverToString(maneuver_type));
      ++tolerance_exceeded;
    }
  }
}

int main() {
  NewGammaSail_Tack();
  NewGammaSail_Jibe();
  NewGammaSail_Change();
  LogNewGammaSail(5);
  LogNewGammaSail(10);  // give wind speed here in m/s
  LogNewGammaSail(15);
  // wind speed in m/s, initial boat direction
  LogNewGammaSail(5, Deg2Rad(5));
  LogNewGammaSail(5, Deg2Rad(15));
  LogNewGammaSail(5, Deg2Rad(25));
  LogNewGammaSail(5, Deg2Rad(-90));
  LogNewGammaSail(5, Deg2Rad(-150));
  LogNewGammaSailReversed(10);
  printf("tolerance_exceeded %d\n", tolerance_exceeded);
  return 0;
}
