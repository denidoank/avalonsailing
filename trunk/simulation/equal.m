% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011
 
function eq = equal(a, b, eps)
% function eq = equal(a, b, eps)
% Matrix equality check for tests.
% a: expected value
% b: actual value
% eps: optional admissible deviation of the norm of (a-b)

if nargin == 2
  eps = 1E-6 * max([norm(a), norm(b), 1]);
endif
eq = abs(norm(a - b)) < eps;
if ~eq
  left = a
  right = b
  left_minus_right = a - b
endif

endfunction
