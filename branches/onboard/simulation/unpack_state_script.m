% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

% unpack_state_script, needs a vector variable named "state" as input.
% Script to "unpack" all state variables, i.e. puts the elements of the state
% vector into reasonably named scalars. Attention: THIS IS A SCRIPT!
assert(isvarname("state"));
Sqqqqq = simulation_options();
assert([Sqqqqq.no_of_states, 1] == size(state));
% By definition state is 
% state = [... 
% x;
% y;
% z;
% phi_x;
% phi_y;
% phi_z;
% v_x;
% v_y;
% v_z;
% omega_x;
% omega_y;
% omega_z;
% gamma_S;
% gamma_R1;
% gamma_R2];

x = state(1);
y = state(2);
z = state(3);
phi_x = state(4);
phi_y = state(5);
phi_z = state(6);
v_x = state(7);
v_y = state(8);
v_z = state(9);
omega_x = state(10);
omega_y = state(11);
omega_z = state(12);
gamma_S = state(13);
gamma_R1 = state(14);
gamma_R2 = state(15);