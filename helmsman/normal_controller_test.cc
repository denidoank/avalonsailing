// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011


#include "helmsman/normal_controller.h"

#include "common/apparent.h"
#include "common/convert.h"
#include "common/delta_angle.h"
#include "common/polar.h"
#include "common/sign.h"
#include "helmsman/sampling_period.h"
#include "helmsman/controller_io.h"
#include "helmsman/rudder_controller_const.h"
#include "lib/testing/testing.h"


void SetEnv(const Polar& wind_true,
            const Polar& boat,
            FilteredMeasurements* filtered,
            ControllerOutput* out) {
  filtered->Reset();
  out->Reset();
  filtered->phi_z_boat = boat.AngleRad();
  filtered->mag_boat = boat.Mag();
  filtered->omega_boat = 0;
  filtered->alpha_true = wind_true.AngleRad();
  filtered->mag_true = wind_true.Mag();

  Apparent(filtered->alpha_true, filtered->mag_true,
           filtered->phi_z_boat, filtered->mag_boat,
           filtered->phi_z_boat,
           &filtered->angle_app, &filtered->mag_app);
}

void SimDrives(const ControllerOutput& out,
               ControllerInput* in) {
  in->drives.gamma_sail_rad +=
       0.8 * (DeltaOldNewRad(in->drives.gamma_sail_rad,
              out.drives_reference.gamma_sail_star_rad));
}

// Check sail control
TEST(NormalController, AllSail) {
  RudderController rudder_controller;
  // coefficients for omega_z, phi_z, int(phi_z)
  rudder_controller.SetFeedback(452.39, 563.75, 291.71, true);

  SailController sail_controller;
  sail_controller.SetOptimalAngleOfAttack(Deg2Rad(20));

  NormalController c(&rudder_controller, &sail_controller);
  ControllerInput in;
  in.drives.gamma_sail_rad = 0;

  FilteredMeasurements filtered;
  ControllerOutput out;

  Polar wind_true(0, 5);      // Wind vector forward to North, with 5m/s.
  Polar boat(Deg2Rad(0), 1);  // Boat going forward as well, with 1 m/s.
                              // So the apparent wind vector is at 0 degrees to
                              // the boats x-axis, 4m/s magnitude.
  SetEnv(wind_true, boat, &filtered, &out);

  in.alpha_star_rad = Deg2Rad(0);
  c.Entry(in, filtered);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-8, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-8, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-8, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  in.alpha_star_rad = 0.001;

  c.Run(in, filtered, &out);

  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  wind_true = Polar(-M_PI / 2, 2);  // wind vector to West, with 2m/s
  boat = Polar(0, 2);               // boat going North, with 2 m/s
  //!! This is not a realistic res tsetup because the boat speed is as big as the wind speed.
  // so the apparent wind vector is at -135 degree to
  // the boats x-axis, sqrt(2) * 2m/s magnitude.
  SetEnv(wind_true, boat, &filtered, &out);
  c.Entry(in, filtered);

  c.Run(in, filtered, &out);
  // sail opposing the apparent wind.
  //EXPECT_FLOAT_EQ(25, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
}

