% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

% Script to "unpack" all time vectors of state variables, i.e. puts the elements of the lsode output matrix 
% into reasonably named vectors. Attention: THIS IS A SCRIPT!
disp("in script");
assert(isvarname("x_sim"));
Sqx = simulation_options();
assert((Sqx.no_of_states) == columns(x_sim));

last = rows(x_sim);

x = x_sim(1:last, 1);
y = x_sim(1:last, 2);
z = x_sim(1:last, 3);
phi_x = x_sim(1:last, 4);
phi_y = x_sim(1:last, 5);
phi_z = x_sim(1:last, 6);
v_x = x_sim(1:last, 7);
v_y = x_sim(1:last, 8);
v_z = x_sim(1:last, 9);
omega_x = x_sim(1:last, 10);
omega_y = x_sim(1:last, 11);
omega_z = x_sim(1:last, 12);
gamma_S = x_sim(1:last, 13);
gamma_R1 = x_sim(1:last, 14);
gamma_R2 = x_sim(1:last, 15);