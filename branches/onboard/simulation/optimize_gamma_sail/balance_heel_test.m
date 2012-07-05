% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, May 2011

function result = balance_heel_test()

debug_switch_init();
S = simulation_options();
result = 1;

"Sail at -20 degrees and wind from portside leads to heel to starboard."
state = zeros(S.no_of_states, 1);
inputs = zeros(S.no_of_inputs, 1);
unpack_state_script;
gamma_S = deg2rad(-20);
pack_state_script;
unpack_inputs_script;
wind_alpha = deg2rad(90);
wind_magnitude = 5;
gamma_S_star = deg2rad(-20);
pack_inputs_script;
heel_opt = balance_heel(state, inputs)
heel_degrees = rad2deg(heel_opt)
unpack_state_script;
phi_x = heel_opt;
pack_state_script;
assert_eq(5.7, heel_degrees, 1);

"Sail at 45 degrees and 10m/s wind from portside leads to bigger heel to starboard."
state = zeros(S.no_of_states, 1);
inputs = zeros(S.no_of_inputs, 1);
unpack_state_script;
gamma_S = deg2rad(45);
pack_state_script;
unpack_inputs_script;
wind_alpha = deg2rad(90);
wind_magnitude = 10;
gamma_S_star = deg2rad(45);
pack_inputs_script;
heel_opt = balance_heel(state, inputs)
heel_degrees = rad2deg(heel_opt)
unpack_state_script;
phi_x = heel_opt;
pack_state_script;
[F, N, omega_drives] = forces(state, inputs, 0)
assert_eq(16, heel_degrees, 1);

endfunction