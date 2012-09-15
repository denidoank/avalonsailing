% Copyright 2012 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, September 2012

function generate_target_circles_toulon_c
% function generate_target_circles_toulon_c.m
% For a trip from Toulon to the sea and back.
% Script to make const table for the route of the boat,
% in C++.
% Produces a file named "target_circle_points_table_toulon_c.h" in the current directory.
% plan B is for wind from W
%
%          toulon
%
%   start  /     \ end
%         /  / \  \
%        /  /   \  \
%       /  /     \  \
%      /  /       \  \
%     /  /         \--
%    |  /
%     --
% 2 * 50 miles + 2 * 40 miles = 180miles (effective 211 because of the bends)
%
% 10h weather prognosis time, speed of 6kn of the chase boat, 60miles off the shore is safe.
% 3kn Avalon speed, 2 days 3kts*48h=144miles, 3 days 200 nautical miles
%



[fid, msg] = fopen("target_circle_points_table_toulon_c.h", "w");
if fid == -1
  msg
  error(msg);
  return;
endif

% Big target circles of 2000m radius
meter2degree = 1.0 / 111111.1;
radius_m = 2000;
radius_deg = radius_m * meter2degree;
r = 2.5*radius_deg;

% 50 miles leg 1
leg_1 = 50 * 1852;
turned = 30 / 180.0 * pi;
turned_bends = turned - pi/4;
delta_lat1 = leg_1 * sin(turned) * meter2degree;
delta_lon1 = leg_1 * cos(turned) * meter2degree * 1 / cos(42.0 / 180 * pi);
delta_leg_1 = [-delta_lat1 -delta_lon1 0];

% 40 miles leg 2
leg_2 = 40 * 1852;
% rotation by using something else than [0.707, 0.707]
delta_lat2 = leg_2 * sin(turned) * meter2degree;
delta_lon2 = leg_2 * cos(turned) * meter2degree * 1 / cos(42.0 / 180 * pi);
delta_leg_2 = [-delta_lat2 -delta_lon2 0];


% Toulon x points
incr = 0.2/sqrt(2);
toulon1 = [42.9414 005.758 r]
toulon = []
for k=0:3
  toulon = [toulon; toulon1 + [-k*incr k*incr 0] ]
endfor


% round ends
center = 0.5 * (toulon(2, :)+delta_leg_1 + toulon(1, :)+delta_leg_1 );
radius = incr / sqrt(2);
bend1 = [];
for step = (-pi/4-pi/8):-pi/8:(-5*pi/4+pi/8)
  bend1 = [bend1; center + [radius*sin(step) radius*cos(step) 0] ]
endfor

center = 0.5 * (toulon(4, :)+delta_leg_2 + toulon(3, :)+delta_leg_2 );
radius = incr / sqrt(2);
bend2 = [];
for step = (-pi/4-pi/8):-pi/8:(-5*pi/4+pi/8)
  bend2 = [bend2; center + [radius*sin(step) radius*cos(step) 0] ]
endfor


% The Toulon Plan C
% lat lon radius / all in degree
% Reverse order, start comes last
points = [ ...
        % Toulon4, use this one for the simple plan to abort the last leg!
        toulon(4, :)
        toulon(4, :)+delta_leg_2
        bend2
        toulon(3, :)+delta_leg_2
        % Toulon3
        toulon(3, :)

        % Toulon2 middle
        toulon(2, :)
        toulon(2, :)+delta_leg_1
        bend1
        toulon(1, :)+delta_leg_1
        % Toulon1
        toulon(1, :)
        % Toulon0
        43.0000 005.9000    r
        ];

scope = [min(points(:, 2)), max(points(:, 2)), min(points(:, 1)), max(points(:, 1))];


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
axis (scope, "equal");

pause

save -ascii mallorca_points.asc points_out

figure
plot_plan(points_out)
hold on

axis (scope, "equal");

hold off


figure
plot_plan(points_out)
hold on
axis ([5.5, 6.2, 42.5, 43.5], "equal");
hold off

figure
plot_plan(points_out)
hold on
axis ([3.2, 3.5, 39.8, 40], "equal");
hold off





x=points_out(:,1);
y=points_out(:,2);
r=points_out(:,3);

fprintf(fid, "const TargetCirclePoint[] = {\n");

for i=1:length(r)
  fprintf(fid, "TCP(%8g, %8g, %8g),\n", x(i), y(i), r(i))
endfor
fprintf(fid, "TCP(       0,         0,         0)};  // end marker\n")

fclose(fid);
endfunction
