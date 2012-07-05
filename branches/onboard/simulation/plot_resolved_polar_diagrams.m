% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function plot_resolved_polar_diagrams(part)
% function plot_resoved_polar_diagrams(part)
% part = "keel", "rudder" or "sail"
% Plot resolved 2D polar diagrams of the profiles for rudder, keel and sail (c_lift over c_drag).
if nargin == 0
  error("Give a part as single argument: keel, rudder or sail (in single or double quotes as an octave string)");
endif

close all
close all
pause

alpha = -5:1:20;
c_lift = zeros(size(alpha));
c_drag = zeros(size(alpha));
c_arm  = zeros(size(alpha));

i=1;
speed1 = 2; % m/s
for a = alpha
  [c_lift(i), c_drag(i), c_arm(i)] = c_aero2d_of(part, speed1, deg2rad(a));
  i += 1;   
endfor

figure
plot(c_drag, c_lift,"b");
xlabel("c_drag");
ylabel("c_lift");
title(["c_{lift}=f(c_{drag} for ", part, ", at 2m/s"]);

