% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function N_x = x_torque(heel)
% N_x = x_torque(heel)
% helper function to find the equilibrium of wind
% forces and righting moment of the boat.
% Needs global_state and global_inputs as global variables.
global global_state;
global global_inputs;

state = global_state;
unpack_state_script;
phi_x = heel;
pack_state_script;
[F, N, omega_drives] = forces(state, global_inputs, 0);
N_x = [N(1, 1)];  
endfunction



