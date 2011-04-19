% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function [c_lift, c_drag, c_arm] = c_aero_of(part, speed_m_per_s, angle_rad)

% part = "keel", "rudder" or "sail"
% speed in m/s
% angle_rad in radians, angle of attack AOA between the direction of the free
% stream airflow and the chord line of the profile 

% http://en.wikipedia.org/wiki/Lift-induced_drag
% angle
% http://www.aerospaceweb.org/question/airfoils/q0150b.shtml

% this is true for symmetrical profiles only
if angle_rad > 0
  sign = 1;
else
  sign = -1;
  angle_rad = abs(angle_rad);
endif

if abs(angle_rad) > 10
  error("angle in radians please!")
endif

rey = reynolds_of(part, speed_m_per_s);

if strcmp(part, "sail") 
  [r1, t1, r2, t2] = polar_tab_sail1(rey);
else
  % keel and rudder have NACA0010
  [r1, t1, r2, t2] = polar_tab_naca0010(rey);
endif

alpha = t1(:,1);

c1 = interp1(alpha, t1(:,2:4), rad2deg(angle_rad), "spline");
c2 = interp1(alpha, t2(:,2:4), rad2deg(angle_rad), "spline");

c = [c1; c2];
if r1 != r2 
  c_final = interp1([r1; r2], [c1; c2], rey, "*linear");
else
  c_final = c1;
endif

c_lift = sign * c_final(1) * lambda_factor_of(part);
c_drag = c_final(2) + c_induced_of(part, c_final(1) ); 
c_arm = c_final(3);
