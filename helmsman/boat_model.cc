// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// A rough physical boat model.

#include <string>
#include "common/normalize.h"
#include "common/sign.h"
#include "helmsman/boat_model.h"
#include "helmsman/compass.h"
#include "helmsman/naca0010.h"

extern int debug;
BoatModel::BoatModel(double sampling_period,
                     double omega,
                     double phi_z,
                     double v_x,
                     double gamma_sail,
                     double gamma_rudder_left,
                     double gamma_rudder_right,
                     Location start_location)
    : period_(sampling_period),
      omega_(omega),
      phi_z_(phi_z),
      v_x_(v_x),
      gamma_sail_(gamma_sail),
      gamma_rudder_left_(gamma_rudder_left),
      gamma_rudder_right_(gamma_rudder_right),
      // The following are not settable for lazyness.
      homed_left_(false),
      homed_right_(false),
      north_deg_(0),
      east_deg_(0),
      x_(0),
      y_(0),
      apparent_(0, 0) {
  SetStartPoint(start_location);
}

// The x-component of the sail force, very roughly.
double BoatModel::ForceSail(Polar apparent, double gamma_sail) {

  double angle_attack = SymmetricRad(gamma_sail - apparent.AngleRad() - M_PI);
  double lift;
  // Improvements see:
  // https://docs.google.com/a/google.com/viewer?a=v&pid=explorer&srcid=0B9PVZMr3Jl1ZZGRlMWRlOTEtOTNhNC00NGY0LWJmNzAtOTU3YzY0NWZhY2U5&hl=en
  if (fabs(angle_attack) < Deg2Rad(25)) {
    lift = angle_attack / Deg2Rad(25);
  } else {
    lift = 0.5 * Sign(angle_attack);
  }
  return cos(gamma_sail + M_PI/2) *
      lift * apparent.Mag() * apparent.Mag() * 8 * 1.184 / 2;  // 8m2*rho_air/2
}

double BoatModel::Saturate(double x, double limit) {
  CHECK_GT(limit, 0);
  if (x >  limit)
    return  limit;
  if (x < -limit)
    return -limit;
  return x;
}

void BoatModel::FollowRateLimited(double in, double max_rate, double* follows) {
  double delta = in - *follows;
  *follows += Saturate(delta, max_rate * period_);
  *follows = SymmetricRad(*follows);
}

void BoatModel::FollowRateLimitedRadWrap(double in, double max_rate, double* follows) {
  double delta = in - *follows;
  // underflow
  if (delta > M_PI) {
    delta -= 2 * M_PI;
  }
  // overflow
  if (delta < -M_PI) {
    delta += 2 * M_PI;
  }

  *follows += Saturate(delta, max_rate * period_);
  *follows = SymmetricRad(*follows);
}

void BoatModel::SimDrives(const DriveReferenceValuesRad& drives_reference,
                          DriveActualValuesRad* drives) {
  if (isnan(drives_reference.gamma_sail_star_rad) ||
      isnan(drives_reference.gamma_rudder_star_left_rad) ||
      isnan(drives_reference.gamma_rudder_star_right_rad)) {
    fprintf(stderr, "Rudder References NaN!\n");
  }

  if (isnan(drives_reference.gamma_sail_star_rad))
    FollowRateLimitedRadWrap(0,
                             kOmegaMaxSail, &gamma_sail_);
  else
    FollowRateLimitedRadWrap(drives_reference.gamma_sail_star_rad,
                             kOmegaMaxSail, &gamma_sail_);
  drives->gamma_sail_rad = gamma_sail_;
  drives->homed_sail = true;

  if (!homed_left_) {
    // fprintf(stderr, "HomingLeft\n");
    gamma_rudder_left_ += Deg2Rad(5 * period_);  // homing speed 5 deg/s
    if (gamma_rudder_left_ > Deg2Rad(90))
      homed_left_ = true;
  } else {
    if (isnan(drives_reference.gamma_rudder_star_left_rad)) {
      FollowRateLimited(0, kOmegaMaxRudder, &gamma_rudder_left_);
    } else {
      FollowRateLimited(drives_reference.gamma_rudder_star_left_rad,
                        kOmegaMaxRudder, &gamma_rudder_left_);
    }
  }

  if (!homed_right_) {
    // fprintf(stderr, "HomingRight\n");
    gamma_rudder_right_ -= Deg2Rad(5 * period_);  // homing speed
    // lets assume the homing reference points (left and right) are not exactly the same (as in reality).
    if (gamma_rudder_right_ < Deg2Rad(-100))
      homed_right_ = true;
  } else {
    if (isnan(drives_reference.gamma_rudder_star_right_rad)) {
      FollowRateLimited(0, kOmegaMaxRudder, &gamma_rudder_right_);
    } else {
      FollowRateLimited(drives_reference.gamma_rudder_star_right_rad,
                        kOmegaMaxRudder, &gamma_rudder_right_);
    }
  }

  drives->homed_rudder_left = homed_left_;
  drives->gamma_rudder_left_rad = gamma_rudder_left_;
  drives->homed_rudder_right = homed_right_;
  drives->gamma_rudder_right_rad = gamma_rudder_right_;
}

