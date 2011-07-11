% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

function collision_test()
% 2 mobile objects A and B, initially at positions x0_a and x0_b, where
% x0_a = [ x_a
%          y_a ]
% x0_b = [ x_b
%          y_b ]
% can possibly collide. Given the speed vector of the B object v_b, where
% v_b = [ v_x_b
%         v_y_b ],
% and the magnitude of the first object A what a direction must A steer to
% hit B and when will the collision happen?


% rotated standard cases
rotation = [ 0 -1;
             1  0];

x0_a = [ 0;  0 ]
mag_v_a =  5
x0_b = [   0;  1000 ]
v_b = [  3.5355; -3.5355]
L =  1000
[phi, time, alpha] = collision(x0_a, mag_v_a, x0_b, v_b)
res = collision_sim(x0_a, mag_v_a, x0_b, v_b, phi, time)
assert_eq(1, res);
assert_eq(-0.78539, alpha, 0.001);
assert_eq(141.42, time, 0.002);
assert_eq(0.78539, phi, 0.001);

x0_a = [ 0;  0 ]
mag_v_a =  5
x0_b = rotation * [1000; 0]
v_b = rotation * [ -10; -3.5355]
[phi, time, alpha] = collision(x0_a, mag_v_a, x0_b, v_b)
res = collision_sim(x0_a, mag_v_a, x0_b, v_b, phi, time)
assert_eq(1, res);
assert_eq([-0.78539   3.92698], alpha, 0.001);
assert_eq([73.879   154.693], time, 0.002);
assert_eq([0.78539 5.49778], phi, 0.001);

% Rotate this case by arbitrary angles
x0_a = [ 0;  0 ]
mag_v_a =  5
for r = 0:(pi/23+0.08741):(2*pi)
  rotation = [ cos(r) -sin(r);
               sin(r)  cos(r)];
  x0_b = rotation * [1000; 0]
  v_b = rotation * [ -10; -3.5355]
  [phi, time, alpha] = collision(x0_a, mag_v_a, x0_b, v_b)
  res = collision_sim(x0_a, mag_v_a, x0_b, v_b, phi, time, 0);
  assert_eq(1, res);
  assert_eq([-0.78539   3.92698], alpha, 0.001);
  assert_eq([73.879   154.693], time, 0.002);
  assert_sorted_eq(normalize(r + [5.4978 3.92698]), phi, 0.001);
endfor


% Standard cases
x0_a = [ 0;  0 ]
mag_v_a =  5
x0_b = [   1000;  0 ]
v_b = [  4.5; 0]
L =  1000
[phi, time, alpha] = collision(x0_a, mag_v_a, x0_b, v_b)
assert_eq(0.0, alpha);
assert_eq(2000, time);
res = collision_sim(x0_a, mag_v_a, x0_b, v_b, phi, time)
assert_eq(1, res);

x0_a = [ 0;  0 ]
mag_v_a =  5
x0_b = [   1000;  0 ]
v_b = [  -3.5355; 3.5355]
L =  1000
[phi, time, alpha] = collision(x0_a, mag_v_a, x0_b, v_b)
assert_eq(0.78539, alpha, 0.001);
assert_eq(141.42, time, 0.002);
res = collision_sim(x0_a, mag_v_a, x0_b, v_b, phi, time)
assert_eq(1, res);

x0_a = [ 0;  0 ]
mag_v_a =  5
x0_b = [   1000;  0 ]
v_b = [  -5; 0]
L =  1000
[phi, time, alpha] = collision(x0_a, mag_v_a, x0_b, v_b)
assert_eq(0.0, alpha);
assert_eq(100, time);
res = collision_sim(x0_a, mag_v_a, x0_b, v_b, phi, time)
assert_eq(1, res);

x0_a = [ 0;  0 ]
mag_v_a =  5
x0_b = [   1000;  0 ]
v_b = [  5; 0]
L =  1000
[phi, time, alpha] = collision(x0_a, mag_v_a, x0_b, v_b)
assert_eq([], alpha);
assert_eq([], time);
res = collision_sim(x0_a, mag_v_a, x0_b, v_b, phi, time)
assert_eq(1, res);

x0_a = [ 0;  0 ]
mag_v_a =  5
x0_b = [   1000;  0 ]
v_b = [  -3.5355; -3.5355]
L =  1000
[phi, time, alpha] = collision(x0_a, mag_v_a, x0_b, v_b)
assert_eq(-0.78539, alpha, 0.001);
assert_eq(141.42, time, 0.002);
res = collision_sim(x0_a, mag_v_a, x0_b, v_b, phi, time)
assert_eq(1, res);

x0_a = [ 0;  0 ]
mag_v_a =  5
x0_b = [   1000;  0 ]
v_b = [  -10; -3.5355]
L =  1000
[phi, time, alpha] = collision(x0_a, mag_v_a, x0_b, v_b)
assert_eq([-0.78539   3.92698], alpha, 0.001);
assert_eq([73.879   154.693], time, 0.002);
res = collision_sim(x0_a, mag_v_a, x0_b, v_b, phi, time)
assert_sorted_eq([5.4978 3.92698], phi, 0.001);
assert_eq(1, res);

endfunction