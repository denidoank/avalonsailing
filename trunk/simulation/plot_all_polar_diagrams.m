% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function plot_all_polar_diagrams(part)
% function plot_all_polar_diagrams(part)
% part = "keel", "rudder" or "sail"
% Plot all 2D polar diagrams of the profiles for rudder, keel and sail.
if nargin == 0
  error("Give a part as single argument: keel, rudder or sail (in single or double quotes as an octave string)");
endif

close all
close all
pause

alpha = -180:1:180;
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
plot(alpha, c_lift,"b");
xlabel("angle of attack/degree");
ylabel("c_l");
title(["c_{lift} for ", part, ", at 2m/s"]);

figure
plot(alpha, c_drag,"k");
xlabel("angle of attack/degree");
ylabel("c_d");
title(["drag, for ", part, ", at 2m/s"]);
  
figure
plot(alpha, c_arm,"g");
xlabel("angle of attack/degree");
ylabel("c_{arm}");
title(["arm, for ", part, ", at 2m/s"]);
pause

"Speed dependant curves"
speed = -10:0.5:10;
c_lift = zeros(size(speed));
c_drag = zeros(size(speed));
c_arm  = zeros(size(speed));

i=1;
aoa = 8;  % here we can see the effect of different reynolds numbers
for v = speed
  [c_lift(i), c_drag(i), c_arm(i)] = c_aero2d_of(part, v, deg2rad(aoa));
  i += 1;
endfor

figure
  plot(speed, c_lift,"b");
  xlabel("speed / m/s");
  ylabel("c_l");
  title(["c_{lift} = f(v) for ", part, ", at 8 degree AOA"]);
  figure(gcf);
figure
  plot(speed, c_drag,"k");
  xlabel("speed / m/s");
  ylabel("c_d");
  title(["drag = f(v), for ", part, ", at 8 degree AOA"]);
  figure(gcf);
figure
  plot(speed, c_arm,"g");
  xlabel("speed / m/s");
  ylabel("c_{arm}");
  title(["arm = f(v), for ", part, ", at 8 degree AOA"]);
  figure(gcf);


% 3D graphics
a = -180:10:180;
speed = -10:2:10;
empty_array = zeros(length(a),length(speed));
c_lift = empty_array;
c_drag = empty_array;
c_arm  = empty_array;
speed_array = empty_array;
angle_array = empty_array;

for n = 1:length(a)
for k = 1:length(speed)
  angle_array(n, k) = a(n);
  speed_array(n, k) = speed(k);  
  [c_lift(n, k), c_drag(n, k), c_arm(n, k)] = c_aero_of(part, speed(k), deg2rad(a(n)));
endfor
endfor

pause

figure
plot3(angle_array, speed_array, c_lift);
title(["c_{lift} for ", part]);
xlabel("angle / degree");
ylabel("speed / m/s");
zlabel("c_l");
figure(gcf);

figure
plot3(angle_array, speed_array, c_drag);
title(["c_{drag} for ", part]);
xlabel("angle / degree");
ylabel("speed / m/s");
zlabel("c_d");
figure(gcf);

figure
plot3(angle_array, speed_array, c_arm);
title(["arm for ", part]);
xlabel("angle / degree");
ylabel("speed / m/s");
zlabel("c_{arm}");
figure(gcf);
