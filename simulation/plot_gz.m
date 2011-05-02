% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.

function plot_gz()
% Plot both gz_x and gz_y functions. gz_x is the righting force lever
% around the x-axis, counteraction the heel angle.
% gz_y is the lever opposing the yaw angle.

close all
a = -120:1:120;
gzx = gz_x(deg2rad(a));
figure
plot(a, gzx,"b*");
xlabel("angle of heel/degree");
ylabel("gz / m");
title(["GZ = f(phi_x) "]);

gzy = gz_y(deg2rad(a));
figure
plot(a, gzy,"b*");
xlabel("phi_y/degree");
ylabel("gz / m");
title(["GZ_y = f(phi_y) "]);

% unique solution range + 10% to see the inverse function
a = -85:1:85;
gzx = gz_x(deg2rad(a));
a2 = rad2deg(inv_gz_x(gzx));

figure;
plot(gzx, a, "b*", gzx, a2, "g+");
ylabel("angle of heel/degree");
xlabel("gz / m");
title(["phi_x = f(GZ) Check of inverse function"]);



