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
    alpha_apparent = 0;
    mag_apparent = 0;
    alpha_true = 0;
    mag_true = 0;
  } 
  double alpha_apparent;
  double mag_apparent;
  double alpha_true;
  double mag_true;
};



#endif  // HELMSMAN_CONTROLLER_IO_H
