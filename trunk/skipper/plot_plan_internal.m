% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

function plot_plan_internal(plan)
  % plan is an x by 3 matrix where the columns are
  %  latitude of target circle, in degrees
  %  longitude of target circle, in degrees
  %  radius of target circle, in degrees
  for i=1:rows(plan)
    c = circle([plan(i, 1); plan(i, 2)], plan(i, 3));
    plot(c(2, :), c(1, :), "b");
  endfor
endfunction
