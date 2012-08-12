% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, July 2011

function generate_target_circles_ufenau
% function generate_target_circles_ufenau.m
% Script to make const table for the route of the boat,
%  in C++.
% Produces a file named "target_circle_points_table_ufenau.h" in the current directory.

[fid, msg] = fopen("target_circle_points_table_ufenau.h", "w");
if fid == -1
  msg
  error(msg);
  return;
endif

% Small target circles of 200m radius
meter2degree = 1.0 / 111111.1;
radius_m=200;
radius_deg = radius_m * meter2degree;

% The Ufenau Plan
% lat lon radius / all in degree
points = [ ...
        % Wollerau
        47.2223, 8.7194, radius_deg
        47.2372, 8.6917, radius_deg
        47.2569, 8.6519, radius_deg
        % Meilen ferry!
        47.2605, 8.6212, radius_deg
        47.2676, 8.6179, radius_deg
        % Meilen ferry !
        47.2718, 8.6021, radius_deg
        % Rueschlikon
        47.3085, 8.5705, radius_deg
        47.3374, 8.5541, radius_deg
        % artificial zigzag
        47.3423, 8.5539, radius_deg
        47.3433, 8.5455, radius_deg
        %start point
        47.3482, 8.5434, radius_deg ];  % where we always test off Wollishofen

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

% plot_plan_map(points_out);
plot_plan(points_out);
axis ([8.5, 8.75, 47.22, 47.36], "equal");

pause

save -ascii ufenau_points.asc points_out

figure
plot_plan(points_out)
hold on
a = antigua_map(); % Sorry, have no lake picture.
patch(a(:,2), a(:,1), [0.1 0.8 0.2]);
axis ([8.5, 8.75, 47.22, 47.36], "equal");

hold off

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








