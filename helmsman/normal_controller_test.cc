// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011


#include "helmsman/normal_controller.h"

#include "common/convert.h"
#include "common/polar.h"
#include "helmsman/sampling_period.h"
#include "helmsman/apparent.h"
#include "helmsman/controller_io.h"
#include "helmsman/rudder_controller_const.h"

#include "lib/testing/testing.h"


void SetEnv(const Polar& wind_true,
            const Polar& boat,
            ControllerInput* in,
            FilteredMeasurements* filtered,
            ControllerOutput* out) {
  in->Reset();
  in->drives.gamma_sail_rad = 0;
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

// Check sail control
TEST(NormalController, AllSail) {
  RudderController rudder_controller;
  // coefficients for omega_z, phi_z, int(phi_z)
  rudder_controller.SetFeedback(452.39, 563.75, 291.71, true);

  SailController sail_controller;
  NormalController c(&rudder_controller, &sail_controller);
  ControllerInput in;
  FilteredMeasurements filtered;
  ControllerOutput out;

  Polar wind_true(0, 2);      // Wind vector forward to North, with 2m/s.
  Polar boat(Deg2Rad(0), 1);  // Boat going forward as well, with 1 m/s.
                              // So the apparent wind vector is at 0 degrees to
                              // the boats x-axis, 1m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);

  in.alpha_star_rad = Deg2Rad(0);
  c.Entry(in, filtered);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.295634, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.605813, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.930537, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  in.alpha_star_rad = 0.001;

  c.Run(in, filtered, &out);

  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  // Sail in spinakker mode
  EXPECT_FLOAT_EQ(-93, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  wind_true = Polar(-M_PI / 2, 2);  // wind vector to West, with 2m/s
  boat = Polar(0, 2);               // boat going North, with 2 m/s
  // so the apparent wind vector is at -135 degree to
  // the boats x-axis, sqrt(2) * 2m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);

  c.Run(in, filtered, &out);
  // sail opposing the apparent wind.
  EXPECT_FLOAT_EQ(25, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
}

TEST(NormalController, AllRudder) {
    // With the magic test speed and the amplification coefficients set right
    // the rudder angle will be equal to the negative control error.
    RudderController rudder_controller;
    // coefficients for        omega_z, phi_z, int(phi_z), no feedforward
    rudder_controller.SetFeedback(0,    1000,     0,       false);

    SailController sail_controller;
    NormalController c(&rudder_controller, &sail_controller);
    double alpha_star_rad = Deg2Rad(90);  // So we steer a sailable course.
    ControllerInput in;
    FilteredMeasurements filtered;
    ControllerOutput out;
    Polar wind_true(0, 2);       // Wind vector forward to North, with 2m/s.
    Polar boat(alpha_star_rad, kMagicTestSpeed);  // Boat going East, with about 1.1 m/s.
                            // So the apparent wind vector is at about -120 degrees to
                            // the boats x-axis, 2.3m/s magnitude.
    SetEnv(wind_true, boat, &in, &filtered, &out);
    in.alpha_star_rad = alpha_star_rad;  //  no tack or jibe zone
    c.Entry(in, filtered);

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
NormalController c(&rudder_controller, &sail_controller);
ControllerInput in;
FilteredMeasurements filtered;
ControllerOutput out;

Polar wind_true(0, 2);       // Wind vector forward to North, with 2m/s.
double alpha_star_rad = Deg2Rad(90);  // So we aren't in the tack zone or jibe zone.
Polar boat(alpha_star_rad, 1);  // Boat going East, with 1 m/s.
                         // So the apparent wind vector is at about -120 degrees to
                         // the boats x-axis, approx. 2.4m/s magnitude.
SetEnv(wind_true, boat, &in, &filtered, &out);

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

// PI vcontroller response
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
  NormalController c(&rudder_controller, &sail_controller);
  ControllerInput in;
  FilteredMeasurements filtered;
  ControllerOutput out;

  Polar wind_true(0, 2);       // Wind vector forward to North, with 2m/s.
  Polar boat(Deg2Rad(90), kMagicTestSpeed);  // Boat going East, with about 1.1 m/s.
                          // So the apparent wind vector is at about -20 degrees to
                          // the boats x-axis, 1m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);
  in.alpha_star_rad = Deg2Rad(90);  //  no tack or jibe zone
  c.Entry(in, filtered);

  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_right_rad));
  // Sail in wing mode
  EXPECT_FLOAT_EQ(40.8281, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_left_rad));
  EXPECT_FLOAT_EQ(0, Rad2Deg(out.drives_reference.gamma_rudder_star_right_rad));
  EXPECT_FLOAT_EQ(40.8281, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  in.alpha_star_rad = Deg2Rad(90.0) + 0.001;

  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.001, out.drives_reference.gamma_rudder_star_left_rad);
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  EXPECT_FLOAT_EQ(40.8281, Rad2Deg(out.drives_reference.gamma_sail_star_rad));

  in.alpha_star_rad = Deg2Rad(90.0);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.0, out.drives_reference.gamma_rudder_star_left_rad);
  EXPECT_EQ(out.drives_reference.gamma_rudder_star_left_rad,
            out.drives_reference.gamma_rudder_star_right_rad);
  EXPECT_FLOAT_EQ(40.8281, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-0.0, out.drives_reference.gamma_rudder_star_left_rad);
  in.alpha_star_rad = Deg2Rad(90.0) - 0.001;
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0.001, out.drives_reference.gamma_rudder_star_left_rad);
  in.alpha_star_rad = Deg2Rad(90.0);
  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(0.0, out.drives_reference.gamma_rudder_star_left_rad);

  wind_true = Polar(M_PI / 2, 2);  // wind vector to East, with 2m/s
  boat = Polar(0, 2);              // boat going North, with 2 m/s
  // so the apparent wind vector is at -135 degree to
  // the boats x-axis, sqrt(2) * 2m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);

  c.Run(in, filtered, &out);
  EXPECT_FLOAT_EQ(-25, Rad2Deg(out.drives_reference.gamma_sail_star_rad));
}

