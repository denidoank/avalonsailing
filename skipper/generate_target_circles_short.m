% Copyright 2012 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, September 2012

% Run the C++ test
% make target_circle_cascade_test.run
% first!
% It produces quiver_data.m and scatter_data.m

function generate_target_circles_short
% function generate_target_circles_short.m
% For the explanation of target circles.
% Script to make const table for the route of the boat,
% in C++.
% Produces a file named "target_circle_points_table_short.h" in the current directory.
% 2 * 50 miles + 2 * 40 miles = 180miles
%
% TODO: Distorsion of the longitude values.


[fid, msg] = fopen("target_circle_points_table_short.h", "w");
if fid == -1
  msg
  error(msg);
  return;
endif

% Big target circles of 2000m radius
meter2degree = 1.0 / 111111.1;
radius_m = 3000;  % in m
radius_deg = radius_m * meter2degree;
r = radius_deg;

% lat lon radius / all in degree
% Reverse order, start comes last
points = [ ...
        42.9000  5.9100    0.8*r
        42.9000  5.8600    0.8*r
        % Les sabelette, Tue 11:00
        43.0000 005.8600   r
        ]

r_margin = max(points(:, 3));
scope = [min(points(:, 2)) - r_margin, max(points(:, 2)) + r_margin,  ...
         min(points(:, 1)) - r_margin, max(points(:, 1)) + r_margin];

plot_plan(points);
axis (scope, "equal");

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

% plot_plan_map(points_out);
plot_plan(points_out);
axis (scope, "equal");

pause

save -ascii short_points.asc points_out

figure
plot_plan(points_out)
hold on
axis (scope, "equal");
hold off


figure
plot_plan(points_out)
hold on
axis ([5.8, 5.95, 42.85, 43.05], "equal");
hold off

figure
plot_quiver
axis ([5.8, 5.95, 42.85, 43.05], "equal");
hold on
plot_plan_internal(points_out)
hold off
pause

figure
plot_scatter
hold on
plot_plan_internal(points_out);
axis (scope, "equal");
pause
hold off

x=points_out(:,1);
y=points_out(:,2);
r=points_out(:,3);

fprintf(fid, "const TargetCirclePoint short_lesson_plan[] = {\n");
l_target = length(r);

for i=1:length(r)
  fprintf(fid, "TCP(%8g, %8g, %8g),  // %d\n", x(i), y(i), r(i), l_target - i + 1)
endfor
fprintf(fid, "TCP(       0,         0,         0)};  // end marker\n")

fclose(fid);
endfunction


function plat(lat)
  b = floor(lat);
  minutes = (lat-b)*60;
  disp([num2str(b), "deg ", num2str(minutes), "min"]);
endfunction
