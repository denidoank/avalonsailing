% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function force_hull_test(stream_H)
% test a value from [endreport, p. 65] 
assert(equal([48.41246; 0; 0], force_hull([knots2m_per_second(4); 0; 0])));
endfunction