// Check sail control
TEST(NormalController, AllSailCloseHauled) {
  RudderController rudder_controller;
  // coefficients for omega_z, phi_z, int(phi_z)
  rudder_controller.SetFeedback(452.39, 563.75, 291.71, true);

  SailController sail_controller;
  sail_controller.SetOptimalAngleOfAttack(Deg2Rad(20));

  NormalController c(&rudder_controller, &sail_controller);
  ControllerInput in;
  in.drives.gamma_sail_rad = 0;

  FilteredMeasurements filtered;
  ControllerOutput out;

  Polar wind_true(0, 2);        // Wind vector forward to North, with 2m/s.
  Polar boat(Deg2Rad(130), 1);  // Boat going SouthEast, with 1 m/s.
                                // Close hauled, on backbord bow.
  SetEnv(wind_true, boat, &filtered, &out);
  in.alpha_star_rad = Deg2Rad(130);

  c.Entry(in, filtered);
  c.Run(in, filtered, &out);
  EXPECT_LT(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));  // too close, fall off left

  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail very close to the middle
  EXPECT_FLOAT_EQ(22.9364121029707, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  c.Run(in, filtered, &out);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(22.9364121029707, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  c.Run(in, filtered, &out);
  SimDrives(out, &in);

  EXPECT_FLOAT_EQ(22.9364121029707, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  wind_true = Polar(Deg2Rad(-35), 2);  // wind turns against us
  // so the apparent wind vector is around 180 degrees
  SetEnv(wind_true, boat, &filtered, &out);
  in.alpha_star_rad = Deg2Rad(130);

  c.Run(in, filtered, &out);
  SimDrives(out, &in);
  // sail opposing the apparent wind.
  // No resistance, try to get through the tack by momentum.
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  wind_true = Polar(Deg2Rad(-49.9), 2);  // wind turns completely against us
  // so the apparent wind vector is around 180 degrees
  SetEnv(wind_true, boat, &filtered, &out);
  in.alpha_star_rad = Deg2Rad(130);

  c.Run(in, filtered, &out);
  SimDrives(out, &in);
  // sail opposing the apparent wind.
  // Least resistance.
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
}

// Check sail control negative
TEST(NormalController, AllSailCloseHauledNegative) {
  RudderController rudder_controller;
  // coefficients for omega_z, phi_z, int(phi_z)
  rudder_controller.SetFeedback(452.39, 563.75, 291.71, true);

  SailController sail_controller;
  sail_controller.SetOptimalAngleOfAttack(Deg2Rad(20));

  NormalController c(&rudder_controller, &sail_controller);
  ControllerInput in;
  in.drives.gamma_sail_rad = 0;

  FilteredMeasurements filtered;
  ControllerOutput out;

  Polar wind_true(0, 2);      // Wind vector forward to North, with 2m/s.
  Polar boat(Deg2Rad(-130), 1);  // Boat going SouthWest, with 1 m/s.
                              // Close hauled, on starboard bow.
  SetEnv(wind_true, boat, &filtered, &out);
  in.alpha_star_rad = Deg2Rad(-130);

  c.Entry(in, filtered);
  c.Run(in, filtered, &out);
  // Too close, fall off right.
  EXPECT_FLOAT_EQ(-3.078200908151791, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail very close to the middle
  EXPECT_FLOAT_EQ(-22.9364121029707, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  c.Run(in, filtered, &out);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-22.9364121029707, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  c.Run(in, filtered, &out);
  SimDrives(out, &in);

  EXPECT_FLOAT_EQ(-22.9364121029707, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  wind_true = Polar(Deg2Rad(35), 2);  // wind turns against us
  // so the apparent wind vector is around 180 degrees
  SetEnv(wind_true, boat, &filtered, &out);
  in.alpha_star_rad = Deg2Rad(-130);

  c.Run(in, filtered, &out);
  SimDrives(out, &in);
  // sail opposing the apparent wind.
  // Keep a low profile.
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  wind_true = Polar(Deg2Rad(49.9), 2);  // wind turns completely against us
  // so the apparent wind vector is around 180 degrees
  SetEnv(wind_true, boat, &filtered, &out);
  in.alpha_star_rad = Deg2Rad(130);

  c.Run(in, filtered, &out);
  SimDrives(out, &in);
  // sail opposing the apparent wind.
  // Keep the least resistance.
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
}

TEST(NormalController, AllRudder) {
    // With the magic test speed and the amplification coefficients set right
    // the rudder angle will be equal to the negative control error.
    RudderController rudder_controller;
    // coefficients for        omega_z, phi_z, int(phi_z), no feedforward
    rudder_controller.SetFeedback(0,    1000,     0,       false);

    SailController sail_controller;
    sail_controller.SetOptimalAngleOfAttack(Deg2Rad(20));
    NormalController c(&rudder_controller, &sail_controller);
    double alpha_star_rad = Deg2Rad(90);  // So we steer a sailable course.
    ControllerInput in;
    in.drives.gamma_sail_rad = 0;
    FilteredMeasurements filtered;
    ControllerOutput out;
    Polar wind_true(0, 2);       // Wind vector forward to North, with 2m/s.
    Polar boat(alpha_star_rad, kMagicTestSpeed);  // Boat going East, with about 1.1 m/s.
                            // So the apparent wind vector is at about -120 degrees to
                            // the boats x-axis, 2.3m/s magnitude.
    SetEnv(wind_true, boat, &filtered, &out);
    in.alpha_star_rad = alpha_star_rad;  //  no tack or jibe zone
    c.Entry(in, filtered);

    c.Run(in, filtered, &out);
    c.Run(in, filtered, &out);
    EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
    EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
              out.drives_reference.gamma_rudder_star_right_rad);

    c.Run(in, filtered, &out);
    EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
    EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
              out.drives_reference.gamma_rudder_star_right_rad);

    in.alpha_star_rad = alpha_star_rad + 0.001;
    c.Run(in, filtered, &out);
    EXPECT_FLOAT_EQ(-0.001, out.drives_reference.gamma_rudder_star_left_rad);
    EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
              out.drives_reference.gamma_rudder_star_right_rad);

}

TEST(NormalController, AllEast) {
RudderController rudder_controller;
// coefficients for omega_z, phi_z, int(phi_z)
rudder_controller.SetFeedback(452.39, 563.75, 291.71, true);

SailController sail_controller;
sail_controller.SetOptimalAngleOfAttack(Deg2Rad(20));

NormalController c(&rudder_controller, &sail_controller);
ControllerInput in;
in.drives.gamma_sail_rad = 0;
FilteredMeasurements filtered;
ControllerOutput out;

Polar wind_true(0, 2);       // Wind vector forward to North, with 2m/s.
double alpha_star_rad = Deg2Rad(90);  // So we aren't in the tack zone or jibe zone.
Polar boat(alpha_star_rad, 1);  // Boat going East, with 1 m/s.
                         // So the apparent wind vector is at about -120 degrees to
                         // the boats x-axis, approx. 2.4m/s magnitude.
SetEnv(wind_true, boat, &filtered, &out);

in.alpha_star_rad = alpha_star_rad;
c.Entry(in, filtered);
in.alpha_star_rad = alpha_star_rad;
c.Run(in, filtered, &out);
EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
          out.drives_reference.gamma_rudder_star_right_rad);

in.alpha_star_rad = alpha_star_rad;
c.Run(in, filtered, &out);
EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));

