% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

function test_result = collision_sim(x0_a, mag_v_a, x0_b, v_b, phi_a, time, plot_this)

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

if nargin < 7
  plot_this = 1;
endif

v_a = mag_v_a * [cos(phi_a); sin(phi_a)];

% Allowed error between A and B postion at collision time.
epsilon = 1;
test_result = 1;

switch length(phi_a) 
  case {0}
    course_a_1_x = [];
    course_a_1_y = [];
    course_a_2_x = [];
    course_a_2_y = [];

    course_b_1_x   = [];
    course_b_1_y   = [];
    course_b_2_x   = [];
    course_b_2_y   = [];
    coll_x         = [];          
    coll_y         = [];         
  case {1}
    x_ca1 = x0_a + time(1) * v_a(:,1);
    x_cb1 = x0_b + time(1) * v_b;

    course_a_1_x = [x0_a(1) x_ca1(1)];
    course_a_1_y = [x0_a(2) x_ca1(2)];
    course_a_2_x = [];
    course_a_2_y = [];

    course_b_1_x   = [x0_b(1) x_cb1(1)];
    course_b_1_y   = [x0_b(2) x_cb1(2)];
    course_b_2_x   = [];
    course_b_2_y   = [];
    coll_x         = [x_ca1(1)];          
    coll_y         = [x_ca1(2)];           
    
    if sumsq(x_ca1 - x_cb1) > epsilon^2
      deviation1 = x_ca1 - x_cb1;
      test_result = 0;
    endif
     
  case {2}
    x_ca1 = x0_a + time(1) * v_a(:,1);
    x_ca2 = x0_a + time(2) * v_a(:,2);
    x_cb1 = x0_b + time(1) * v_b;
    x_cb2 = x0_b + time(2) * v_b;

    course_a_1_x = [x0_a(1) x_ca1(1)];
    course_a_1_y = [x0_a(2) x_ca1(2)];
    course_a_2_x = [x0_a(1) x_ca2(1)];
    course_a_2_y = [x0_a(2) x_ca2(2)];

    course_b_1_x   = [x0_b(1) x_cb1(1)];
    course_b_1_y   = [x0_b(2) x_cb1(2)];
    course_b_2_x   = [x0_b(1) x_cb2(1)];
    course_b_2_y   = [x0_b(2) x_cb2(2)];
    coll_x         = [x_ca1(1) x_ca2(1)];          
    coll_y         = [x_ca1(2) x_ca2(2)];           

    if sumsq(x_ca1 - x_cb1) > epsilon^2 || ...
       sumsq(x_ca2 - x_cb2) > epsilon^2
      deviation1 = x_ca1 - x_cb1;
      deviation2 = x_ca2 - x_cb2;
      test_result = 0;
    endif
  otherwise
    error("unexpected length");
endswitch

if ! plot_this
  return
endif

%figure
title("Collision simulation");

plot(  ...
    course_a_1_x, course_a_1_y, "r-*;A1;",
    course_a_2_x, course_a_2_y, "m-*;A2;",
    course_b_1_x, course_b_1_y, "g-^;B1;",
    course_b_2_x, course_b_2_y, "b-+;B2;",
    coll_x, coll_y, "bo;C;"
)
axis("equal");
xlabel("x / m");
ylabel("y / m");

"Done, switch to Graphics window!"
pause






endfunction

