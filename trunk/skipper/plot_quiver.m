% Copyright 2012 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, Nov. 2012

function plot_quiver()
  quiver_data  % script written from C++ target_circle_cascade_test.
  % x,y,u,v,c

  h = quiver (x, y, u, v, 0.04);
  set (h, "maxheadsize", 0.33);
  xlabel("lon / deg");
  ylabel("lat / deg");
  axis ([5.8, 5.95, 42.85, 43.05], "equal");

endfunction
