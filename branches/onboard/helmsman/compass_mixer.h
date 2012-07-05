// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, April 2012
#ifndef HELMSMAN_COMPASS_MIXER_H
#define HELMSMAN_COMPASS_MIXER_H


// A class to keep the internal state of the SignalWatchMen.
class CompassMixer {
 public:
  // Mix 3 angles in radians. Something like the weighted average of the
  // alphas.
  // The angles wrap around. input angles in [-M_PI, 2*M_PI).
  // The weights are in [0, 1].
  // 0 means "ignore this", 1 means "rock solid reliable measurement"
  // A vector interface would be good for the mixer, but awkward for the caller.
  // Returns consensus < 0.5 if no good input data are available or no consens can be found.
  double Mix(double alpha1_rad, double weight1,
             double alpha2_rad, double weight2,
             double alpha3_rad, double weight3,
             double* consensus);
 private:
  double input_[3];
  double weight_[3];
};

#endif   // HELMSMAN_COMPASS_MIXER_H