void UpdateMagnetic(double phi_z, ControllerInput* in) {
  in->imu.compass.valid = true;
  in->imu.compass.phi_z_rad = phi_z;
  in->compass_sensor.phi_z_rad = GeographicToMagnetic(phi_z);
}

void BoatModel::Simulate(const DriveReferenceValuesRad& drives_reference,
                         Polar true_wind,
                         ControllerInput* in) {
  debug = 0;

  // std::string deb_string = drives_reference.ToString();
  // fprintf(stderr, "Simulate: %s\n", deb_string.c_str());

  in->imu.Reset();
  in->wind_sensor.Reset();
  in->compass_sensor.Reset();

  // alpha_star remains

  apparent_ = Polar (0, 0);  // apparent wind in the boat frame
  ApparentPolar(true_wind, Polar(phi_z_, v_x_), phi_z_, &apparent_);

  double delta_omega;
  double force_rudder_x;
  IntegrateRudderModel(&delta_omega,
                       &force_rudder_x);
  // fprintf(stderr, "delta_omega %g \n", delta_omega);
  omega_ += delta_omega;
  // Euler integration, acc turns clockwise
  phi_z_ += omega_ * period_;
  phi_z_ = NormalizeRad(phi_z_);

  //fprintf(stderr, "v_x %g %g %g\n", v_x_, kRhoWater, gamma_sail_);

  // effective cross section area (with C_d=1) for forward and backward motion.
  // equiv. area 0.03m^2 for forward, 0.022m^2 according to simulation code.
  double A_eff_hull = v_x_ >= 0 ? 0.03 : 0.12;  // A_eff = A * C_d

  double force_x = ForceSail(apparent_, gamma_sail_) +
                   A_eff_hull * v_x_ * v_x_ * -Sign(v_x_) * kRhoWater/2;
                   // + force_rudder_x;

  // Turbulent drag above 5 knots, not very scientific.
  if (v_x_ > 2.5)
    force_x -= (v_x_ - 2.5) * 300.0;
  //fprintf(stderr, "force_x %g\n", force_x);
  v_x_ += force_x * period_ / kMass;

  // Produce GPS info.
  // Convert a meter of way into the change of latitude, roughly, at equator.
  const double MeterToDegree = Rad2Deg(1/6378100.0);
  north_deg_ += v_x_ * cos(phi_z_) * period_ * MeterToDegree;
  east_deg_  += v_x_ * sin(phi_z_) * period_ * MeterToDegree;
  x_ += v_x_ * cos(phi_z_) * period_;
  y_ += v_x_ * sin(phi_z_) * period_;

  // Sensor signal is:
  // The true wind vector direction - (all the angles our sensor is turned by),
  // where the latter expands to
  // phi_z_ (rotation of the boat) +
  // gamma_s (rotation of the mast) +
  // sensor Offset (rotation of the sensor zero relative to the mast) +
  // pi (sensor direction convention is to indicate where the wind comes from,
  // not where the vector is pointing to as we do with all our stuff)

  // The apparent wind vector in the boat system contains the true wind and the
  // boats rotation already. Thus:
  double angle_sensor = apparent_.AngleRad() - (
      kWindSensorOffsetRad + gamma_sail_ + M_PI);

  // exact wind calculation
  in->wind_sensor.alpha_deg = NormalizeDeg(Rad2Deg(angle_sensor));
  in->wind_sensor.mag_m_s = apparent_.Mag();
  in->wind_sensor.valid = true;
  // fakewind formula
  //  angle_sensor = true_wind.AngleRad() - (
  //    kWindSensorOffsetRad + gamma_sail_ + M_PI);
  //  in->wind_sensor.alpha_deg = NormalizeDeg(Rad2Deg(angle_sensor));
  //  in->wind_sensor.mag_m_s = true_wind.Mag();

  SimDrives(drives_reference, &in->drives);
  in->imu.position.longitude_deg = east_deg_;
  in->imu.position.latitude_deg = north_deg_;
  in->imu.position.altitude_m = 0;
  in->imu.attitude.phi_x_rad = 0;
  in->imu.attitude.phi_y_rad = 0;
  in->imu.attitude.phi_z_rad = phi_z_;
  in->imu.velocity.x_m_s = v_x_;
  // IMU sensor velocity is in NED system, but the IMUcat converts
  // the speed into boat coordinates.
  in->imu.velocity.y_m_s = 0;
  in->imu.velocity.z_m_s = 0;
  in->imu.acceleration.x_m_s2 = 0;
  in->imu.acceleration.y_m_s2 = 0;
  in->imu.acceleration.z_m_s2 = -9.81;  // The IMU compass needs the acceleration vector.
  in->imu.gyro.omega_x_rad_s = 0;
  in->imu.gyro.omega_y_rad_s = 0;
  in->imu.gyro.omega_z_rad_s = omega_;
  in->imu.temperature_c = 28;

  UpdateMagnetic(phi_z_, in);

  // fprintf(stderr, "model: latlon:%g/%g phi_z:%g vx: %g om: %g\n", north_deg_, east_deg_, phi_z_, v_x_, omega_);
  // deb_string = in->imu.ToString();
  // fprintf(stderr, "%s\n", deb_string.c_str());

  CHECK_IN_INTERVAL(-80, in->imu.position.longitude_deg, 20);
  CHECK_IN_INTERVAL(0, in->imu.position.latitude_deg, 60);
  CHECK_IN_INTERVAL(-2*M_PI, in->imu.attitude.phi_z_rad, 2*M_PI);
  CHECK_IN_INTERVAL(-10, in->imu.velocity.x_m_s, 10);
  CHECK_IN_INTERVAL( -1, in->imu.velocity.y_m_s, 1);
  //CHECK_IN_INTERVAL(-3.5, in->imu.gyro.omega_z_rad_s, 3.5);  // This is a simulation problem at high speeds

}

