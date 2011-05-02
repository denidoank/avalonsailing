% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011
 
function assert_eq(expected, actual, eps)
% function assert_eq(expected, actual, eps)
% Matrix equality check for tests.
% expected: expected value
% actual: actual value
% eps: optional admissible deviation of the norm of (expected-actual)
% asserts if both matrices are not equal.

if nargin == 2
  eps = 1E-6 * max([norm(expected), norm(actual), 1]);
endif
eq = abs(norm(expected - actual)) < eps;
if ~eq
  disp("Expected value:")
  expected
  disp("differs from actual value:")
  actual
  disp("delta:");
  delta = expected - actual
  assert(eq);
endif

endfunction