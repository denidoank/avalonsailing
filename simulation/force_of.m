% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function [force, center_of_force] = force_of(part, angle_of_attack, mag_speed, gamma)
% [F, pos] = force_of(part, angle_of_attack, mag_speed_m_per_s, gamma)
% Returns the force vector and the center of that force in boat coordinates.
% part = "keel", "rudder" or "sail"
% angle of attack:  in rad
% mag_speed_m_per_s: magnitude of speed in in m/s

assert(size(angle_of_attack) == [1 1]);
assert(size(mag_speed) == [1 1]);
assert(size(gamma) == [1 1]);

Const = physical_constants();
B = boat();
if strcmp(part, "keel")
  area = B.area_K;
  rho = Const.rho_water;
elseif strcmp(part, "rudder")
  area = B.area_R;
  rho = Const.rho_water;
elseif strcmp(part, "sail")
  area = B.area_S;
  rho = Const.rho_air;
else
  part
  error("part undefined");
endif
[c_lift, c_drag, c_arm] = c_aero3d_of(part, mag_speed, angle_of_attack);

F_lift = c_lift * rho * 0.5 * area * mag_speed^2;
F_drag = c_drag * rho * 0.5 * area * mag_speed^2;

% transform resulting vector into boat coordinate system
eps = gamma - angle_of_attack;
force = [ ...
  cos(eps)  -sin(eps)
  sin(eps)  cos(eps)
  0         0        ] * [-F_drag; F_lift];

center_of_force = center_of_force_of(part, gamma, c_arm);
endfunction