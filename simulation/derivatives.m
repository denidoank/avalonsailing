% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function x_dot = derivatives(state, t)
% derivatives of x for solution of differential equation.
S = simulation_options();
inputs = saved_inputs();
% position and angles of the boats body coordinate system
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
unpack_state_script;

speed = [v_x; v_y; v_z];
omega = [omega_x; omega_y; omega_z];

% position and angles of the boats body coordinate system
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

B = boat();
[F, N, omega_drives] = forces(state, inputs);

x_dot = [ ...
  speed;
  omega;
  B.mass_matrix_inverted * [F; N];  % for speed and omega
  omega_drives];

assert([S.no_of_states, 1] == size(x_dot));