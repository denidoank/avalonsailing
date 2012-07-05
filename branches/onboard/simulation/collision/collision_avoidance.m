% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011
 
function [alpha, phi_c, t_c, blocked_start, blocked_end, near_pass_by] = collision_avoidance(mag_v_a, x0, v, t_scope, minimum_distance) 
% Assume that we (object A) are at the origin of the coordinate system (0, 0).
% We can move with speed magnitude v_a into all directions.
% In our vicinity there a N other objects (e.g. ships) B_i, i E [1, N], each with a position x0_i
% and a speed vector v_i.
% We are interested in the directions that we can steer without colliding [*] with any B
% for a given future time horizon t_scope.
% x0 is a matrix of initial positions, each column for one B.
% v is a matrix of constant speeds, each column for one B.
% t_scope is or temporal scope
% [*] colliding is defined as getting closer than minimum_distance to that object .
% speed v, t_scope and minimum_distance have to be gven in compatible units to x0 and v.

phi_c = [];
t_c   = [];
for i=1:columns(x0)
  [phi_a, t] = collision([0; 0], mag_v_a, x0(:, i), v(:, i));
  phi_c = [phi_c phi_a];
  t_c   = [t_c t];
endfor

% Each collision point is surrounded by a circle of radius minimum_distance
blocked_start = [];
blocked_end = [];
near_pass_by = 0;
for i=1:columns(phi_c)
  way_c = mag_v_a * t_c(i);
  way_tangent_squared = way_c^2 - minimum_distance^2;  
  if way_tangent_squared > 0  
    way_tangent = sqrt(way_c^2 - minimum_distance^2);
    blocked_angle_half = asin(minimum_distance / way_c);
  else
    % The starting point of A lies within the circle around the 
    % collision point. I this case we block a sector of -+90 degrees
    % and note that it might be impossible to keep the minimum distance.
    blocked_angle_half = pi / 2;
    near_pass_by = 1;
  endif
  blocked_start = [blocked_start phi_c(i) - blocked_angle_half];
  blocked_end = [blocked_end phi_c(i) + blocked_angle_half];
endfor

[blocked_start, blocked_end] = merge_blocks(blocked_start, blocked_end);

% todo: pick best non blocked direction!
alpha=9

endfunction
