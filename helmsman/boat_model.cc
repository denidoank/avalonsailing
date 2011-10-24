// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// A very rough physical boat model.

#include <string>
#include "common/sign.h"
#include "helmsman/boat_model.h"
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
    lift * apparent.Mag() * apparent.Mag() * 8 * 1.184 / 2; // 8m2*rho_air/2
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
      isnan(drives_reference.gamma_rudder_star_right_rad))
    return;

  FollowRateLimitedRadWrap(drives_reference.gamma_sail_star_rad,
                           kOmegaMaxSail, &gamma_sail_); 
  drives->gamma_sail_rad = gamma_sail_;
  drives->homed_sail = true;

  if (homed_left_) {
    FollowRateLimited(drives_reference.gamma_rudder_star_left_rad,
                      kOmegaMaxRudder, &gamma_rudder_left_);
  } else {
    gamma_rudder_left_ += Deg2Rad(5 * period_);  // homing speed
    if (gamma_rudder_left_ > Deg2Rad(90));
      homed_left_ = true;
  }

  if (homed_right_) {
    FollowRateLimited(drives_reference.gamma_rudder_star_right_rad,
                      kOmegaMaxRudder, &gamma_rudder_right_);
  } else {
    gamma_rudder_right_ -= Deg2Rad(5 * period_);  // homing speed
    if (gamma_rudder_right_ < Deg2Rad(-100));
      homed_right_ = true;
  }

  drives->homed_rudder_left = homed_left_;
  drives->gamma_rudder_left_rad = gamma_rudder_left_;
  drives->homed_rudder_right = homed_right_;
  drives->gamma_rudder_right_rad = gamma_rudder_right_;
}

void BoatModel::Simulate(const DriveReferenceValuesRad& drives_reference, 
                         Polar true_wind,
                         ControllerInput* in) {
  // std::string deb_string = drives_reference.ToString();
  // printf("Simulate: %s\n", deb_string.c_str()); 

  in->imu.Reset();
  in->wind_sensor.Reset();
  // alpha_star remains
 
  apparent_ = Polar (0, 0);  // apparent wind in the boat frame
  ApparentPolar(true_wind, Polar(phi_z_, v_x_), phi_z_, &apparent_);

  double delta_omega;
  double force_rudder_x;
  IntegrateRudderModel(&delta_omega,
                       &force_rudder_x);
  printf("delta_omega %g \n", delta_omega);
  omega_ += delta_omega;
  // Euler integration, acc turns clockwise
  phi_z_ += omega_ * period_;
  phi_z_ = NormalizeRad(phi_z_);

  //printf("v_x %g %g %g\n", v_x_, kRhoWater, gamma_sail_);
  double force_x = ForceSail(apparent_, gamma_sail_) +
                   0.3 * v_x_ * v_x_ * -Sign(v_x_) * kRhoWater/2;  // eqiv. area 0.3m^2
                   // + force_rudder_x;
  // Turbulent drag above 6 knots.
  if (v_x_ > 3)
    force_x -= ((v_x_ - 3) * (v_x_ - 3)) * 500.0;
  //printf("force_x %g\n", force_x);
  v_x_ += force_x * period_ / 535.0;

  
  // Produce GPS info.
  // Convert a meter of way into the change of latitude, roughly.
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
  // fakewind formula
  //  angle_sensor = true_wind.AngleRad() - (
  //    kWindSensorOffsetRad + gamma_sail_ + M_PI); 
  //  in->wind_sensor.alpha_deg = NormalizeDeg(Rad2Deg(angle_sensor));
  //  in->wind_sensor.mag_m_s = true_wind.Mag();
    
  SimDrives(drives_reference, &in->drives);
  in->imu.speed_m_s = v_x_;
  in->imu.position.longitude_deg = east_deg_;
  in->imu.position.latitude_deg = north_deg_;
  in->imu.position.altitude_m = 0;
  in->imu.attitude.phi_x_rad = 0;
  in->imu.attitude.phi_y_rad = 0;
  in->imu.attitude.phi_z_rad = phi_z_;
  in->imu.velocity.x_m_s = v_x_;
  in->imu.velocity.y_m_s = 0;
  in->imu.velocity.z_m_s = 0;
  in->imu.acceleration.x_m_s2 = 0;
  in->imu.acceleration.y_m_s2 = 0;
  in->imu.acceleration.z_m_s2 = 0;
  in->imu.gyro.omega_x_rad_s = 0;
  in->imu.gyro.omega_y_rad_s = 0;
  in->imu.gyro.omega_z_rad_s = omega_;
  in->imu.temperature_c = 28;
 
  // printf("model: latlon:%g/%g phi_z:%g vx: %g om: %g\n", north_deg_, east_deg_, phi_z_, v_x_, omega_);
  // deb_string = in->imu.ToString();
  // printf("%s\n", deb_string.c_str()); 

  CHECK_IN_INTERVAL(-80, in->imu.position.longitude_deg, 20);
  CHECK_IN_INTERVAL(0, in->imu.position.latitude_deg, 60);
  CHECK_IN_INTERVAL(-2*M_PI, in->imu.attitude.phi_z_rad, 2*M_PI);
  CHECK_IN_INTERVAL(-10, in->imu.velocity.x_m_s, 10);
  CHECK_IN_INTERVAL(-10, in->imu.speed_m_s, 10);
  //CHECK_IN_INTERVAL(-3.5, in->imu.gyro.omega_z_rad_s, 3.5);  // This is a simulation problem at high speeds
  
}

