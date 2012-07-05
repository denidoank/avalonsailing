// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011
#ifndef SKIPPER_TARGET_CIRCLE_H
#define SKIPPER_TARGET_CIRCLE_H

/*
The TargetCircle class helps to find the optimal path
to our goal. According to the rules of the Micro-Transat
race our boat has to reach a certain target point in the
Caribean sea. 

In the vicinity of the target point the boat shall steer
towards that target point and the boats desired heading
can be calculated with basic trigonometry from the positions
of target point and boat. Lets assume that we can draw a 
circle of a certain radus around the target point that is
free of any obstacles like coasts or islands. Then our boat
can safely sail towards target point at any time.
Thus we may define a TargetCircle object equivalent to a 
circle with the target point as center and the radius and
define operations like ToCenter() returning the heading to
the center for a given boat position or In() returning true
if our boat is in that TargetCircle.

Now lets build a cascade of TargetCircles such that
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


// Coordinate system convention
// x and y are the usual cartesian coordinate axis, angle sign definition is mathematical, i.e. angles go counterclockwise.
// The spherical distortions are not small but do not compromise the function.
// Fortunately the x-y coordinate system can be turned upside down and is then
// equivalent to a North-East-system where the angle has clockwise sign definition
// as we need it for the headng angle output.
// All we have to do is to add a minus sign to all West and South coordinates.


// the angular error is 10 degree because of the scaling of the longitude.
// eliminate the scalar versions , work with degree only!


#include <math.h>
#include "common/check.h"
#include "skipper/lat_lon.h"

class TargetCircle {
 public:
  // x0, y0 and radius in degree. (x0 center latitude, y0 center longitude)
  TargetCircle(double x0, double y0, double radius);

  // Lat-Lon-input
  TargetCircle(LatLon lat_lon, double radius_deg);

  // Distance of [x, y] to the center is not more than the radius.
  bool In(double x, double y, double expansion = 1) const;
  bool In(LatLon lat_lon, double expansion = 1) const;
  bool In(const TargetCircle& t) const;

  // Heading from [x, y] towards the center of this target circle, in degrees.
  double ToDeg(double x, double y) const;
  double ToDeg(LatLon lat_lon) const;

  double Distance(double x, double y) const;
  double x0() const;
  double y0() const;
  double radius() const;


 private: 
  double DistanceSquared(double x, double y) const;
  
  double x0_;  // in degree
  double y0_;  // in degree
  double radius_squared_;  // in deg^2
  double lon_factor_;
};

#endif  // SKIPPER_TARGET_CIRCLE_H
