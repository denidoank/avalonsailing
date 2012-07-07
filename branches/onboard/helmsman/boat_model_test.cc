// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011


#include "boat_model.h"

#include <math.h>
#include <stdio.h>
#include "lib/unknown.h"
#include "lib/convert.h"
#include "lib/polar.h"
#include "apparent.h"
#include "controller_io.h"
#include "lib/testing/testing.h"


namespace {
const double kSamplingPeriod = 0.1;
const double kHomingTime = 25;
}


TEST(BoatModel, Zeros) {
  // Set initial boat state.
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  0);               // gamma_sail_ / rad, e.g. -90 degrees here: -M_PI / 2 
  
  DriveReferenceValuesRad ref;
  Polar true_wind(0, 0);           // forward wind blows from South to North, 10m/s
  ControllerInput in;
  
  model.PrintHeader();
  double t;
  for (t = 0; t < 20; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
    // artificial speed override (as an example of forcing a state to be constant)
    // in.imu.speed_m_s = 2;
    model.Print(t); 
  }
  // expect all zeros
  printf("\nExpect all zeros:");
  model.PrintHeader(); 
  model.Print(t); 
  printf("\n\n");
}


// The boat shall slow down if initially pushed.
TEST(BoatModel, Pushed) {
  // Set initial boat state.
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  0);               // gamma_sail_ / rad

  DriveReferenceValuesRad ref;
  Polar true_wind(0, 0);           // forward wind blows from South to North, 10m/s
  ControllerInput in;
  
  model.PrintHeader();
  double t;
  // Wait for homing to be finished, otherwise the rudders turn and cause
  // a direction change.
  for (t = 0; t < kHomingTime; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
  }
  // Override the physical model
  model.SetSpeed(1);
  for (t = 0; t < 20; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
    model.Print(t); 
  }
  printf("\nExpect all zeros, and positive position change north-bound \n"
         "and a speed of less than 1 m/s.");
  model.PrintHeader(); 
  model.Print(t); 
  printf("\n\n");
}


// The boat sails straight at 1m/s initial speed all the time.
// Sail east.
TEST(BoatModel, SailEast) {
  // Set initial boat state.
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  0);               // gamma_sail_ / rad 

  DriveReferenceValuesRad ref;
  Polar true_wind(0, 10);           // forward wind blows from South to North, 10m/s
  ControllerInput in;
  
  model.PrintHeader();
  double t;
  // Wait for homing to be finished, otherwise the rudders turn and cause
  // a direction change.
  for (t = 0; t < kHomingTime; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
  }
  // gamma rudder +
  ref.gamma_rudder_star_left_rad = Deg2Rad(0);
  ref.gamma_rudder_star_right_rad = Deg2Rad(0);
  ref.gamma_sail_star_rad = Deg2Rad(45);
  // Override the physical model
  model.SetSpeed(1);
  model.SetPhiZ(M_PI / 2);
  
  for (t = 0; t < 50; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
    model.Print(t); 
  }
  printf("\nExpect to sail straight east,"
         "and a speed of roughly 3 m/s.");
  model.PrintHeader(); 
  model.Print(t); 
  printf("\n\n");
}

// The boat pulled at 1m/s all the time and turn right with a positive rudder angle.
// Must go in a circle of a certain radius.
TEST(BoatModel, PulledRudderRight) {
  // Set initial boat state.
  int oversampling = 1; //10;
  BoatModel model(kSamplingPeriod / oversampling,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  0);               // gamma_sail_ / rad

  DriveReferenceValuesRad ref;
  Polar true_wind(0, 0);           // forward wind blows from South to North, 10m/s
  ControllerInput in;
  
  model.PrintHeader();
  double t;
  // Wait for homing to be finished, otherwise the rudders turn and cause
  // a direction change.
  for (t = 0; t < kHomingTime; t += kSamplingPeriod) {
    for (int i = 0; i < oversampling; ++i)
      model.Simulate(ref, 
                     true_wind,
                     &in);
  }
  // gamma rudder +
  ref.gamma_rudder_star_left_rad = Deg2Rad(5);
  ref.gamma_rudder_star_right_rad = Deg2Rad(5);
  for (t = 0; t < 200; t += kSamplingPeriod) {
    // Override the physical model
    model.SetSpeed(1);
    for (int i = 0; i < oversampling; ++i)
      model.Simulate(ref, 
                     true_wind,
                     &in);
    model.Print(t); 
  }
  printf("\nExpect a right turn circle of 30m diameter,"
         "and a speed of 1 m/s. (theoretical diameter 43.43m, get 32.16m) ");
  model.PrintHeader(); 
  model.Print(t); 
  printf("\n\n");
}


