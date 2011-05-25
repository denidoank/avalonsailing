% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function result = pack_state_script_test()
  state = [1:15]';  % make a column vector
  original_state = state;
  unpack_state_script;
  assert(z == 3);
  assert(gamma_R2 == 15);
  pack_state_script;
  assert_eq(original_state, state);
  z = 300;
  pack_state_script;
  assert(300 == state(3));
  result = 1;
endfunction
