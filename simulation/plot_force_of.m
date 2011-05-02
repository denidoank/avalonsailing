% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function plot_force_of() 
close all
clear all



figure
title("gamma sail 0-90 degree, at constant angle of attack of 0: expect a circle");
axis("equal");
aoa = deg2rad(0);
speed = 10;
all_gammas = 0:pi/15:(pi / 2);
fx = zeros(size(all_gammas));
fy = zeros(size(all_gammas));
k = 1;
for gamma = all_gammas
  [F, pos] = force_of("sail", aoa, speed, gamma);
  plot_force_arrow_2d(F, pos);
  hold on;
  pause(0.5);
  axis("equal");
  fx(k) = F(1);
  fy(k) = F(2);
  k += 1;
endfor
% looks ok

"Done, switch to Graphics window!"
hold off
pause



figure
hold on

title("gamma sail 0-90 degree, at constant angle of attack: expect a circle");
axis("equal");
aoa = deg2rad(10);
speed = 10;
all_gammas = 0:pi/15:(pi / 2);
fx = zeros(size(all_gammas));
fy = zeros(size(all_gammas));
k = 1;
for gamma = all_gammas
  [F, pos] = force_of("sail", aoa, speed, gamma);
  plot_force_arrow_2d(F, pos);
  pause(0.5);
  fx(k) = F(1);
  fy(k) = F(2);
  k += 1;
endfor

"Done, switch to Graphics window!"
hold off

pause
figure
plot(fy, fx);
axis("equal");
figure(gcf);
"Done, switch to Graphics window!"
pause 

all_alphas = 0:2:25;
fx = zeros(size(all_alphas));
fy = zeros(size(all_alphas));
k = 1;
gamma = deg2rad(-80); 
speed = 10; 
"alpha_sail 0-25 degree"
for alpha = all_alphas
  aoa = deg2rad(alpha);
  [F, pos] = force_of("sail", aoa, speed, gamma);
  plot_force_arrow_2d(F, pos);
  fx(k) = F(1);
  fy(k) = F(2);
  k += 1;
endfor
"Done, switch to Graphics window!"
pause

figure
% plot with view from above so the axis x and y are swapped.
plot(fy, fx);
xlabel("F_y");
ylabel("F_x");
axis("equal");
figure(gcf);
"Done, switch to Graphics window!"

endfunction