// The boat sails straight at 1m/s initial speed all the time.
// Sail east.
TEST(BoatModel, SailWest) {
  // Set initial boat state.
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  0);               // gamma_sail_ / rad

  DriveReferenceValuesRad ref;
  Polar true_wind(0, 10);           // forward wind blows from South to North, 10m/s
  ControllerInput in;
  
  model.PrintHeader();
  double t;
  // Wait for homing to be finished, otherwise the rudders turn and cause
  // a direction change.
  for (t = 0; t < kHomingTime; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
  }
  // gamma rudder +
  ref.gamma_rudder_star_left_rad = Deg2Rad(0);
  ref.gamma_rudder_star_right_rad = Deg2Rad(0);
  ref.gamma_sail_star_rad = Deg2Rad(-45);
  // Override the physical model once.
  model.SetSpeed(1);
  model.SetPhiZ(-M_PI / 2);
  
  for (t = 0; t < 50; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
    model.Print(t); 
  }
  printf("\nExpect to sail straight west"
         "at a speed of roughly 3 m/s.");
  model.PrintHeader(); 
  model.Print(t); 
  printf("\n\n");
}

// The boat sails straight at 1m/s initial speed all the time.
// Sail east.
TEST(BoatModel, SailNorth) {
  // Set initial boat state.
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  0);               // gamma_sail_ / rad

  DriveReferenceValuesRad ref;
  Polar true_wind(0, 10);           // forward wind blows from South to North, 10m/s
  ControllerInput in;
  
  model.PrintHeader();
  double t;
  // Wait for homing to be finished, otherwise the rudders turn and cause
  // a direction change.
  for (t = 0; t < kHomingTime; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
  }
  // gamma rudder +
  ref.gamma_rudder_star_left_rad = Deg2Rad(0);
  ref.gamma_rudder_star_right_rad = Deg2Rad(0);
  ref.gamma_sail_star_rad = Deg2Rad(-90);
  // Override the physical model once.
  model.SetSpeed(1);
  model.SetPhiZ(0);
  
  for (t = 0; t < 50; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
    model.Print(t); 
  }
  printf("\nExpect to sail straight North"
         "at a speed of roughly 2.7 m/s.");
  model.PrintHeader(); 
  model.Print(t); 
  printf("\n\n");
}



// The boat shall slow down if initially pushed and turn right with a positive rudder angle.
TEST(BoatModel, PushedRudderRight) {
  // Set initial boat state.
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  0);               // gamma_sail_ / rad

  DriveReferenceValuesRad ref;
  Polar true_wind(0, 0);
  ControllerInput in;
  
  model.PrintHeader();
  double t;
  // Wait for homing to be finished, otherwise the rudders turn and cause
  // a direction change.
  for (t = 0; t < kHomingTime; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
  }
  // Override the physical model
  model.SetSpeed(1);
  // gamma rudder +
  ref.gamma_rudder_star_left_rad = Deg2Rad(5);
  ref.gamma_rudder_star_right_rad = Deg2Rad(5);
  for (t = 0; t < 20; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
    model.Print(t); 
  }
  printf("\nExpect a left turn, and positive position change north-bound \n"
         "a negative change (west) "
         "and a speed of less than 1 m/s.");
  model.PrintHeader(); 
  model.Print(t); 
  EXPECT_LT(Rad2Deg(SymmetricRad(model.GetPhiZ())), -10);
  EXPECT_IN_INTERVAL(0.3, model.GetSpeed(), 1.0);
  printf("\n\n");
}

// The boat pulled at 1m/s all the time and turn right with a negative rudder angle.
// Must go in a circle of a certain radius.
TEST(BoatModel, PulledRudderRightX) {
  // Set initial boat state.
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  0);               // gamma_sail_ / rad

  DriveReferenceValuesRad ref;
  Polar true_wind(0, 0);           // forward wind blows from South to North, 10m/s
  ControllerInput in;
  
  model.PrintHeader();
  double t;
  // Wait for homing to be finished, otherwise the rudders turn and cause
  // a direction change.
  for (t = 0; t < kHomingTime; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
  }
  // gamma rudder +
  ref.gamma_rudder_star_left_rad = Deg2Rad(-5);
  ref.gamma_rudder_star_right_rad = Deg2Rad(-5);
  for (t = 0; t < 200; t += kSamplingPeriod) {
    // Override the physical model
    model.SetSpeed(1);
    model.Simulate(ref, 
                   true_wind,
                   &in);
    model.Print(t); 
  }
  printf("\nExpect a left turn circle of 30m diameter,"
         "and a speed of 1 m/s. (theoretical diameter 43.43m) ");
  model.PrintHeader(); 
  model.Print(t); 
  printf("\n\n");
}