in.alpha_star_rad = alpha_star_rad - Deg2Rad(0.1);  // -0.1 degree control error

// PI controller response
for (int i = 0; i < 10; ++i) {
  printf("%d  \n", i);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0.0739085 + i * (0.0775447 - 0.0739085), Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
}
c.Run(in, filtered, &out);
EXPECT_FLOAT_EQ(0.110271, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
}

TEST(NormalController, Synthetic) {
  // With the magic test speed and the amplification coefficients set right
  // the rudder angle will be equal to the negative control error.
  RudderController rudder_controller;
  // coefficients for        omega_z, phi_z, int(phi_z), no feedforward
  rudder_controller.SetFeedback(0,    1000,     0,       false);

  SailController sail_controller;
  sail_controller.SetOptimalAngleOfAttack(Deg2Rad(20));

  NormalController c(&rudder_controller, &sail_controller);
  ControllerInput in;
  in.drives.gamma_sail_rad = 0;

  FilteredMeasurements filtered;
  ControllerOutput out;

  Polar wind_true(0, 2);       // Wind vector forward to North, with 2m/s.
  Polar boat(Deg2Rad(90), kMagicTestSpeed);  // Boat going East, with about 1.1 m/s.
                          // So the apparent wind vector is at about -20 degrees to
                          // the boats x-axis, 1m/s magnitude.
  SetEnv(wind_true, boat, &filtered, &out);
  in.alpha_star_rad = Deg2Rad(90);  //  no tack or jibe zone
  c.Entry(in, filtered);

  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_right_rad));
  // Sail in wing mode
  EXPECT_FLOAT_EQ(50.2369181956312, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_right_rad));
  EXPECT_FLOAT_EQ(50.2369181956312, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  in.alpha_star_rad = Deg2Rad(90.0) + 0.001;

  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.001, out.drives_reference.gamma_rudder_star_left_rad);
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  EXPECT_FLOAT_EQ(50.2369181956312, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  in.alpha_star_rad = Deg2Rad(90.0);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.0, out.drives_reference.gamma_rudder_star_left_rad);
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  EXPECT_FLOAT_EQ(50.2369181956312, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.0, out.drives_reference.gamma_rudder_star_left_rad);
  in.alpha_star_rad = Deg2Rad(90.0) - 0.001;
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0.001, out.drives_reference.gamma_rudder_star_left_rad);
  in.alpha_star_rad = Deg2Rad(90.0);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0.0, out.drives_reference.gamma_rudder_star_left_rad);

  wind_true = Polar(M_PI / 2, 2);  // wind vector to East, with 2m/s
  boat = Polar(0, 1);              // boat going North, with 2 m/s
  // so the apparent wind vector is at -135 degree to
  // the boats x-axis, sqrt(2) * 2m/s magnitude.
  SetEnv(wind_true, boat, &filtered, &out);
  c.Entry(in, filtered);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-50.23691819563117, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
}

