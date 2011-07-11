% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011
 
function [alpha, phi_c_full, t_c_full, v_a_full, blocked_start_full, blocked_end_full, near_pass_by] = ...
  collision_avoidance_full(mag_v_a_min, mag_v_a_max, x0, v, t_scope, minimum_distance) 
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


phi_c_full = [];
t_c_full = [];
v_a_full = [];
blocked_start_full = [];
blocked_end_full = [];
near_pass_by_full = 0;

for mag_v_a=mag_v_a_min:((mag_v_a_max-mag_v_a_min)/25):mag_v_a_max
  [alpha, phi_c, t_c, blocked_start, blocked_end, near_pass_by] = ...
      collision_avoidance(mag_v_a, x0, v, t_scope, minimum_distance);
  phi_c_full = [phi_c_full phi_c];
  t_c_full = [t_c_full t_c];
  % We might get 0 to 2 solutions and need a multiplicity of mag_v_a.
  v_a_full = [v_a_full mag_v_a*ones(1, length(t_c)) ];
  blocked_start_full = [blocked_start_full blocked_start];
  blocked_end_full = [blocked_end_full blocked_end];
  if near_pass_by
    near_pass_by_full = 0;
  endif
endfor


[blocked_start_full, blocked_end_full] = merge_blocks(blocked_start_full, blocked_end_full);

endfunction