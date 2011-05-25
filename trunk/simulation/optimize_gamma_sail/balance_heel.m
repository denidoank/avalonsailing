% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function heel = balance_heel(state, inputs)
% function heel = balance_heel(state, inputs)
% Iterate until the balance of wind force and heeling is found.

global global_state;
global global_inputs;
global_state = state;
global_inputs = inputs;

[F, N, omega_drives] = forces(state, inputs, 0);
heel_0 = 0.8 * heeling(N(1, 1));
[heel, fval, info] = fzero("x_torque", [heel_0], optimset("TolX", 0.005));
assert(info == 1, "fzero failed.");
endfunction





