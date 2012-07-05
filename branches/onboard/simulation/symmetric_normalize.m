% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function normalized = symmetric_normalize(alpha)
% function normalized = symmetric_normalize(alpha)
% force angle into -pi ... pi

assert(abs(alpha) < 10, ["alpha in radians expected! ", num2str(alpha)]) 
normalized = mod(alpha, 2 * pi);
if normalized > pi
  normalized = normalized - 2 * pi;
endif
assert(abs(normalized <= pi));
endfunction