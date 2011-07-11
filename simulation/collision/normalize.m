% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

% force into [0 2*pi)
function normal = normalize(x)
  normal = mod(x, 2 * pi);
endfunction 