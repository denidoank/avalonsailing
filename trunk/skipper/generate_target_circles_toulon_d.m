% Copyright 2012 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, September 2012

function generate_target_circles_toulon_d
% function generate_target_circles_toulon_d.m
% For a trip from Toulon to the sea and back.
% Script to make const table for the route of the boat,
% in C++.
% Produces a file named "target_circle_points_table_toulon_d.h" in the current directory.
% plan A is for wind from NW
%
%          toulon
%
%   start  /
%         /  / ---------------------]
%        /  /     ------------------|
%       /  /     end
%      /  /
%     /  /
%    |  /
%     --
% 2 * 50 miles + 2 * 40 miles = 180miles
%
% 10h weather prognosis time, speed of 6kn of the chase boat, 60miles off the shore is safe.
% 3kn Avalon speed, 2 days 3kts*48h=144miles
%



[fid, msg] = fopen("target_circle_points_table_toulon_d.h", "w");
if fid == -1
  msg
  error(msg);
  return;
endif

% Big target circles of 2000m radius
meter2degree = 1.0 / 111111.1;
radius_m = 2000;
radius_deg = radius_m * meter2degree;
r = 1.5*radius_deg;

% 3 miles leg 1
leg_1 = 3 * 1852;
delta_lat1 = leg_1 * 1 * meter2degree;
delta_lon1 = leg_1 * 0 * meter2degree * 1 / cos(42.0 / 180 * pi);
delta_leg_1 = [-delta_lat1 -delta_lon1 0];

% x miles leg 2
leg_2 = 50 * 1852;
% rotation by using something else than [0.707, 0.707]
alpha3=35/180.0*pi; % more or less east - 10 degrees, bearing 70.
delta_lat2 = leg_2 * sin(alpha3) * meter2degree;
delta_lon2 = leg_2 * cos(alpha3) * meter2degree * 1 / cos(42.0 / 180 * pi);
delta_leg_2 = [delta_lat2 delta_lon2 0];


% Toulon x points
incr = 0.2/sqrt(2);
toulon1 = [42.9414 005.758 r]
toulon = []
for k=0:3
  toulon = [toulon; toulon1 + [-k*incr k*incr 0] ];
  plat(toulon(k+1, 1))
  plat(toulon(k+1, 2))
endfor

toulon
disp ("Note these!");
pause
% round ends
center = 0.5 * (toulon(2, :)+delta_leg_1 + toulon(1, :)+delta_leg_1 );
radius = incr / sqrt(2);
bend1 = [];
for step= (-1*pi/4-pi/8):-pi/8:(-3*pi/4+pi/8)
  bend1 = [bend1; center + [radius*sin(step) radius*cos(step) 0] ]
endfor



% The Toulon Plan A
% lat lon radius / all in degree
% Reverse order, start comes last
points = [ ...
        43.05314 5.95  r   % 162 nm
        42.94832 6.19814  r
        42.97494 6.47943  r
        43.33759 7.25226  r   % now turn back
        43.23611 7.3599   r
        43.14793 7.38631  r
        42.87    6.47638  r
        42.84    6.19615  r   % S of Ile de Pocqurolles
        42.9900  5.96572  r
        42.9233  5.95572   r
        42.9233  5.8601    r
        % les sabbelette, Tue 11:00
        43.0400 005.8664  r
        ]
for ppp=1:6
  plat(points(ppp, 1));
  plat(points(ppp, 2));
endfor

pause
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
