% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

% Script to "pack" all inputs variables, i.e. construct the inputs vector from its elements.
% Attention: THIS IS A SCRIPT!
assert(isvarname("wind_alpha"));
assert(isvarname("wind_magnitude"));
assert(isvarname("stream_alpha"));
assert(isvarname("stream_magnitude"));
assert(isvarname("z_0"));
assert(isvarname("gamma_S_star"));
assert(isvarname("gamma_R1_star"));
assert(isvarname("gamma_R2_star"));

inputs = [ ...
wind_alpha;
wind_magnitude;
stream_alpha;
stream_magnitude;
z_0;
gamma_S_star;
gamma_R1_star;
gamma_R2_star];

S = simulation_options();
assert(S.no_of_inputs == length(inputs));