% Copyright 2012 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, Nov. 2012

function plot_scatter()
  scatter_data  % script written from C++ target_circle_cascade_test.
  % x,y,u,v,c with high resolution.
  % index into color map
  cm = colormap;
  scaling = size(cm);
  rrr = scaling(1);
  colour = scaling(1) / max(c) * c;
  h = scatter(x, y, 1, colour, "*");
  xlabel("lon / deg");
  ylabel("lat / deg");
  axis ([5.8, 5.95, 42.85, 43.05], "equal");

endfunction