// Tests of the reference value rate limiting and maneuver planning.
TEST(NormalController, ReferenceValueShaping) {
  RudderController rudder_controller;
  // coefficients for        omega_z, phi_z, int(phi_z), no feedforward
  rudder_controller.SetFeedback(0,    1000,     0,       false);

  SailController sail_controller;
  sail_controller.SetOptimalAngleOfAttack(Deg2Rad(20));
  NormalController c(&rudder_controller, &sail_controller);
  ControllerInput in;
  FilteredMeasurements filtered;
  ControllerOutput out;

  Polar wind_true(0, 25);  // Wind vector blowing to the North, with 25m/s.
  Polar boat(Deg2Rad(90), kMagicTestSpeed);  // Boat going East, with about 1.1 m/s.
                            // So the apparent wind vector is at about +90 degrees to
                            // the boats x-axis, 25m/s magnitude.
  SetEnv(wind_true, boat, &in, &filtered, &out);  // calculates the apparent wind
  // straight ahead, in the forbidden zone around the dead run.
  in.alpha_star_rad = Deg2Rad(90);
  c.Entry(in, filtered);
  double phi_z_star = Deg2Rad(90);
  double omega_z_star = 0;
  double gamma_sail_star = Deg2Rad(90 - 20);
  // The boat turns and the apparent wind direction is going to change.
  // For a slow boat we might approximate the apparent wind as true wind
  // direction - boat direction .
#define SHAPE(alpha_star) \
  c.ShapeReferenceValue(Deg2Rad(alpha_star),                   \
                        wind_true.AngleRad(), wind_true.Mag(), \
                        boat.AngleRad(), boat.Mag(),           \
                        wind_true.AngleRad() - phi_z_star, filtered.mag_app,  \
                        gamma_sail_star,                       \
                        &phi_z_star,                           \
                        &omega_z_star,                         \
                        &gamma_sail_star);                     \
    printf("77 %6.2lf %6.2lf %6.2lf\n", phi_z_star, omega_z_star, gamma_sail_star);

  SHAPE(90);
  EXPECT_FLOAT_EQ(Deg2Rad(90), phi_z_star);
  EXPECT_FLOAT_EQ(0, omega_z_star);
  EXPECT_IN_INTERVAL(Deg2Rad(60), gamma_sail_star, Deg2Rad(80));

  // We sail no tack or jibe on this course. We expect a ramp over 1 second.
  const double length_of_ramp = 1.0;  // seconds
  const double new_alpha_star_deg = 90 + Rad2Deg(c.RateLimit() * length_of_ramp);
  const double change_per_call_rad = c.RateLimit() * kSamplingPeriod;
  for (int i = 0; i < length_of_ramp / kSamplingPeriod - 0.5; ++i) {
    SHAPE(new_alpha_star_deg);
    EXPECT_FLOAT_EQ(Deg2Rad(90) + (i + 1) * change_per_call_rad, phi_z_star);
    EXPECT_FLOAT_EQ(0, omega_z_star);
    EXPECT_IN_INTERVAL(Deg2Rad(60), gamma_sail_star, Deg2Rad(80));
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
    EXPECT_IN_INTERVAL(Deg2Rad(60), gamma_sail_star, Deg2Rad(80));
  }
  SHAPE(90);
  EXPECT_FLOAT_EQ(Deg2Rad(90), phi_z_star);
  SHAPE(91);
  SHAPE(91);
  SHAPE(91);
  // A wide tack, turning over 178 degrees through the wind. Run for 30s.
  for (int i = 0; i < 30.0/kSamplingPeriod; ++i) {
    SHAPE(-91);
  }
  // And back.
  for (int i = 0; i < 30.0/kSamplingPeriod; ++i) {
    SHAPE(91);
  }
  // A gentle change of bearing
  for (int i = 0; i < 2.0/kSamplingPeriod; ++i) {
    SHAPE(89);
  }
  // A wide jibe, turning over 178 degrees from the wind.
  for (int i = 0; i < 50.0/kSamplingPeriod; ++i) {
    SHAPE(-89);
  }



#undef SHAPE
}


int main(int argc, char* argv[]) {
  NormalController_AllSail();
  NormalController_AllRudder();
  NormalController_AllEast();
  NormalController_Synthetic();
  NormalController_ReferenceValueShaping();
  return 0;
}
