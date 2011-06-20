// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_CONTROLLER_IO_H
#define HELMSMAN_CONTROLLER_IO_H

#include "helmsman/imu.h"
#include "helmsman/wind.h"
#include "helmsman/drive_data.h"
//#include "skipper/skipper_input.h"  

// Input definition
struct ControllerInput {
  void Reset () {
    imu.Reset();
    wind.Reset();
    drives.Reset();
    alpha_star = 0;  // natural 
  }
  
  Imu imu;
  Wind wind;
  DriveActualValuesRad drives;  // Actual positions and Ready flags
  double alpha_star;            // from Skipper
};

// Output definition
struct ControllerOutput {
  void Reset() {
    drives_reference.Reset();
    //skipper_input.Reset();
  }
  //SkipperInput skipper_input;
  DriveReferenceValuesRad drives_reference;
};

struct FilteredMeasurements {
  void Reset() {
    alpha_boat = 0;
    mag_boat = 0;
    omega_boat = 0;
    alpha_true = 0;
    mag_true = 0;
    alpha_app = 0;
    mag_app = 0;
    longitude_deg = 0;
    latitude_deg = 0;
    phi_x_rad = 0;
    phi_y_rad = 0;
    temperature_c = 0;
  } 
  // Apply the same filters for all 3 vectors!
  // true speed, global frame
  double alpha_boat;
  double mag_boat;
  double omega_boat;
  // true wind, global frame
  double alpha_true;
  double mag_true;
  // apparent wind direction, boat frame
  double alpha_app;
  double mag_app;
  // GPS data
  double longitude_deg;
  double latitude_deg;

  double phi_x_rad;  // roll or heel
  double phi_y_rad;  // pitch
  double temperature_c;       // in deg C        
};


/*  double alpha_app;
  double mag_app;
  Apparent(alpha_true, mag_true,
           alpha_boat, mag_boat,
           &alpha_app, &mag_app);
*/
#endif  // HELMSMAN_CONTROLLER_IO_H