int TicksNeeded(const NormalController& c, double from, double to) {
  return Deg2Rad(fabs(from - to)) / c.RateLimit() / kSamplingPeriod;
}

// phi_z (boat.AngleRad()) is important for the sail controller.
// Assume a perfect rudder controller: phi_z = phi_z* 
#define SHAPE(alpha_star)               \
  {                                     \
  Polar boat2(phi_z_star, boat.Mag());  \
  Polar apparent_wind(SymmetricRad(wind_true.AngleRad() - phi_z_star), filtered.mag_app);  \
  c.ShapeReferenceValue(SymmetricRad(Deg2Rad(alpha_star)),     \
                        wind_true,                             \
                        boat2,                                 \
                        apparent_wind,                         \
                        old_gamma_sail,                        \
                        &phi_z_star,                           \
                        &omega_z_star,                         \
                        &gamma_sail_star,                      \
                        &out);                                 \
  printf("%6.2lf %6.2lf %6.2lf\n", phi_z_star, omega_z_star, gamma_sail_star); \
  old_gamma_sail = gamma_sail_star;  \
  }


// Tests of the reference value rate limiting and maneuver planning.
// Approximate the apparent wind as true wind direction - boat direction.
// Run this test as make normal_controller_test.run > x.csv
// to get graph output.
TEST(NormalController, ReferenceValueShaping) {
  RudderController rudder_controller;
  // coefficients for        omega_z, phi_z, int(phi_z), no feedforward
  rudder_controller.SetFeedback(0,    1000,     0,       false);
  SailController sail_controller;
  sail_controller.SetOptimalAngleOfAttack(Deg2Rad(20));
  NormalController c(&rudder_controller, &sail_controller);
  ControllerInput in;
  in.drives.gamma_sail_rad = 0;

  FilteredMeasurements filtered;
  ControllerOutput out;
  Polar wind_true(0, 25);  // Wind vector blowing to the North, with 25m/s.
  Polar boat(Deg2Rad(90), kMagicTestSpeed);  // Boat going East, with about 1.1 m/s.
                            // So the apparent wind vector is at about +90 degrees to
                            // the boats x-axis, 25m/s magnitude.
  SetEnv(wind_true, boat, &filtered, &out);  // calculates the apparent wind
  // straight ahead, in the forbidden zone around the dead run.
  in.alpha_star_rad = Deg2Rad(90);
  c.Entry(in, filtered);
  double phi_z_star = Deg2Rad(90);
  double omega_z_star = 0;
  double gamma_sail_star = Deg2Rad(90 - 20);
  double old_gamma_sail = gamma_sail_star;
  // The boat turns and the apparent wind direction is going to change.
  // For a slow boat we might approximate the apparent wind as true wind
  // direction - boat direction .

  SHAPE(90);
  EXPECT_FLOAT_EQ(Deg2Rad(90), phi_z_star);
  EXPECT_FLOAT_EQ(0, omega_z_star);
  // small angle of attack due to the high wind speed.
  EXPECT_IN_INTERVAL(Deg2Rad(75), gamma_sail_star, Deg2Rad(85));

  // We sail no tack or jibe on this course. We expect a ramp over 1 second.
  const double length_of_ramp = 1.0;  // seconds
  const double new_alpha_star_deg = 90 + Rad2Deg(c.RateLimit() * length_of_ramp);
  const double change_per_call_rad = c.RateLimit() * kSamplingPeriod;
  for (int i = 0; i < length_of_ramp / kSamplingPeriod - 0.5; ++i) {
    SHAPE(new_alpha_star_deg);
    EXPECT_FLOAT_EQ(Deg2Rad(90) + (i + 1) * change_per_call_rad, phi_z_star);
    EXPECT_FLOAT_EQ(0, omega_z_star);
    EXPECT_IN_INTERVAL(Deg2Rad(70), gamma_sail_star, Deg2Rad(80));
  }
  SHAPE(new_alpha_star_deg);
  EXPECT_FLOAT_EQ(Deg2Rad(new_alpha_star_deg), phi_z_star);
  SHAPE(new_alpha_star_deg);
  EXPECT_FLOAT_EQ(Deg2Rad(new_alpha_star_deg), phi_z_star);
  // and back to 90 degrees ...
  for (int i = 0; i < length_of_ramp / kSamplingPeriod - 0.5; ++i) {
    SHAPE(90);
    EXPECT_FLOAT_EQ(Deg2Rad(90) + (9 - i) * change_per_call_rad, phi_z_star);
    EXPECT_FLOAT_EQ(0, omega_z_star);
    EXPECT_IN_INTERVAL(Deg2Rad(70), gamma_sail_star, Deg2Rad(80));
  }
  SHAPE(90);
  EXPECT_FLOAT_EQ(Deg2Rad(90), phi_z_star);
  SHAPE(91);
  SHAPE(91);
  SHAPE(91);
  EXPECT_FLOAT_EQ(Deg2Rad(91), phi_z_star);
  // A wide tack, turning over 178 degrees through the wind. Run for 30s.
  for (int i = 0; i < TicksNeeded(c, 91, -91) + 50; ++i) {
    SHAPE(-91);
  }
  EXPECT_FLOAT_EQ(Deg2Rad(-91), phi_z_star);
  // And back.
  for (int i = 0; i < TicksNeeded(c, -91, 91) + 50; ++i) {
    SHAPE(91);
  }
  EXPECT_FLOAT_EQ(Deg2Rad(91), phi_z_star);
  // A gentle change of bearing
  for (int i = 0; i < TicksNeeded(c, 91, 89) + 50; ++i) {
    SHAPE(89);
  }
  EXPECT_FLOAT_EQ(Deg2Rad(89), phi_z_star);
  // The NormalController enforces a rate limit on the desired direction.
  // Tacks happen faster than than, jibes take longer because of the
  // 180 degree sail rotation (180 degree / 13 degree/s = 14s).
  // So we estimate each change to take delta_angle / kRateLimit with
  // an extra 10s for jibes.
  // TODO Check where the extra 3 steps come from.
  // A wide jibe, turning over 178 degrees from the wind.
  for (int i = 0; i < TicksNeeded(c, 89, -89) + 100; ++i) {
    SHAPE(-89);
  }
  EXPECT_FLOAT_EQ(Deg2Rad(-89), phi_z_star);

  for (int i = 0; i < TicksNeeded(c, -89, 66) + 100; ++i) {
    SHAPE(66);
  }
  EXPECT_FLOAT_EQ(0, omega_z_star);
  EXPECT_FLOAT_EQ(Deg2Rad(66), phi_z_star);

  for (int i = 0;
       i < TicksNeeded(c, 66, 50) + 30;
       ++i) {
    SHAPE(50);
  }
  EXPECT_FLOAT_EQ(0, omega_z_star);
  EXPECT_FLOAT_EQ(Deg2Rad(50), phi_z_star);

  // Jibes need about 14s extra time for the sail turn of 180 degrees.
  for (int i = 0; i < TicksNeeded(c, 50, -120) + 100; ++i) {
    SHAPE(-120);
  }
  EXPECT_FLOAT_EQ(0, omega_z_star);
  EXPECT_FLOAT_EQ(Deg2Rad(-120), phi_z_star);
  EXPECT_IN_INTERVAL(-60, Rad2Deg(gamma_sail_star), -50);
  // TODO Check where the extra 3 steps come from.
  for (int i = 0; i < TicksNeeded(c, -120, 50) + 100; ++i) {
    SHAPE(50);
  }
  EXPECT_FLOAT_EQ(0, omega_z_star);
  EXPECT_FLOAT_EQ(Deg2Rad(50), phi_z_star);
}

