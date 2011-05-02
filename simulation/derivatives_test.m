% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function result = derivatives_test()
  
result = 1;
S = simulation_options();
debug_switch_init();
state = zeros(S.no_of_states,1);
inputs = zeros(S.no_of_inputs, 1);
state_inputs = [state; inputs];
x_dot = derivatives(state_inputs);
assert_eq(zeros(S.no_of_states + S.no_of_inputs, 1), x_dot);

unpack_state_script;
v_x = 2;
pack_state_script;
state_inputs = [state; inputs];
t1 = time();
x_dot = derivatives(state_inputs);
runtime_derivatives = 1000 * (time() - t1)
endfunction