void BoatModel::PrintLatLon(double t) {
 fprintf(stderr, "%6.3lf %10.8lf %10.8lf %8.3lf %8.3lf %8.3lf %8.3lf (%8.3lf)\n",
        t, north_deg_, east_deg_, Rad2Deg(SymmetricRad(phi_z_)), v_x_,
        Rad2Deg(gamma_sail_), Rad2Deg(gamma_rudder_right_), Rad2Deg(apparent_.AngleRad()));
}

void BoatModel::Print(double t) {
 fprintf(stderr, "%6.1lf %8.3lf %8.3lf %8.3lf %8.3lf %8.3lf %8.3lf (%8.3lf)\n",
        t, x_, y_, Rad2Deg(SymmetricRad(phi_z_)), v_x_,
        Rad2Deg(gamma_sail_), Rad2Deg(gamma_rudder_right_), Rad2Deg(apparent_.AngleRad()));
}

void BoatModel::PrintHeader() {
 fprintf(stderr, "\n%-7s %-8s %-8s %-8s %-8s %-8s %-8s\n",
        "time/s", "North/m", "East/m",
        "phi/deg", "v/m/s", "sail/deg", "rudder/deg, apparent/deg");
}

void BoatModel::SetSpeed(double speed) {
  v_x_ = speed;
}

