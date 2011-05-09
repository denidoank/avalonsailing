% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

% unpack_sim_inputs_script
% Script to "unpack" all time vectors of input variables i_sim
% into reasonably named vectors. Attention: THIS IS A SCRIPT!
assert(isvarname("i_sim"));
Sqx = simulation_options();
assert((Sqx.no_of_inputs) == columns(i_sim));

last = rows(i_sim);

% inputs=[...
wind_alpha = x_sim(1:last, 1);
wind_magnitude = x_sim(1:last, 2);
stream_alpha = x_sim(1:last, 3);
stream_magnitude = x_sim(1:last, 4);
z_0 = x_sim(1:last, 5);         % z_0 for waves
gamma_S_star = x_sim(1:last, 6);
gamma_R1_star = x_sim(1:last, 7);
gamma_R2_star = x_sim(1:last, 8);