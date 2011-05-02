% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function plot_force_arrow_2d(F, pos, scale_F) 
% function plot_force_arrow_2d(F, pos, scale_F) 
% Plot a force arrow in the current figure
% F : force vector (only first 2 dimensions are shown)
% pos: position where force acts, (only first 2 dimensions are shown)
% scale_F (default: 1/1000) is used as a factor of F to transform forces into dimensions.
if nargin == 2
  scale_F = 1/1000;
endif
scaled_force = (scale_F * F)(1:2);

tip_angle = deg2rad(15);
tip_relative_length = 0.15;
% unit = [scaled_force(1); scaled_force(2)] / (sqrt(sumsq(scaled_force)));  % for constant size tip
unit = [scaled_force(1); scaled_force(2)];  % for constant relative size tip

tau = pi - tip_angle;

left  = tip_relative_length * [cos(tau)  sin(tau);  -sin(tau)  cos(tau)] * unit;
right = tip_relative_length * [cos(-tau) sin(-tau); -sin(-tau) cos(-tau)] * unit;

end_x = pos(1)+scaled_force(1);
end_y = pos(2)+scaled_force(2);

x = [pos(1) end_x end_x+left(1) end_x end_x+right(1)];
y = [pos(2) end_y end_y+left(2) end_y end_y+right(2)];
plot(y, x); % swapped coordinates

endfunction