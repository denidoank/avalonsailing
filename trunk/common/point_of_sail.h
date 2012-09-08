// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

#ifndef COMMON_POINT_OF_SAIL_H_
#define COMMON_POINT_OF_SAIL_H_


typedef enum Sector {
  TackPort = 1,
  TackStar,
  ReachStar,
  JibeStar,
  JibePort,
  ReachPort
} SectorT;



class PointOfSail {
 public:
  PointOfSail();
  // Reset all buffers.
  void Reset();

  // Calculate the sailable heading under the given circumstances.
  // target is not written for sector == ReachStar or target == ReachPort
  double SailableHeading(double alpha_star,    // desired heading alpha*
                         double alpha_true,    // true wind vector direction
                         double alpha_app,     // apparent wind vector direction
                         double mag_app,       // apparent wind vector magnitude
                         double phi_z,         // boat direction
                         double previous_output,  // previous output direction, needed to implement hysteresis
                         SectorT* sector,      // sector codes for state handling and maneuver
                         double* target);      // target angle for maneuver

 private:
  double buffer1_;
  double buffer2_;
  double buffer3_;
  double buffer4_;
};

#endif  // COMMON_POINT_OF_SAIL_H_
