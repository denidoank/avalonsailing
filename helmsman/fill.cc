// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

#include "helmsman/fill.h"

#include <string>

#include "common/check.h" 
#include "common/convert.h" 


bool FillImu(const string& raw, 
             Imu* imu) {
  imu->Reset();
}

bool FillWind(const string& raw, 
              WindRad* wind_rad) {
  wind_rad->Reset();
}

bool FillDrivesActual(const string& raw, 
                      DriveActualValuesRad* drive_actual_rad) {
  drive_actual_rad->Reset();
}