void BoatModel::SetPhiZ(double  phi_z) {
  phi_z_ = phi_z;
}

void BoatModel::SetOmega(double omega) {
  omega_ = omega;
}

void BoatModel::SetLatLon(double lat, double lon) {
  north_deg_ = lat;
  east_deg_ = lon;
}

double BoatModel::GetSpeed() {
  return v_x_;
}

double BoatModel::GetPhiZ() {
  return NormalizeRad(phi_z_);
}

void BoatModel::SetStartPoint(Location start_location) {
  switch(start_location) {
    case kBrest:
      north_deg_ = 48.2390;
      east_deg_ = -4.7698;
      x_ = 0;
      y_ = 0;
      break;
    case kSukku:
      north_deg_ = 47.2962-0.008;
      east_deg_ = 8.5812-0.008;
      x_ = 0;
      y_ = 0;
      break;
    default:
      assert(0);
  }
}


void BoatModel::OneRudder(double gamma_rudder, double v_rudder_alpha,
                          double v_rudder_mag,
                          double* force_x, double* force_y) {
  // Angle of attack for the rudder
  double alpha_aoa = gamma_rudder - v_rudder_alpha;        // (3)

  double force_lift = ForceLift(alpha_aoa, v_rudder_mag);  // (4)
  double force_drag = ForceDrag(alpha_aoa, v_rudder_mag);  // (5)

  *force_x = sin(v_rudder_alpha) * force_lift - cos(v_rudder_alpha) * force_drag;  // (6)
  *force_y = cos(v_rudder_alpha) * force_lift - sin(v_rudder_alpha) * force_drag;  // (7)
}

// More precise and stable rudder force model
// We had problems with the very simple one during simulations
// due to the feedback effect and the too simple Euler intergration model.
// kLeverR = 1.43;  // 1.43 m distance COG to rudder
// boat.h kInertiaZ = 150;      // different sources speak of 100 to 150 kg/m^2

void BoatModel::RudderModel(double omega,
                            double period,
                            double* delta_omega_rudder,
                            double* force_rudder_x) {
  // if (debug) fprintf(stderr, "N: omega: %6.4lf deg/s, v: %6.4lf m/s period: %gs\n", Rad2Deg(omega), v_x_, period);
  // Relative speed vector of the rudder axis through the water.
  double v_y = omega * kLeverR;
  double v_rudder_mag = sqrt(v_x_ * v_x_ + v_y * v_y);  // (1) assuming the boat has no y speed
  double v_rudder_alpha = 0;
  if (omega != 0 || v_x_ != 0)
    v_rudder_alpha = atan2(-v_y, v_x_);                 // (2)
  if (debug) fprintf(stderr, "FLR: alpha water: %6.4lf deg, speed %6.4lf\n", Rad2Deg(v_rudder_alpha), v_rudder_mag);

  double force_x_left;
  double force_y_left;
  double force_x_right;
  double force_y_right;
  double force_y;
  if (fabs(gamma_rudder_left_ - gamma_rudder_right_) < Deg2Rad(1)) {
    // Rudders parallel, forces can be calculated more easily.
    OneRudder(gamma_rudder_left_, v_rudder_alpha, v_rudder_mag,
              &force_x_left, &force_y_left);
    if (debug) fprintf(stderr, "FLR: force_y_L %6.4lf (R==L)  | force_x_left %6.4lf\n", force_y_left, force_x_left);
    *force_rudder_x = 2 * force_x_left;
    force_y = 2 * force_y_left;
  } else {

    OneRudder(gamma_rudder_left_, v_rudder_alpha, v_rudder_mag,
              &force_x_left, &force_y_left);
    OneRudder(gamma_rudder_right_, v_rudder_alpha, v_rudder_mag,
              &force_x_right, &force_y_right);

    if (debug) fprintf(stderr, "FLR: force_y_L %6.4lf force_y_R %6.4lf | force_x_left: %6.4lf\n", force_y_left, force_y_right, force_x_left);
    *force_rudder_x = force_x_left + force_x_right;
    force_y = force_y_left + force_y_right;
  }
  const double hull_rotation_resistance = 400;  // This is the counter-force at 53 degrees per second and at the rudder lever/radius.
  force_y += Sign(omega) * omega * omega * hull_rotation_resistance;
  // Because the rudder is at the rear end of the boat a postive y-force causes
  // a negative angular acceleration.
  *delta_omega_rudder = -force_y * kLeverR / kInertiaZ * period;  // (8)
  // if (debug) fprintf(stderr, "N: force_y %6.4lf \n", force_y_left + force_y_right);
}

