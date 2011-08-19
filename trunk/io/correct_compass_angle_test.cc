// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, Aug 2011

#include <algorithm>
#include "correct_compass_angle.h"

#include "lib/testing/testing.h"

using std::min;
using std::max;


#define EXPECT_NEAR(expected, error, act) { \
  double tmp_actual = act; \
  EXPECT_IN_INTERVAL(expected - error, tmp_actual, expected + error); \
}



double DegMin(double deg, double minu) {
  CHECK(deg != 0);  // woud need a West/East flag.
  return deg + sign(deg) * minu / 60.0;
}

double Decl(double lat_deg, double lat_min, double lon_deg, double lon_min) {
  double decl = 0;
  CorrectCompassAngle(DegMin(lat_deg, lat_min), DegMin(lon_deg, lon_min), &decl);
  return decl;
}

ATEST(CorrectCompassDirection, AgainstExternalReferences) {



/*
Test references from:
http://geomag.nrcan.gc.ca/apps/mdcal-eng.php?Year=2011&Month=8&Day=17&Lat=48&Min=15&LatSign=1&Long=5&Min2=21&LongSign=-1&Submit=Calculate+magnetic+declination&CityIndex=0

IGRF-2010 Results: 
Year: 2011 08 17	Latitude: 47¡ 21' North	Longitude: 8¡ 32' East
 
Calculated magnetic declination: 1¡ 30' East
*/
EXPECT_FLOAT_EQ(DegMin(1, 30), Decl( 47, 21,	 8, 32));
/*

Year: 2011 08 17	Latitude: 48¡ 15' North	Longitude: 5¡ 21' West
 
Calculated magnetic declination: 2¡ 55' West
*/
EXPECT_FLOAT_EQ(DegMin(-2, 55), Decl( 48, 15,	 -5, 21));
/*
 	 	 
Year: 2011 08 17	Latitude: 19¡ North	Longitude: 27¡ West
 
Calculated magnetic declination: 11¡ 17' West
*/
EXPECT_FLOAT_EQ(DegMin(-11, 17), Decl( 19, 0,	 -27, 0));
/*
 
IGRF-2010 Results: 
Year: 2011 08 17	Latitude: 19¡ North	Longitude: 55¡ West
 
Calculated magnetic declination: 16¡ 22' West
*/
EXPECT_FLOAT_EQ(DegMin(-16, 22), Decl( 19, 0,	 -55, 0));
/*


}




int main(int argc, char **argv) {
  return testing::RunAllTests();
}
