% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function result = force_of_test() 

B = boat();
% Keel tests
aoa = 0;
gamma = 0;
speed = 0;
[F, pos] = force_of("keel", aoa, speed, gamma);
% The following is always true:
assert(F(3) == 0);
assert_eq([0; 0; 0], F);
assert_eq([B.K], pos, 0.5);  % center of force must be on a line through B.K

aoa = deg2rad(-5);
gamma = 0;
speed = 0;
[F, pos] = force_of("keel", aoa, speed, gamma);
% The following is always true:
assert(F(3) == 0);
assert_eq([0; 0; 0], F);
assert_eq([B.K], pos, 0.5);  % center of force must be on a line through B.K

aoa = deg2rad(5);
gamma = 0;
speed = 1;
[F, pos] = force_of("keel", aoa, speed, gamma);
% The following is always true:
assert(F(3) == 0);
assert_eq([-7; 94; 0], F, 2);
assert_eq(B.K, pos, 0.5);

% Rudder
aoa = 0;
gamma = 0;
speed = 0;
[F, pos] = force_of("rudder", aoa, speed, gamma);
assert_eq([0; 0; 0], F);
assert_eq(B.R, pos, 0.3);

aoa = deg2rad(5);
gamma = deg2rad(5);
speed = 10;
[F, pos] = force_of("rudder", aoa, speed, gamma);
assert_eq([-171; 1923; 0], F, 50);
assert_eq(B.R, pos, 0.3);

gamma = deg2rad(-90); % sail turned 90 degrees to the left, most of it is on starboard side
aoa = deg2rad(90);
speed = 10;
[F, pos] = force_of("sail", aoa, speed, gamma);
assert_eq([910; -24; 0], F, 10);
assert_eq(B.S + [0; 0.3; 0], pos, 0.4);

"gamma sail 0-360 degree, at constant angle of attack: expect a circle"
aoa = deg2rad(15);
speed = 10;
all_gammas = 0:pi/15:(2*pi);
fx = zeros(size(all_gammas));
fy = zeros(size(all_gammas));
k = 1;
for gamma = all_gammas
  [F, pos] = force_of("sail", aoa, speed, gamma);
  assert_eq(360.14, sqrt(sumsq(F)), 10);
  fx(k) = F(1);
  fy(k) = F(2);
  k += 1;
endfor
%figure
%plot(fy, fx);
%axis("equal");
%figure(gcf);
%pause 

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
  fx(k) = F(1);
  fy(k) = F(2);
  k += 1;
endfor
% figure
% plot with view from above so the axis x and y are swapped.
% plot(fy, fx);
% title("F_x=f(F_y)");
% xlabel("F_y");
% ylabel("F_x");
% axis("equal");
% figure(gcf);

result = 1;
endfunction
