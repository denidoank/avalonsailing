// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011


#include "skipper/collision.h"

#include "common/convert.h"
#include "lib/testing/testing.h"



TEST(Collision, All) {
/*
% 2 mobile objects A and B, initially at positions x0_a and x0_b, where
% x0_a = [ x_a
%          y_a ]
% x0_b = [ x_b
%          y_b ]
% can possibly collide. Given the speed vector of the B object v_b, where
% v_b = [ v_x_b
%         v_y_b ],
% and the magnitude of the first object A what a direction must A steer to
% hit B and when will the collision happen?
*/

vector<double> alpha;
vector<double> t;
vector<double> phi_t;

Cartesian x0_a(0,  0);
mag_v_a =  5;
Cartesian x0_b(  0;  1000);
Cartesian v_b(3.5355; -3.5355);
Collision(x0_a, mag_v_a, x0_b, v_b, &phi, &time, &alpha);

EXPECT_EQ(true, CollisionSim(x0_a, mag_v_a, x0_b, v_b, phi, time));

EXPECT_EQ_TOL(-0.78539, alpha[0], 0.001);
EXPECT_EQ_TOL(141.42, time[0], 0.002);
EXPECT_EQ_TOL(0.78539, phi[0], 0.001);

/*
% rotated standard cases
rotation = [ 0 -1;
             1  0];
*/
x0_a = Cartesian(0;  0);
mag_v_a =  5;
x0_b = Cartesian(0, 1000);
v_b = Cartesian(3.5355, -10);
Collision(x0_a, mag_v_a, x0_b, v_b, &phi, &time, &alpha);
EXPECT_EQ(true, CollisionSim(x0_a, mag_v_a, x0_b, v_b, phi, time));
EXPECT_EQ_TOL(-0.78539, alpha[0], 0.001);
EXPECT_EQ_TOL(3.92698,  alpha[1], 0.001);
EXPECT_EQ_TOL(73.879,  time[0], 0.002);
EXPECT_EQ_TOL(154.693, time[1], 0.002);
EXPECT_EQ_TOL(0.78539, phi[0], 0.001);
EXPECT_EQ_TOL(5.49778, phi[1], 0.001);

/* I leave this as an exercise task to the inclined reader.
% Rotate this case by arbitrary angles
x0_a = Cartesian(0;  0);
mag_v_a =  5;
for r = 0:(pi/23+0.08741):(2*pi)
  rotation = [ cos(r) -sin(r);
               sin(r)  cos(r)];
  x0_b = rotation * [1000; 0]
  v_b = rotation * [ -10; -3.5355]
  Collision(x0_a, mag_v_a, x0_b, v_b, &phi, &time, &alpha);
  EXPECT_EQ(true, CollisionSim(x0_a, mag_v_a, x0_b, v_b, phi, time));
  EXPECT_EQ_TOL([-0.78539   3.92698], alpha, 0.001);
  EXPECT_EQ_TOL([73.879   154.693], time, 0.002);
  assert_sorted_eq(normalize(r + [5.4978 3.92698]), phi, 0.001);
endfor
*/

% Standard cases
x0_a = Cartesian(0;  0);
mag_v_a =  5;
x0_b = Cartesian(  1000;  0);
v_b = Cartesian( 4.5; 0);
Collision(x0_a, mag_v_a, x0_b, v_b, &phi, &time, &alpha);
EXPECT_EQ(0.0, alpha);
EXPECT_EQ(2000, time);
EXPECT_EQ(true, CollisionSim(x0_a, mag_v_a, x0_b, v_b, phi, time));

x0_a = Cartesian(0;  0);
mag_v_a =  5;
x0_b = Cartesian(  1000;  0);
v_b = Cartesian( -3.5355; 3.5355);
Collision(x0_a, mag_v_a, x0_b, v_b, &phi, &time, &alpha);
EXPECT_EQ_TOL(0.78539, alpha, 0.001);
EXPECT_EQ_TOL(141.42, time, 0.002);
EXPECT_EQ(true, CollisionSim(x0_a, mag_v_a, x0_b, v_b, phi, time));

x0_a = Cartesian(0;  0);
mag_v_a =  5;
x0_b = Cartesian(  1000;  0);
v_b = Cartesian( -5; 0);
Collision(x0_a, mag_v_a, x0_b, v_b, &phi, &time, &alpha);
EXPECT_EQ(0.0, alpha);
EXPECT_EQ(100, time);
EXPECT_EQ(true, CollisionSim(x0_a, mag_v_a, x0_b, v_b, phi, time));

x0_a = Cartesian(0;  0);
mag_v_a =  5;
x0_b = Cartesian(  1000;  0);
v_b = Cartesian( 5; 0);
Collision(x0_a, mag_v_a, x0_b, v_b, &phi, &time, &alpha);
EXPECT_EQ(0, alpha.size());
EXPECT_EQ(0, time.size());
EXPECT_EQ(true, CollisionSim(x0_a, mag_v_a, x0_b, v_b, phi, time));

x0_a = Cartesian(0;  0);
mag_v_a =  5;
x0_b = Cartesian(  1000;  0);
v_b = Cartesian( -3.5355; -3.5355);
Collision(x0_a, mag_v_a, x0_b, v_b, &phi, &time, &alpha);
EXPECT_EQ_TOL(-0.78539, alpha, 0.001);
EXPECT_EQ_TOL(141.42, time, 0.002);
EXPECT_EQ(true, CollisionSim(x0_a, mag_v_a, x0_b, v_b, phi, time));

x0_a = Cartesian(0;  0);
mag_v_a =  5;
x0_b = Cartesian(  1000;  0);
v_b = Cartesian( -10; -3.5355);
Collision(x0_a, mag_v_a, x0_b, v_b, &phi, &time, &alpha);
EXPECT_EQ_TOL(-0.78539, alpha[0], 0.001);
EXPECT_EQ_TOL(3.92698, alpha[1], 0.001);
EXPECT_EQ_TOL(73.879, time[0], 0.002);
EXPECT_EQ_TOL(154.693, time[1], 0.002);
EXPECT_EQ_TOL(5.4978, phi[0], 0.001);
EXPECT_EQ_TOL(3.92698, phi[1], 0.001);
EXPECT_EQ(true, CollisionSim(x0_a, mag_v_a, x0_b, v_b, phi, time));

}


int main(int argc, char* argv[]) {
  Collision_All();
  return 0;
}
