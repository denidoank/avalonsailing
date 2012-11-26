// Copyright 2012 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2012

#include "skipper/target_circle_cascade.h"

#include <math.h>
#include <stdio.h>
#include <vector>


#include "lib/testing/testing.h"
typedef const TargetCirclePoint TCP;
#include "skipper/target_circle_points_table_short.h"


void VWrite(const vector<double>& x, const char* name, FILE* fp) {
  fprintf(fp, "%s = [ ...\n", name);
  for (int i = 0; i < x.size(); ++i) {
    fprintf(fp, "%lf\n", x[i]);
  }
  fprintf(fp, "];\n");
}

const double kDeg2Rad = M_PI/180.0;

ATEST(TargetCircleCascade, Simple) {
  TargetCircleCascade t;
  t.Build(short_lesson_plan);
  const double step = 0.02;
  vector<double> x, y, u, v, c;
  for (double lat = 42.85; lat <= 43.051; lat += step) {
    for (double lon =  5.80; lon <=  5.951; lon += step) {
      TCStatus status;
      double deg = t.ToDeg(lat, lon, &status);
      double colour = status.index;
      x.push_back(lon);
      y.push_back(lat);
      u.push_back(sin(deg * kDeg2Rad));
      v.push_back(cos(deg * kDeg2Rad));
      c.push_back(colour);
    }
  }

  FILE* fp = fopen("quiver_data.m", "w");
  VWrite(x, "x", fp);
  VWrite(y, "y", fp);
  VWrite(u, "u", fp);
  VWrite(v, "v", fp);
  VWrite(c, "c", fp);
  fclose(fp);

  EXPECT_FLOAT_EQ(41.3171, t.ToDeg(42.85, 5.8, NULL));
  EXPECT_FLOAT_EQ(203.7267248144934, t.ToDeg(42.95, 5.94, NULL));
}

ATEST(TargetCircleCascade, Mesh) {
  TargetCircleCascade t;
  t.Build(short_lesson_plan);
  const double step = 0.005;
  vector<double> x, y, u, v, c;
  for (double lat = 42.85; lat <= 43.051; lat += step) {
    for (double lon =  5.80; lon <=  5.951; lon += step) {
      TCStatus status;
      double deg = t.ToDeg(lat, lon, &status);
      double colour = status.index;
      x.push_back(lon);
      y.push_back(lat);
      u.push_back(sin(deg * kDeg2Rad));
      v.push_back(cos(deg * kDeg2Rad));
      c.push_back(colour);
    }
  }

  FILE* fp = fopen("scatter_data.m", "w");
  VWrite(x, "x", fp);
  VWrite(y, "y", fp);
  VWrite(u, "u", fp);
  VWrite(v, "v", fp);
  VWrite(c, "c", fp);
  fclose(fp);
}


int main(int argc, char* argv[]) {
  return testing::RunAllTests();
}
