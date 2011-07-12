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
      homing_counter_left_(50),
      homing_counter_right_(20),
      north_deg_(0),
      east_deg_(0),
      apparent_(0, 0) {
  SetStartPoint(start_location);
}

// The x-component of the sail force, very roughly.
double BoatModel::ForceSail(Polar apparent, double gamma_sail) {

  double angle_attack = SymmetricRad(gamma_sail - apparent.AngleRad() - M_PI);
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
double BoatModel::RudderAcc(double gamma_rudder, double water_speed) {
  // This can lead to oscillations in the model due to the bad Euler integration
  // model. The magic damping factor 0.7 dampens these oscillation artefacts.
  double alpha_water = 0;
  if (water_speed > 0)
    alpha_water = 0.7 * -atan2(omega_ * 1.43, water_speed);  // 1.43 m distance COG to rudder, was 1.9m
  gamma_rudder -= alpha_water;
    
  // stall above 25 degrees
  double lift = (fabs(gamma_rudder) < Deg2Rad(25)) * -gamma_rudder / Deg2Rad(10);

  return (lift * water_speed * water_speed * Sign(water_speed) * 
          (0.085 * 2 * 1030 / 2)        // 2 rudders * area * rho_water / 2; 
          - 20 * omega_) /              // viscose damping
              120 ;                     // assumed inertia of 120 kg m^2
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
  const double kOmegaMaxRudder = Deg2Rad(30);

  FollowRateLimitedRadWrap(drives_reference.gamma_sail_star_rad,
                    kOmegaMaxSail, &gamma_sail_); 
  drives->gamma_sail_rad = gamma_sail_;
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
  in->imu.Reset();
  in->wind.Reset();
  // alpha_star remains
 
  apparent_ = Polar (0, 0);  // apparent wind in the boat frame
  ApparentPolar(true_wind, Polar(phi_z_, v_x_), phi_z_, &apparent_);

  // Euler integration, acc turns clockwise
  // Homing is symmetric and does not produce much rotation.
  if (in->drives.homed_rudder_right && in->drives.homed_rudder_left)
    omega_ += RudderAcc(gamma_rudder_right_, v_x_) * period_;
  phi_z_ += omega_ * period_;
  phi_z_ = NormalizeRad(phi_z_);


  v_x_ += period_ / 535.0 * (ForceSail(apparent_, gamma_sail_) +
                             0.2 * v_x_ * v_x_ * -Sign(v_x_) * 1030/2);  // eqiv. area 0.2m^2

  // Produce GPS info.
  // Convert a meter of way into the change of latitude, roughly.
  const double MeterToDegree = Rad2Deg(1/6378100.0); 
  north_deg_ += v_x_ * cos(phi_z_) * period_ * MeterToDegree;
  east_deg_  += v_x_ * sin(phi_z_) * period_ * MeterToDegree;
  
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

  in->wind.alpha_deg = NormalizeDeg(Rad2Deg(angle_sensor));
  in->wind.mag_kn = MeterPerSecondToKnots(apparent_.Mag());
  SimDrives(drives_reference, &in->drives);
  in->imu.speed_m_s = v_x_;
  in->imu.position.longitude_deg = east_deg_;
  in->imu.position.latitude_deg = north_deg_;
  in->imu.position.altitude_m = 0;
  in->imu.attitude.phi_x_rad = 0;
  in->imu.attitude.phi_y_rad = 0;
  in->imu.attitude.phi_z_rad = phi_z_;
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
 printf("%6.3f %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f (%8.3f)\n",
        t, north_deg_, east_deg_, Rad2Deg(SymmetricRad(phi_z_)), v_x_,
        Rad2Deg(gamma_sail_), Rad2Deg(gamma_rudder_right_), Rad2Deg(apparent_.AngleRad()));
}

void BoatModel::PrintHeader() {
 printf("\n%-7s %-8s %-8s %-8s %-8s %-8s %-8s\n",
        "time/s", "North/m", "East/m",
        "phi/deg", "v/m/s", "sail/deg", "rudder/deg");
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
      break;
    case kSukku:
      north_deg_ = 47.2962-0.008;
      east_deg_ = 8.5812-0.008;
      break;
    default:
      assert(0);  
  }
}