void BoatModel::IntegrateRudderModel(double* delta_omega_rudder,
                                     double* force_rudder_x) {
  double r_left = gamma_rudder_left_;
  double r_right = gamma_rudder_right_;
  double delta_max = 0;
  if (debug)
    fprintf(stderr, "rudders L/R: %6.4lf deg, %6.4lf deg\n", Rad2Deg(gamma_rudder_left_), Rad2Deg(gamma_rudder_right_));
  if (fabs(r_left - r_right) > Deg2Rad(10)) {
    // rudders not parallel, stationary rotation not possible
    delta_max = 0.000;  // TODO Make this corner case during rudder drive homing work.
    // fprintf(stderr, "rudders not parallel\n");
  } else  if (fabs(r_left + r_right) / 2 > Deg2Rad(88)) {
    // A Rudder angle of 90 degrees would correspond to an infinite rotational speed.
    delta_max = 0.5;
    // fprintf(stderr, "rudders sideways\n");
  } else {
    // For parallel rudders
    // Use Luuks idea of a stationary rotational speed as an upper bound.
    // Without external torques, omega can never have a bigger magnitude
    // than this omega_stat.
    CHECK(fabs((r_left + r_right) / 2) < Deg2Rad(90));
    // Use average rudders angle
    double omega_stat = -v_x_ * tan((r_left + r_right) / 2) * kLeverR;
    delta_max = 1.5 * (omega_stat - omega_) + 0.1;
    if (debug)
      fprintf(stderr, "omega_stat: %6.4lf deg/s, omega_: %lf delta_max %lf\n", Rad2Deg(omega_stat), Rad2Deg(omega_), Rad2Deg(delta_max));
  }
  if (debug)
    fprintf(stderr, "delta_max: %6.4lf deg/s\n", delta_max);

  // Trapez integration model in respect to omega.
  double delta_omega1_unlimited;
  double force_rudder_x1;
  // First calculate with the old omega
  RudderModel(omega_, period_, &delta_omega1_unlimited, &force_rudder_x1);
  // limit the initially calculated acceleration
  double delta_omega1 = std::min(std::max(delta_omega1_unlimited, -fabs(delta_max)),
                                 fabs(delta_max));
  if (delta_omega1_unlimited != delta_omega1 && debug)
    fprintf(stderr, "O1 Limited: %lf %lf\n", delta_omega1_unlimited, delta_omega1);

  // Then calculate with the new omega.
  double delta_omega2_unlimited;
  double force_rudder_x2;
  RudderModel(omega_ + delta_omega1,
              period_,
              &delta_omega2_unlimited, &force_rudder_x2);
  // limit the initially calculated acceleration
  double delta_omega2 = std::min(std::max(delta_omega2_unlimited, -fabs(delta_max)),
                                 fabs(delta_max));
  if (delta_omega2_unlimited != delta_omega2 && debug)
    fprintf(stderr, "O2 Limited: %lf %lf\n", delta_omega2_unlimited, delta_omega2);

  // The truth lies in the middle (probably) ...
  double delta_omega_m_unlimited;
  double force_rudder_x_m;
  RudderModel(omega_ + (delta_omega1 + delta_omega2) / 2,
              period_,
              &delta_omega_m_unlimited,
              &force_rudder_x_m);
  double delta_omega_m = std::min(std::max(delta_omega1_unlimited, -fabs(delta_max)),
                                  fabs(delta_max));

  // Check
  double range_12 = fabs(delta_omega2 - delta_omega1);
  double range_1m = fabs(delta_omega_m - delta_omega1);

  if ((delta_omega1 <= delta_omega_m && delta_omega_m <= delta_omega2) ||
      (delta_omega2 <= delta_omega_m && delta_omega_m <= delta_omega1) ||
      (range_12 != 0 && range_1m / range_12 < 1.1) ||
      (fabs(delta_omega1) < 0.001 &&
       fabs(delta_omega2) < 0.001 &&
       fabs(delta_omega2) < 0.001)) {
    if (range_12 != 0) {
      if (debug) fprintf(stderr, "mix at %5.4lf %%\n" , 100 * range_1m / range_12);
    } else {
      if (debug) fprintf(stderr, "mix at %5.4lf of 0\n" , 100 * range_1m );
    }

    *delta_omega_rudder = delta_omega_m;
    *force_rudder_x = force_rudder_x_m;
  } else {
    // At angle of attack around +- 175 degrees and for negative speeds it
    // happens that c_lift is at the boundary between linear and turbulent flow
    // and we get jumps in the output. These are rare and have little effect
    // The middle value used is never totally off.
    fprintf(stderr, "Integration instable!\n");
    fprintf(stderr, "Inputs: ");
    fprintf(stderr, "omega: %6.4lf deg/s, v: %6.4lf m/s\n", Rad2Deg(omega_), v_x_);
    fprintf(stderr, "No good mix at %6.4lf %%\n" , 100 * range_1m / range_12);

    *delta_omega_rudder = (delta_omega1 + delta_omega2 + delta_omega_m) / 3;
    *force_rudder_x = (force_rudder_x1 + force_rudder_x2 + force_rudder_x_m) / 3;
    fprintf(stderr, "Using  %6.4lf from 1:%6.4lf 2:%6.4lf m:%6.4lf \n", *delta_omega_rudder, delta_omega1, delta_omega2, delta_omega_m);

    // Redo with debug logging on.
    debug = 1;
    RudderModel(omega_, period_, &delta_omega1, &force_rudder_x1);
    RudderModel(omega_ + delta_omega1, period_, &delta_omega2, &force_rudder_x2);
    RudderModel(omega_ + (delta_omega1 + delta_omega2) / 2, period_, &delta_omega_m, &force_rudder_x_m);
    debug = 0;
  }
}

