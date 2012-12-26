// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: grundmann@google.com (Steffen Grundmann)

#ifndef COMMON_POINT_OF_SAIL_H_
#define COMMON_POINT_OF_SAIL_H_


// An asymmetric filter. Input pulses pass through and cause a
// slow return to zero. prev serves as state.
// decay is the maximum amount of decrease of the output per call.
double FilterOffset(double offset, double decay, double* prev);
double PositiveFilterOffset(double in, double decay, double* prev);
double NegativeFilterOffset(double in, double decay, double* prev);

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
                         double previous_output,  // previous output direction, needed to implement hysteresis
                         SectorT* sector,      // sector codes for state handling and maneuver
                         double* target);      // target angle for maneuver

  // If the sector is stable, we can calculate a bearing correction in response to fast wind
  // turns.
  double AntiWindGust(SectorT sector,       // sector codes
                      double alpha_app_rad, // apparent wind vector direction
                      double mag_app_m_s);  // apparent wind vector magnitude


 private:
  double buffer1_;
  double buffer2_;
};

#endif  // COMMON_POINT_OF_SAIL_H_
