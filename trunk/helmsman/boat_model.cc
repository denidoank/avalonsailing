// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

// A very rough physical boat model.

#include "helmsman/boat_model.h" 

BoatModel::BoatModel(double sampling_period,
                     double omega,
                     double phi_z, 
                     double v_x,
                     double gamma_sail,
                     double gamma_rudder_left,
                     double gamma_rudder_right) 
    : period_(sampling_period),
      omega_(omega),
      phi_z_(phi_z), 
      v_x_(v_x),
      gamma_sail_(gamma_sail),
      gamma_rudder_left_(gamma_rudder_left),
      gamma_rudder_right_(gamma_rudder_right),
      // The following are not settable for lazyness.   
      homing_counter_left_(50),
      homing_counter_right_(20),
      north_(0),
      east_(0) {}

// The x-component of the sail force, very roughly.
double BoatModel::ForceSail(Polar apparent, double gamma_sail) {

  double angle_attack = gamma_sail - apparent.AngleRad() - M_PI;
  double lift;
  // Improvements see:
  // https://docs.google.com/a/google.com/viewer?a=v&pid=explorer&srcid=0B9PVZMr3Jl1ZZGRlMWRlOTEtOTNhNC00NGY0LWJmNzAtOTU3YzY0NWZhY2U5&hl=en
  if (fabs(angle_attack) < Deg2Rad(15)) {
    lift = angle_attack / Deg2Rad(15);
  } else {
    lift = 1.5 * Sign(angle_attack); 
  }
  return cos(gamma_sail + M_PI/2) *
    lift * apparent.Mag() * apparent.Mag() * 8 * 1.184 / 2; // 8m2*rho_air/2
}

// The rotational acceleration effect of the rudder force, very roughly.
double BoatModel::RudderAcc(double water_speed, double gamma_rudder) {
  // staal above 25 degrees
  double lift = (fabs(gamma_rudder) < Deg2Rad(25)) * -gamma_rudder / Deg2Rad(10);
  // 2 rudders * area * rho_water / 2; assumed 120 kg m^2
  return lift * water_speed * water_speed * 0.085 * 2 * 1030 / 2 / 120 * Sign(water_speed);
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

void BoatModel::SimDrives(const DriveReferenceValuesRad& drives_reference,
                          DriveActualValuesRad* drives) {
  const double kOmegaMaxRudder = Deg2Rad(30);

  FollowRateLimited(drives_reference.gamma_sail_star_rad,
                    kOmegaMaxSail, &gamma_sail_); 
     drives->homed_sail = true;

  if (homing_counter_left_ <= 0) {
    FollowRateLimited(drives_reference.gamma_rudder_star_left_rad,
                      kOmegaMaxRudder, &gamma_rudder_left_);
    drives->gamma_rudder_left_rad = gamma_rudder_left_;
    drives->homed_rudder_left = true;
  } else {
    gamma_rudder_left_ += Deg2Rad(50 * period_);  // homing speed
    drives->gamma_rudder_left_rad = gamma_rudder_left_;
    drives->homed_rudder_left = false;
    --homing_counter_left_;
  }

  if (homing_counter_right_ <= 0) {
    FollowRateLimited(drives_reference.gamma_rudder_star_right_rad,
                      kOmegaMaxRudder, &gamma_rudder_right_);
    drives->gamma_rudder_right_rad = gamma_rudder_right_;
    drives->homed_rudder_right = true;
  } else {
    gamma_rudder_right_ -= Deg2Rad(50 * period_);  // homing speed
    drives->homed_rudder_right = false;
    --homing_counter_right_;
  }
}

void BoatModel::Simulate(const DriveReferenceValuesRad& drives_reference, 
                         Polar true_wind,
                         ControllerInput* in) {
  in->Reset();
  Polar apparent(0 ,0);  // apparent wind in the boat frame
  ApparentPolar(true_wind, Polar(phi_z_, v_x_), &apparent);

  // Euler integration, acc turns clockwise
  omega_ += RudderAcc(gamma_rudder_right_, v_x_) * period_  - 0.01 * omega_;
  phi_z_ += omega_ * period_;
  phi_z_ = NormalizeRad(phi_z_);


  v_x_ += period_ / 535.0 * (ForceSail(apparent, gamma_sail_) +
                             v_x_ * v_x_ * -Sign(v_x_) * 1030/2*0.1);  // eqiv. area 0.5m^2
  //printf("%g\n", v_x_ * v_x_ * -Sign(v_x_) * 1030/2*0.5);
  
  north_ += v_x_ * cos(phi_z_) * period_;
  east_  += v_x_ * sin(phi_z_) * period_;
  
  double angle_sensor = apparent.AngleRad() + kWindSensorOffsetRad - gamma_sail_;

  // Add Wind sensor offset to boat.m

  in->wind.alpha_deg = NormalizeDeg(Rad2Deg(angle_sensor));
  in->wind.mag_kn = MeterPerSecondToKnots(apparent.Mag());
  SimDrives(drives_reference, &in->drives);
  in->imu.speed_m_s = v_x_;
  in->imu.position.longitude_deg = east_;
  in->imu.position.latitude_deg = north_;
  in->imu.position.altitude_m = 0;
  in->imu.attitude.phi_x_rad = phi_z_;
  in->imu.attitude.phi_y_rad = 0;
  in->imu.attitude.phi_z_rad = 0;
  in->imu.velocity.x_m_s = 0;
  in->imu.velocity.y_m_s = 0;
  in->imu.velocity.z_m_s = v_x_;
  in->imu.acceleration.x_m_s2 = 0;
  in->imu.acceleration.y_m_s2 = 0;
  in->imu.acceleration.z_m_s2 = 0;
  in->imu.gyro.omega_x_rad_s = 0;
  in->imu.gyro.omega_y_rad_s = 0;
  in->imu.gyro.omega_z_rad_s = omega_;
  in->imu.temperature_c = 28;
}

void BoatModel::Print(double t) {
 printf("%5.3f %6.4f %6.4f %6.4f %6.4f %6.4f %6.4f\n",
        t, north_, east_, Rad2Deg(SymmetricRad(phi_z_)), v_x_,
        Rad2Deg(gamma_sail_), Rad2Deg(gamma_rudder_right_));
}

void BoatModel::PrintHeader() {
 printf("time/s North/m, East/m, phi/degree, v/m/s, sail, rudder");
}