const static double kLinearRudder = Deg2Rad(9);

double BoatModel::ForceLift(double alpha_aoa_rad, double speed) {
  // if (debug) fprintf(stderr, "N: aoa: %6.4lf deg\n", Rad2Deg(alpha_aoa_rad));
  int sign = 1;
  if (alpha_aoa_rad < 0) {
    sign = -1;
    alpha_aoa_rad = -alpha_aoa_rad;
  }
  double c_lift = 0;
  bool stalled = false;
  if (alpha_aoa_rad < kLinearRudder) { // linear range forward, neglect dependency of the stall angle from the speed
    c_lift = naca0010::kCLiftPerRad * alpha_aoa_rad;
  } else if (alpha_aoa_rad < Deg2Rad(175)) { // stall range
    c_lift = 1.1 * sin(2*alpha_aoa_rad);
    stalled = true;
  } else { // linear range backwards
    c_lift = naca0010::kCLiftPerRadReverse * (alpha_aoa_rad - Deg2Rad(180));
  }
  if (debug && stalled)
    fprintf(stderr, "Rudder stall, cL: %g\n", c_lift);
  // if (debug) fprintf(stderr, "N: c_lift: %6.4lf \n", sign * c_lift);
  return sign * c_lift * speed * speed *
          (kAreaR * kRhoWater / 2);      // area * rho_water / 2;
}

double BoatModel::ForceDrag(double alpha_aoa_rad, double speed) {
  if (alpha_aoa_rad < 0) {
    alpha_aoa_rad = -alpha_aoa_rad;
  }
  double c_drag = 0.02; // linear flow ranges forward & backwaards
  if (kLinearRudder < alpha_aoa_rad && alpha_aoa_rad < M_PI - kLinearRudder) // stall range
    c_drag = 1.8 * sin(alpha_aoa_rad) * sin(alpha_aoa_rad);

  // if (debug) fprintf(stderr, "N: c_drag: %6.4lf \n", c_drag);

  return c_drag * speed * speed * (kAreaR * kRhoWater / 2);
}

