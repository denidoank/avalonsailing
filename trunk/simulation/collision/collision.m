% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

function [phi_a, t, alpha] = collision(x0_a, mag_v_a, x0_b, v_b) 
% function [alpha_collision, t] = collision(x0_a, mag_v_a, x0_b, v_b) 
% 2 mobile objects A and B, initially at positions x0_a and x0_b, where
% x0_a = [ x_a
%          y_a ]
% x0_b = [ x_b
%          y_b ]
% can possibly collide. Given the speed vector of the B object v_b, where
% v_b = [ v_x_b
%         v_y_b ],
% and the magnitude of the first object A what a direction must A steer to
% hit B and when in the future will the collision happen, if at all?
% x0_a and x0_b are the initial position of the objects (e.g. ships)
% mag_v_a is the magnitude of A's speed
% v_b is B's  speed vector
% Results are the direction phi_a A has to steer to collide with B and
% t, the future time when this will happen.
% alpha is the angle in of the course relative to the connecting line AB.
% phi_b, t and alpha are vectors with 0, 1 or 2 elements.
% positions and speeds have to be given in compatible units.

% x0_a, mag_v_a, x0_b, v_b

  delta = x0_a - x0_b;
  L = hypot(delta(1), delta(2));
  % course from B to A
  lambda = atan2(delta(2), delta(1));
  phi_b = atan2(v_b(2), v_b(1));
  beta = lambda - phi_b;
  mag_v_b = hypot(v_b(1), v_b(2));
  [alpha, t] = coll(mag_v_a, mag_v_b, beta, L);

  if (any(abs(alpha + beta - pi) < 1/180.0*pi))
    disp("imprecision");  
  endif

  % lambda - pi is course from A to B
  phi_a = normalize(lambda - pi + alpha);
endfunction 


function [alpha_out, t] = coll(v_a, v_b, beta, L)
  rat = sin(beta) * v_b / v_a;
  if abs(rat) > 1
    alpha_out = [];
    t = [];
    return;
  elif rat == 1
    alpha = [pi/2];
  else
    alpha_1 = asin(rat); 
    alpha_2 = pi - alpha_1;
    alpha = [alpha_1 alpha_2];
  endif
  t = [];
  alpha_out = [];
  for a = alpha
    rel_speed = v_a * cos(a) + v_b * cos(beta);
    if (abs(rel_speed) > 0.001)
      t_ = L / rel_speed;
    else
      t_ = -1;
    endif
    if t_ >= 0
      t = [t t_];
      alpha_out = [alpha_out a];
    endif
  endfor
  % alpha_out, t
endfunction
