% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

function plot_plan(plan)
  % plan is an x by 3 matrix where the columns are
  %  latitude of target circle, in degrees
  %  longitude of target circle, in degrees
  %  radius of target circle, in degrees
  figure
  hold on
  plot_plan_internal(plan);
  axis ("equal");
  pause
  hold off
endfunction