// The boat sails straight at 1m/s initial speed, then returns.
// Sail east, with the sail at the *wrong* angle.
TEST(BoatModel, SailEastReversed) {
  // Set initial boat state.
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  0);               // gamma_sail_ / rad, 

  DriveReferenceValuesRad ref;
  Polar true_wind(0, 10);           // The wind blows from South to North, at 10m/s.
  ControllerInput in;
  
  model.PrintHeader();
  double t;
  // Wait for homing to be finished, otherwise the rudders turn and cause
  // a direction change.
  for (t = 0; t < kHomingTime; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
  }
  ref.gamma_rudder_star_left_rad = Deg2Rad(0);
  ref.gamma_rudder_star_right_rad = Deg2Rad(0);
  ref.gamma_sail_star_rad = Deg2Rad(-45);
  // Override the physical model
  model.SetSpeed(1);
  model.SetPhiZ(M_PI / 2);
  
  for (t = 0; t < 50; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
    model.Print(t); 
  }
  printf("\nExpect to start eastbound, then return "
         "and a speed of roughly -0.7 m/s (slow because of backwards motion).");
  model.PrintHeader(); 
  model.Print(t); 
  EXPECT_IN_INTERVAL(80, Rad2Deg(model.GetPhiZ()), 100);
  EXPECT_IN_INTERVAL(-2, model.GetSpeed(), -0.5);
  printf("\n\n");
}

TEST(BoatModel, SailNorthReversed) {
  // Set initial boat state.
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  0);               // gamma_sail_ / rad 
  
  DriveReferenceValuesRad ref;
  Polar true_wind(-M_PI, 10);       // backwards wind blows from North to South, 10m/s
  ControllerInput in;
  
  model.PrintHeader();
  double t;
  // Wait for homing to be finished, otherwise the rudders turn and cause
  // a direction change.
  for (t = 0; t < kHomingTime; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
  }
  // gamma rudder +
  ref.gamma_rudder_star_left_rad = Deg2Rad(0);
  ref.gamma_rudder_star_right_rad = Deg2Rad(0);
  ref.gamma_sail_star_rad = Deg2Rad(-90);
  // Override the physical model once.
  model.SetSpeed(1);
  model.SetPhiZ(0);
  
  for (t = 0; t < 50; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
    model.Print(t); 
  }
  printf("\nExpect to sail straight North intially and then reverse "
         "at a speed of roughly -1.6 m/s.");
  model.PrintHeader(); 
  model.Print(t); 
  EXPECT_IN_INTERVAL(-10, Rad2Deg(model.GetPhiZ()), 10);
  EXPECT_IN_INTERVAL(-2, model.GetSpeed(), -0.6);
  printf("\n\n");
}

TEST(BoatModel, SailNorthReversedRudderPositive) {
  // Set initial boat state.
  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right 
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed  
                  0);               // gamma_sail_ / rad 
  
  DriveReferenceValuesRad ref;
  Polar true_wind(-M_PI, 10);       // backwards wind blows from North to South, 10m/s
  ControllerInput in;
  
  model.PrintHeader();
  double t;
  // Wait for homing to be finished, otherwise the rudders turn and cause
  // a direction change.
  for (t = 0; t < kHomingTime; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
  }
  // gamma rudder +
  ref.gamma_rudder_star_left_rad = Deg2Rad(5);
  ref.gamma_rudder_star_right_rad = Deg2Rad(5);
  ref.gamma_sail_star_rad = Deg2Rad(-90);
  // Override the physical model once.
  model.SetSpeed(1);
  model.SetPhiZ(0);
  
  for (t = 0; t < 50; t += kSamplingPeriod) {
    model.Simulate(ref, 
                   true_wind,
                   &in);
    model.Print(t); 
  }
  printf("\nExpect to sail straight North intially and then reverse "
         "turning around the portside, finally pointing east.");
  model.PrintHeader(); 
  model.Print(t);
  EXPECT_IN_INTERVAL(80, Rad2Deg(model.GetPhiZ()), 100);
  printf("\n\n");
}

int main(int argc, char* argv[]) {
  BoatModel_PulledRudderRightX();
  BoatModel_Zeros();
  BoatModel_Pushed();
  BoatModel_PushedRudderRight();
  BoatModel_SailEast(); 
  BoatModel_SailWest(); 
  BoatModel_SailNorth(); 
  BoatModel_PulledRudderRight();
  BoatModel_SailEastReversed(); 
  BoatModel_SailNorthReversed();
  BoatModel_SailNorthReversedRudderPositive();
  return 0;
}

