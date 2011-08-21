% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, July 2011

function generate_target_circles
% function generate_target_circles.m
% Script to make const table for the route of the boat,
%  in C++.
% Produces a file named "target_circle_points_table.h" in the current directory.
alpha = zeros(361,1);
c_lift = alpha;
c_drag = alpha;
c_arm  = alpha;

[fid, msg] = fopen("target_circle_points_table.h", "w");
if fid == -1
  msg
  error(msg);
  return;
endif

% The Caribbean Plan
points = [ ...
16.877  -61.7721 0.09  % 15 km south of Antigua
16.877  -61.2887 0.1
17   -60   0.5         % Cross the target line
18   -58   1
22   -45   2
23.5 -25   3        % Northern <Wendekreis>, tropic of cancer
30   -20   2
33   -20   2        % west of Madeira
45   -13   1        % NW of La Coruna
47   -9    0.5      % SSW of Brittany buoy, target for crossing the traffic line, SW heading is good at NW wind
48.2 -5    0.25 ];  % W of Brest

plot_plan(points);


x=points(:,1);
y=points(:,2);
r=points(:,3);

overlap=1.3

points_out = points(1,:);
for i=2:length(r)
  phi = atan2(y(i) - y(i-1), x(i) - x(i-1))
  leg = sqrt(sumsq([y(i)-y(i-1) x(i)-x(i-1)]))
  avg_dist = (r(i) + r(i-1)) / 2 / overlap
  N = ceil(leg/avg_dist) + 1
  first_dist = r(i-1) / overlap
  last_dist  = r(i) / overlap
  dist = first_dist + [1:(N)]' * (last_dist-first_dist)/N
  r_new = dist * overlap
  c_dist = cumsum(dist)
  l_index=length(c_dist)
  dist_actual = c_dist(l_index)
  c_dist *= leg / dist_actual
  x_new = x(i-1) + c_dist * cos(phi)
  y_new = y(i-1) + c_dist * sin(phi)
  points_out
  [x_new y_new r_new]
  points_out = [points_out; [x_new y_new r_new]]
  % pause
endfor

plot_plan(points_out)

figure
plot_plan(points_out)
axis ([-62, -58, 16, 19], "equal");



x=points_out(:,1);
y=points_out(:,2);
r=points_out(:,3);

fprintf(fid, "const TargetCirclePoint[] = {\n");

for i=1:length(r)
  fprintf(fid, "TCP(%8g, %8g, %8g),\n", x(i), y(i), r(i))
endfor
fprintf(fid, "TCP(       0,         0,         0);  // end marker\n")

fclose(fid);
endfunction
