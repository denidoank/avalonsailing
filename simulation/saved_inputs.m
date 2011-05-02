% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function inputs = saved_inputs(new_inputs)
% function inputs = saved_inputs(new_inputs)
% set and get the inputs vector, which is persistently stored here.
% (persistent is similar to static in C.)
persistent inputs_vector = [];
if nargin == 1
  inputs_vector = new_inputs;
  inputs = inputs_vector;
else
  inputs = inputs_vector;
endif
  
S = simulation_options();
assert(size(inputs) == [S.no_of_inputs, 1]);
