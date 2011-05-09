% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function result = simu_4()
% Compare to simu_3 (Simple Euler integration), here we use lsode.
% max_stap_width 2ms,
% period of z oscillations < 0.75s at amplitude 0.00001 
% z_0 = -0.00001, T = 0.727s
% z_0 = -0.001, T = 0.727s
% z_0 = -0.1, T = 0.725s, stronger decrease of amplitudes


% Options for LSODE include:
%
%  keyword                                             value
%  -------                                             -----
%  absolute tolerance                                  1.5e-08
%  relative tolerance                                  0.3
%  integration method                                  stiff
%  initial step size                                   0.01
%  maximum order                                       -1
%  maximum step size                                   0.002
%  minimum step size                                   0.01
%  step limit                                          100000
%



result = 1;
S = simulation_options();
debug_switch_init();
state = zeros(S.no_of_states,1);
inputs = zeros(S.no_of_inputs, 1);
saved_inputs(inputs);
unpack_state_script;  % needs state
z = -10;
pack_state_script;    % assembles state
x0 = state;
t = 0:0.01:10;

[x_sim, should_be_2, msg] = lsode("derivatives", x0, t)

if should_be_2 != 2
  disp(["lsode failed with: ", msg]);
endif
size(x_sim)

unpack_sim_results_script;
figure
plot(t, z);
title("z(t)");

figure
plot(t, v_z);
title("v_z(t)")

endfunction