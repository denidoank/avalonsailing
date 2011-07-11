% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

function collision_avoidance_test()
% N mobile objects B_i, initially at positions  x0_i, where
% x0_b = [ x_i
%          y_i ]
% can possibly collide with us. Given the speed vector of the B object v_b, where
% v_b = [ v_x_b
%         v_y_b ],
% and the magnitude of the speed the first object A what a direction must A steer to
% avoid all B ?

x0 = [ ...
  1000    2000     5000   -2500  -5000 ;
  -3000   -4000    2500   10000  -8000]
v = [ ...
  -4      -5       -2      3      5  ;
  1       1.5      -2     -3      2  ];  

mag_v_a = 3;
t_scope = 5000;
minimum_distance = 300;


% for modified speeds

[alpha, phi_c, t_c, blocked_start, blocked_end] = collision_avoidance(0.8 * mag_v_a, x0, v, t_scope, minimum_distance);
collision_avoidance_sim(mag_v_a, x0, v, t_scope, minimum_distance, phi_c, t_c, blocked_start, blocked_end);

[alpha, phi_c, t_c, blocked_start, blocked_end] = collision_avoidance(0.9 * mag_v_a, x0, v, t_scope, minimum_distance);
collision_avoidance_sim(mag_v_a, x0, v, t_scope, minimum_distance, phi_c, t_c, blocked_start, blocked_end);

[alpha, phi_c, t_c, blocked_start, blocked_end] = collision_avoidance(1.0 * mag_v_a, x0, v, t_scope, minimum_distance);
collision_avoidance_sim(mag_v_a, x0, v, t_scope, minimum_distance, phi_c, t_c, blocked_start, blocked_end);

[alpha, phi_c, t_c, blocked_start, blocked_end] = collision_avoidance(1.1 * mag_v_a, x0, v, t_scope, minimum_distance);
collision_avoidance_sim(mag_v_a, x0, v, t_scope, minimum_distance, phi_c, t_c, blocked_start, blocked_end);

[alpha, phi_c, t_c, blocked_start, blocked_end] = collision_avoidance(1.2 * mag_v_a, x0, v, t_scope, minimum_distance);
collision_avoidance_sim(mag_v_a, x0, v, t_scope, minimum_distance, phi_c, t_c, blocked_start, blocked_end);


for trials=1:20
  N = 50;
  x0 =  1000 * 2 * (rand(2, N) - 0.5);
  v =   20   * 2 * (rand(2, N) - 0.5);

  mag_v_a = 3;
  t_scope = 400;
  minimum_distance = 80;

  [alpha, phi_c, t_c, blocked_start, blocked_end] = collision_avoidance(mag_v_a, x0, v, t_scope, minimum_distance);

  collision_avoidance_sim(mag_v_a, x0, v, t_scope, minimum_distance, phi_c, t_c, blocked_start, blocked_end);
endfor

endfunction