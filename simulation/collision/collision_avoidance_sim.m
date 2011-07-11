% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, June 2011

function collision_avoidance_sim(mag_v_a, x0, v, t_scope, minimum_distance, phi_c, t_c, v_a, blocked_start, blocked_end, plot_this)

figure 

hold on
% all circles around collision points
for i=1:length(phi_c)
  collision_point = v_a(i) * t_c(i) * [cos(phi_c(i)); sin(phi_c(i))];
  c = circle(collision_point , minimum_distance);
  plot(c(1, :), c(2, :), "r.");
endfor

% blocked circle segments
radius = t_scope * max(v_a)(1);
line = blocked_segments([0; 0], radius, blocked_start, blocked_end);
plot(line(1, :), line(2, :), "k");

axis([-1.2*radius 1.2*radius -1.2*radius 1.2*radius], "equal");
xlabel("x / m");
ylabel("y / m");

hold off

"Done, switch to Graphics window!"
pause
endfunction

