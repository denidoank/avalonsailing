% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

% Script to "unpack" all inputs variables, i.e. puts the elements of the inputs
% vector into reasonably named scalars. Attention: THIS IS A SCRIPT!
assert(isvarname("inputs"));
Sqqqqqq = simulation_options();
assert([Sqqqqqq.no_of_inputs, 1] == size(inputs));
% By definition inputs is 
% inputs = [...
% wind_alpha;
% wind_magnitude;
% stream_alpha;
% stream_magnitude;
% z_0;
% gamma_S_star;
% gamma_R1_star;
% gamma_R2_star;

wind_alpha = inputs(1);
wind_magnitude = inputs(2);
stream_alpha = inputs(3);
stream_magnitude = inputs(4);
z_0 = inputs(5);
gamma_S_star = inputs(6);
gamma_R1_star = inputs(7);
gamma_R2_star = inputs(8);