// storm, sail controller always in wing mode.
TEST(NormalController, ReferenceValueShapingStormJibe) {
  RudderController rudder_controller;
  // coefficients for        omega_z, phi_z, int(phi_z), no feedforward
  rudder_controller.SetFeedback(0,    1000,     0,       false);
  SailController sail_controller;
  sail_controller.SetOptimalAngleOfAttack(Deg2Rad(20));
  NormalController c(&rudder_controller, &sail_controller);
  ControllerInput in;
  in.drives.gamma_sail_rad = 0;
  FilteredMeasurements filtered;
  ControllerOutput out;
  Polar wind_true(Deg2Rad(-90), 25);  // Wind vector blowing to the West, with 25m/s. (15 is storm limit)
  Polar boat(Deg2Rad(-45), kMagicTestSpeed);  // Boat going NorthWest, with about 1.1 m/s.
                            // So the apparent wind vector is -45 degrees to
                            // the boats x-axis, 24m/s magnitude.

  SetEnv(wind_true, boat, &filtered, &out);  // calculates the apparent wind
  // NorthEast, downwind.
  in.alpha_star_rad = Deg2Rad(-45);
  c.Entry(in, filtered);
  double phi_z_star = Deg2Rad(-45);
  double omega_z_star = 0;
  double gamma_sail_star = Deg2Rad(120);
  double old_gamma_sail = gamma_sail_star;
  for (int i = 0; i < TicksNeeded(c, -45, -180) + 100; ++i) {
    SHAPE(-180);
  }
  EXPECT_FLOAT_EQ(0, omega_z_star);
  EXPECT_FLOAT_EQ(Deg2Rad(-180), phi_z_star);
  EXPECT_IN_INTERVAL(-85, Rad2Deg(gamma_sail_star), -75);

  // South to North.
  // TODO Check where the extra steps come from.
  for (int i = 0; i < TicksNeeded(c, 0, -180) + 80; ++i) {
    SHAPE(0);
  }
  EXPECT_FLOAT_EQ(0, omega_z_star);
  EXPECT_FLOAT_EQ(Deg2Rad(0), phi_z_star);
  EXPECT_IN_INTERVAL(75, Rad2Deg(gamma_sail_star), 85);  // storm: small angle of attack
}


