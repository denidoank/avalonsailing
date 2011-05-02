% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function S = simulation_options()
% function S = simulation_options()
% simulation options

S.no_of_states = 15;
S.no_of_inputs = 8;

S.T = 0.1;
lsode_options ("absolute tolerance", 1E-4);
lsode_options ("relative tolerance", 1E-4);
lsode_options ("minimum step size", 0.0002);
lsode_options ("maximum step size", 0.002);  % 0.002s seems to be a value guaranteeing stability and accuracy


% activate later!
% ignore_function_time_stamp ("all")     % on
% ignore_function_time_stamp ("system")     % back to default
% addpath("~/octave")
endfunction

   