% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

function line = blocked_segments(center_point, radius, blocked_start, blocked_end)
  line = center_point;
  for i = 1:length(blocked_start)
    line = [line seg(center_point, radius, blocked_start(i), blocked_end(i)) center_point];
  endfor
endfunction 

function s = seg(center_point, radius, start, ende)
  segments = 60;
  phi = [start:(2 * pi / segments):ende ende];
  s = center_point * ones(1, length(phi)) + radius * [cos(phi); sin(phi)];
endfunction