TEST(NormalController, ReferenceValueShapingWest) {
  RudderController rudder_controller;
  // coefficients for        omega_z, phi_z, int(phi_z), no feedforward
  rudder_controller.SetFeedback(0,    1000,     0,       false);
  SailController sail_controller;
  sail_controller.SetOptimalAngleOfAttack(Deg2Rad(20));
  NormalController c(&rudder_controller, &sail_controller);
  ControllerInput in;
  in.drives.gamma_sail_rad = 0;
  FilteredMeasurements filtered;
  ControllerOutput out;
  Polar wind_true(Deg2Rad(-90), 10);  // Wind vector blowing to the West, with 10m/s (15 is storm limit).
  Polar boat(Deg2Rad(-90), kMagicTestSpeed);  // Boat going East, with about 1.1 m/s.
                            // So the apparent wind vector is 0 degrees to
                            // the boats x-axis, 24m/s magnitude.

  SetEnv(wind_true, boat, &filtered, &out);  // calculates the apparent wind
  // NorthEast, downwind.
  in.alpha_star_rad = Deg2Rad(-45);
  c.Entry(in, filtered);
  double phi_z_star = Deg2Rad(-45);
  double omega_z_star = 0;
  double gamma_sail_star = Deg2Rad(93);
  double old_gamma_sail = gamma_sail_star;
  // Approximate the apparent wind as true wind direction - boat direction .

  for (int i = 0; i < TicksNeeded(c, -180, 45); ++i) {
    SHAPE(-180);
  }
  EXPECT_FLOAT_EQ(0, omega_z_star);
  EXPECT_FLOAT_EQ(Deg2Rad(-180), phi_z_star);
  EXPECT_IN_INTERVAL(-65, Rad2Deg(gamma_sail_star), -55);

  // South to North Jibe.
  // TODO Check where the extra 10 steps come from.
  for (int i = 0;
       i < TicksNeeded(c, 0, 180) + 80 + 15;
       ++i) {
    SHAPE(0);
  }
  EXPECT_FLOAT_EQ(0, omega_z_star);
  EXPECT_FLOAT_EQ(Deg2Rad(0), phi_z_star);
  EXPECT_IN_INTERVAL(55, Rad2Deg(gamma_sail_star), 65);

  for (int i = 0;
       i < TicksNeeded(c, 30, 0) + 20;
       ++i) {
    SHAPE(30);
  }
  EXPECT_FLOAT_EQ(0, omega_z_star);
  EXPECT_FLOAT_EQ(Deg2Rad(30), phi_z_star);
  EXPECT_IN_INTERVAL(25, Rad2Deg(gamma_sail_star), 35);

  // A tack.
  for (int i = 0;
       i < TicksNeeded(c, 150, 30) + 100;
       ++i) {
    SHAPE(150);
  }
  EXPECT_FLOAT_EQ(0, omega_z_star);
  EXPECT_FLOAT_EQ(Deg2Rad(150), phi_z_star);
  EXPECT_IN_INTERVAL(-35, Rad2Deg(gamma_sail_star), -25);

  // A circle.
  printf("Circle\n");
  for (int i = 0; i < Deg2Rad(500) / c.RateLimit() / kSamplingPeriod; ++i) {
    SHAPE(Rad2Deg(i * c.RateLimit() * kSamplingPeriod));
  }
}

#undef SHAPE

int main(int argc, char* argv[]) {
  NormalController_AllSailCloseHauled();
  NormalController_AllSailCloseHauledNegative();
  NormalController_AllSail();
  NormalController_AllRudder();
  NormalController_AllEast();
  NormalController_Synthetic();
  NormalController_ReferenceValueShaping();
  NormalController_ReferenceValueShapingStormJibe();
  NormalController_ReferenceValueShapingWest();
  return 0;
}
