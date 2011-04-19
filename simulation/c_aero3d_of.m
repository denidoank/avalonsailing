% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function [c_lift, c_drag, c_arm] = c_aero3d_of(part, speed_m_per_s, angle_rad)
% function [c_lift, c_drag, c_arm] = c_aero3d_of(part, speed_m_per_s, angle_rad)
% Aerodynamic coefficients for 3 dimensional case (with efects of wing tips)
% part = "keel", "rudder" or "sail"
% speed_m_per_s: speed in m/s
% angle_rad in radians, angle of attack AOA between the direction of the free
%   stream airflow and the chord line of the profile 
% See:
% http://en.wikipedia.org/wiki/Lift-induced_drag
% angle
% http://www.aerospaceweb.org/question/airfoils/q0150b.shtml

[c_lift_2d, c_drag_2d, c_arm_2d] = c_aero2d_of(part, speed_m_per_s, angle_rad);
c_lift = c_lift_2d * lambda_factor_of(part);
c_drag = c_drag_2d + c_induced_of(part, c_lift_2d); 
c_arm = c_arm_2d;