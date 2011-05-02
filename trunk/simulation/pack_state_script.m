% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

% Script to "pack" all state variables, i.e. construct the state vector from its elements.
% Attention: THIS IS A SCRIPT!
assert(isvarname("x"));
assert(isvarname("y"));
assert(isvarname("z"));
assert(isvarname("phi_x"));
assert(isvarname("phi_y"));
assert(isvarname("phi_z"));
assert(isvarname("v_x"));
assert(isvarname("v_y"));
assert(isvarname("v_z"));
assert(isvarname("omega_x"));
assert(isvarname("omega_y"));
assert(isvarname("omega_z"));
assert(isvarname("gamma_S"));
assert(isvarname("gamma_R1"));
assert(isvarname("gamma_R2"));

% By definition state is 
state = [... 
  x;
  y;
  z;
  phi_x;
  phi_y;
  phi_z;
  v_x;
  v_y;
  v_z;
  omega_x;
  omega_y;
  omega_z;
  gamma_S;
  gamma_R1;
  gamma_R2];
S = simulation_options();
assert(S.no_of_states == length(state));
