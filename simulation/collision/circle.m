% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

function circle_xy = circle(center, radius)
  segments = 40;
  phi = (2 * pi / segments) * (0:segments);
  circle_xy = center * ones(1, segments + 1) + radius * [cos(phi); sin(phi)];
endfunction 