% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function tangential_speed_test()
assert(equal([0;  10; 0], tangential_speed([0; 0; -10], [1; 0; 0])));
assert(equal([-10; 0; 0], tangential_speed([0; 0; -10], [0; 1; 0])));
assert(equal([  0; 0; 0], tangential_speed([0; 0; -10], [0; 0; 1])));
endfunction