void BoatModel::PrintLatLon(double t) {
 printf("%6.3f %10.8f %10.8f %8.3f %8.3f %8.3f %8.3f (%8.3f)\n",
        t, north_deg_, east_deg_, Rad2Deg(SymmetricRad(phi_z_)), v_x_,
        Rad2Deg(gamma_sail_), Rad2Deg(gamma_rudder_right_), Rad2Deg(apparent_.AngleRad()));
}

void BoatModel::Print(double t) {
 printf("%6.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f (%8.3f)\n",
        t, x_, y_, Rad2Deg(SymmetricRad(phi_z_)), v_x_,
        Rad2Deg(gamma_sail_), Rad2Deg(gamma_rudder_right_), Rad2Deg(apparent_.AngleRad()));
}

void BoatModel::PrintHeader() {
 printf("\n%-7s %-8s %-8s %-8s %-8s %-8s %-8s\n",
        "time/s", "North/m", "East/m",
        "phi/deg", "v/m/s", "sail/deg", "rudder/deg, apparent/deg");
}

void BoatModel::SetSpeed(double speed){
  v_x_ = speed;
}

void BoatModel::SetPhiZ(double  phi_z){
  phi_z_ = phi_z;
}

void BoatModel::SetOmega(double omega){
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
  return phi_z_;
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
  if (debug) printf("N: omega: %6.4f deg/s, v: %6.4f m/s period: %gs\n", Rad2Deg(omega), v_x_, period);
  // Relative speed vector of the rudder axis through the water.
  double v_y = omega * kLeverR;
  double v_rudder_mag = sqrt(v_x_ * v_x_ + v_y * v_y);  // (1)
  double v_rudder_alpha = 0;
  if (omega != 0 || v_x_ != 0)
    v_rudder_alpha = atan2(-v_y, v_x_);                 // (2)
  if (debug) printf("N: alpha water: %6.4f deg\n", Rad2Deg(v_rudder_alpha));

  double force_x_left;
  double force_y_left;
  double force_x_right;
  double force_y_right;
  OneRudder(gamma_rudder_left_, v_rudder_alpha, v_rudder_mag,
            &force_x_left, &force_y_left);
  OneRudder(gamma_rudder_right_, v_rudder_alpha, v_rudder_mag,
            &force_x_right, &force_y_right);

  *force_rudder_x = force_x_left + force_x_right;
  const double hull_rotation_resistance = 400; // What force for 50 degrees per second ?
  double force_y = force_y_left + force_y_right + Sign(omega) * omega * omega * hull_rotation_resistance;
  // Because the rudder is at the rear end of the boat a postive y-force causes
  // a negative angular acceleration.
  *delta_omega_rudder = -force_y * kLeverR / kInertiaZ * period;  // (8)
  if (debug) printf("N: force_y %6.4f \n", force_y_left + force_y_right);
}

