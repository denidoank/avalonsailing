// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011


#include "helmsman/c_lift_to_rudder_angle.h"

#include "common/convert.h"
#include "common/unknown.h"
#include "lib/testing/testing.h"


// print clift to alpha chart.
// Columns: C_Lift, angle/degree, limited ,
// at 0.5, 1, 2, 3, 6 kts water speed.
void LogCLiftDiagram() {
  double speeds_kn[] = {-1, 0.5, 1, 2, 3, 6};
  for (size_t i = 0; i < sizeof(speeds_kn) / sizeof(speeds_kn[0]); ++i) {
    double water_speed = KnotsToMeterPerSecond(speeds_kn[i]);
    printf("\n For water_speed: %g m/s\n", water_speed);
    printf("C_Lift, angle/degree, limited\n");
    for (double c_lift = -1.5; c_lift <= 1.5; c_lift += 0.2) {
      int limited;
      double rudder_angle;
      CLiftToRudderAngle (c_lift,
                          water_speed,
                          &rudder_angle,
                          &limited);
      printf("%6.3g,%6.3g,%6d\n",
             c_lift, Rad2Deg(rudder_angle), limited);
    }
  }
}

// print speed to alpha chart.
// Columns: C_Lift, angle/degree, limited ,
// at 0.5, 1, 2, 3, 6 kts water speed.
void LogCLiftOverSpeedDiagram() {
  printf("Desired C_lift = 1.5. This is never feasible, i.e. we are always limited./n");
  printf("speed / m/s, angle/degree, limited\n");
  for (double water_speed = -10; water_speed < 10; water_speed += 1) {
    double c_lift = 1.5;  // thus we will always be limited
    int limited;
    double rudder_angle;
    CLiftToRudderAngle (c_lift,
                        water_speed,
                        &rudder_angle,
                        &limited);
    printf("%6.3g,%6.3g,%6d\n",
           water_speed, Rad2Deg(rudder_angle), limited);
  }
  printf("Desired C_lift = 0.5 . Never limited./n");
  printf("speed / m/s, angle/degree, limited\n");
  for (double water_speed = -10; water_speed < 10; water_speed += 1) {
    double c_lift = 0.5;
    int limited;
    double rudder_angle;
    CLiftToRudderAngle (c_lift,
                        water_speed,
                        &rudder_angle,
                        &limited);
    printf("%6.3g,%6.3g,%6d\n",
           water_speed, Rad2Deg(rudder_angle), limited);
  }
}



TEST(CLiftToAngleTest, All) {
  double c_lift = 0.5;    // result must be 4-5 degree
  double water_speed_m_s = 3;
  int limited;
  double rudder_angle;
  CLiftToRudderAngle(c_lift,
                     water_speed_m_s,
                     &rudder_angle,
                     &limited);
  EXPECT_FLOAT_EQ(4.4722719, Rad2Deg(rudder_angle));
  EXPECT_EQ(0, limited);

  c_lift = 0.5;
  water_speed_m_s = -3;
  CLiftToRudderAngle(c_lift,
                     water_speed_m_s,
                     &rudder_angle,
                     &limited);
  EXPECT_FLOAT_EQ(-9.61538, Rad2Deg(rudder_angle));
  EXPECT_EQ(0, limited);

  c_lift = 1.5;
  water_speed_m_s = 13;
  CLiftToRudderAngle(c_lift,
                     water_speed_m_s,
                     &rudder_angle,
                     &limited);
  EXPECT_FLOAT_EQ(10, Rad2Deg(rudder_angle));
  EXPECT_EQ(1, limited);

  c_lift = -1.5;
  water_speed_m_s = 13;
  CLiftToRudderAngle(c_lift,
                     water_speed_m_s,
                     &rudder_angle,
                     &limited);
  EXPECT_FLOAT_EQ(-10, Rad2Deg(rudder_angle));
  EXPECT_EQ(-1, limited);

  c_lift = 1.5;
  water_speed_m_s = 13;
  CLiftToRudderAngle(c_lift,
                     water_speed_m_s,
                     &rudder_angle,
                     &limited);
  EXPECT_FLOAT_EQ(10, Rad2Deg(rudder_angle));
  EXPECT_EQ(1, limited);

  c_lift = -1.5;
  water_speed_m_s = 13;
  CLiftToRudderAngle(c_lift,
                     water_speed_m_s,
                     &rudder_angle,
                     &limited);
  EXPECT_FLOAT_EQ(-10, Rad2Deg(rudder_angle));
  EXPECT_EQ(-1, limited);

  LogCLiftDiagram();
  LogCLiftOverSpeedDiagram();
}

int main(int argc, char* argv[]) {
  CLiftToAngleTest_All();
  return 0;
}

