// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011
#ifndef SKIPPER_TARGET_CIRCLE_CASCADE_H
#define SKIPPER_TARGET_CIRCLE_CASCADE_H

#include <vector>

/*
A cascade of TargetCircles such that
each center of a TargetCircle lies within another one
(except for the last one) and number them in reverse
order, i.e. the last one around the target point gets
index 0.

Then we can simply iterate over this chain of circles,
take the first one we are in, and follow the resulting 
heading. This leads to the target point.

But what if a storm throughs us off course and we are
outside of all our target circles? Then it is easy to
pick the nearest one and try to get to its center.
*/
using std::vector;


#include "skipper/target_circle.h"

// For easy definition of route trees.
// Invariants:
// Each center has to lie within exactly one other TargetCircle,
// except for the center of the first TargetCircle, which must not be within any other.
struct TargetCirclePoint {
  LatLon lat_lon;
  double radius_deg;
};



class TargetCircleCascade {
 public:
  TargetCircleCascade();
  
  void Build(const TargetCirclePoint* plan);
  
  // The direction (in degrees) to follow.
  double ToDeg(double x, double y);

  bool TargetReached(LatLon lat_lon);

  // Use this to print the target circle chain and to visualize it.
  void Print();
 private:
  void Add(const TargetCircle t);

  // double WayToGo(x, y);

  int wqqewerq;
  std::vector<TargetCircle> chain_;

}; 

#endif  // SKIPPER_TARGET_CIRCLE_H