void BoatModel::IntegrateRudderModel(double* delta_omega_rudder,
                                     double* force_rudder_x) {
  // Use Luuks idea of a stationary rotational speed as an upper bound.
  // Without external torques, omega can never have a bigger magnitude
  // than this omega_stat.
  CHECK(fabs(gamma_rudder_right_) < Deg2Rad(90));
  // Use average rudders angle
  double omega_stat = -v_x_ * tan(gamma_rudder_left_ + gamma_rudder_right_) / 2 * kLeverR;
  double delta_max = omega_stat - omega_;
  // printf("omega_stat: %6.4f deg/s, omega_: %f delta_max %f\n", Rad2Deg(omega_stat), Rad2Deg(omega_), Rad2Deg(delta_max));
           
  // Trapez integration model in respect to omega.
  double delta_omega1;
  double force_rudder_x1;
  // First calculate with the old omega
  RudderModel(omega_, period_, &delta_omega1, &force_rudder_x1);
  // Then calculate with the new omega.
  double delta_omega2;
  double force_rudder_x2;
  // limit the initially calculated acceleration
  // printf("QQQ: %f %f\n", delta_omega1, delta_max);
  delta_omega1 = std::min(std::max(delta_omega1, -fabs(delta_max)), fabs(delta_max));
  RudderModel(omega_ + delta_omega1,
              period_,
              &delta_omega2, &force_rudder_x2);
  // The truth lies in the middle (probably) ...
  double delta_omega_m;
  double force_rudder_x_m;
  RudderModel(omega_ + (delta_omega1 + delta_omega2) / 2, period_, &delta_omega_m, &force_rudder_x_m);

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
      if (debug && 0) printf("mix at %5.4f %%\n" , 100 * range_1m / range_12);
    } else {
      if (debug && 0) printf("mix at %5.4f of 0\n" , 100 * range_1m );
    }

    *delta_omega_rudder = delta_omega_m;
    *force_rudder_x = force_rudder_x_m;
  } else {
    // At angle of attack around +- 175 degrees and for negative speeds it
    // happens that c_lift is at the boundary between linear and turbulent flow
    // and we get jumps in the output. These are rare and have little effect
    // The middle value used is never totally off.
    printf("Integration instable!\n");
    printf("Inputs: ");
    printf("omega: %6.4f deg/s, v: %6.4f m/s\n", Rad2Deg(omega_), v_x_);
    printf("No good mix at %6.4f %%\n" , 100 * range_1m / range_12);

    *delta_omega_rudder = (delta_omega1 + delta_omega2 + delta_omega_m) / 3;
    *force_rudder_x = (force_rudder_x1 + force_rudder_x2 + force_rudder_x_m) / 3;
    printf("Using  %6.4f from 1:%6.4f 2:%6.4f m:%6.4f \n", *delta_omega_rudder, delta_omega1, delta_omega2, delta_omega_m);

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
  //if (debug) printf("N: aoa: %6.4f deg\n", Rad2Deg(alpha_aoa_rad));
  int sign = 1;
  if (alpha_aoa_rad < 0) {
    sign = -1;
    alpha_aoa_rad = -alpha_aoa_rad;
  }
  double c_lift = 0;
  if (alpha_aoa_rad < kLinearRudder) { // linear range forward, neglect dependency of the stall angle from the speed
    c_lift = naca0010::kCLiftPerRad * alpha_aoa_rad;
  } else if (alpha_aoa_rad < Deg2Rad(175)) { // stall range
    c_lift = 1.1 * sin(2*alpha_aoa_rad);
  } else { // linear range backwards
    c_lift = naca0010::kCLiftPerRadReverse * (alpha_aoa_rad - Deg2Rad(180));
  }
  if (debug && alpha_aoa_rad > kLinearRudder)
    printf("Rudder stall, cL: %g\n", c_lift);
  //if (debug) printf("N: c_lift: %6.4f \n", sign * c_lift);
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
  //if (debug) printf("N: c_drag: %6.4f \n", c_drag);

  return c_drag * speed * speed * (kAreaR * kRhoWater / 2);
}

