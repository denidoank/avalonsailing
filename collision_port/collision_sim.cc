% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

bool CollisionSim(const Cartesian& x0_a, doublemag_v_a, const Cartesian& x0_b, const Cartesian& v_b, 
     const vector<double>& phi_a, const vector<double>& time) {
/*
% function [alpha_collision, t] = collision(x0_a, mag_v_a, x0_b, v_b) 
% 2 mobile objects A and B, initially at positions x0_a and x0_b, where
% x0_a = [ x_a
%          y_a ]
% x0_b = [ x_b
%          y_b ]
% can possibly collide. Given the speed vector of the B object v_b, where
% v_b = [ v_x_b
%         v_y_b ],
% and the magnitude of the speed of the first object A what a direction must A steer to
% hit B and when in the future will the collision happen, if at all?
% x0_a and x0_b are the initial position of the objects (e.g. ships)
% mag_v_a is the magnitude of A's speed
% v_b is B's  speed vector
% phi is a vector of size 0 to 2 containing the course A has to take to collide.
% t is a corresponding vector containing the times when this will happen. 
% The return value does not guarantee he correctness of the calulation
% but if it is false then phi_a and t were wrong. 
% positions and speeds have to be given in compatible units.
% All this in a conventional cartesian coordinate system.
% 
%  y ^                <-.
%    |                   \
%    +--->           phi  |
%        x
% all x_ are positions, all v_ are speeds
*/

// Allowed error between A and B postion at collision time.
double epsilon = 1;
bool test_result = 1;

for (int i = 0; i < alpha.size(); ++i) {
  double v_a_x = mag_v_a * cos(phi_a[i]); 
  double v_a_y = mag_v_a * sin(phi_a[i]); 

  double x_a_c = x0_a.x + t[i] * v_a_x;
  double y_a_c = x0_a.y + t[i] * v_a_y;

  double x_b_c = x0_b.x + t[i] * v_b.x;
  double y_b_c = x0_b.y + t[i] * v_b.y;
  
  if ( (x_a_c-x_b_c) * (x_a_c-x_b_c) + (y_a_c-y_b_c) * (y_a_c-y_b_c) > epsilon*epsilon)
    test_result = false;
}

return test_result;
}


