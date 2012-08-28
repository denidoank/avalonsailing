// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

// contants of the NACA0010 profile used for the rudders, see
// https://docs.google.com/a/google.com/document/d/1BgF8fvjCNbGezJQp2gilepTe0SjeRyiNkGsDLWKS5-4/edit?hl=en#
// and the JAVAFOIL software documentation please.
// See also the NACA0010* data in ~/grundmann/avalon/JavaFoil
// get javafoil.jar and mhclasses.jar, then
// cd /home/grundmann/avalon/JavaFoil/
// java -classpath ./java/javafoil.jar:./java/mhclasses.jar:/usr/java/lib/rt.jar -jar ./java/javafoil.jar

// The optimal angles of attack given by other sources are higher because
// they apply for bigger Reynold numbers, i.e. are valid at higher speeds.
// We operate in a region of low speeds.

// Reynold numbers (rounded) for Avalon
// kts  m/s    Re(Keel)    Re(Rudder)
//  1    0.514  175000       82000
//  3    1.542  350000      250000
//  6    3.084  700000      500000


#ifndef HELMSMAN_NACA0010_H_
#define HELMSMAN_NACA0010_H_

#include <math.h>
#include "common/convert.h"

namespace naca0010 {

const double kCLiftPerRad = 0.1118 * 180 / M_PI;        // 1/rad
const double kCLiftPerRadReverse = 0.052 * 180 / M_PI;  // 1/rad

// Below speed_1 we apply limit_1 etc. .
const double kAlphaLimit0Rad = Deg2Rad(6);  // I made this up.
const double kSpeed0_m_s = 0.25;
const double kAlphaLimit1Rad = Deg2Rad(7);
const double kSpeed1_m_s = 0.5;
const double kAlphaLimit2Rad = Deg2Rad(8);
const double kSpeed2_m_s = 1.5;
const double kAlphaLimit3Rad = Deg2Rad(9);
const double kSpeed3_m_s = 3;
const double kAlphaLimit4Rad = Deg2Rad(10);

}  // namespace naca0010

#endif  // HELMSMAN_NACA0010_H_

