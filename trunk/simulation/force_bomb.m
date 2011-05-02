% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function f = force_bomb(stream_B)
% function f = force_bomb(stream_B)
% hydrodynamic force of the bomb.
% Because the keel has been prolonged by the bomb diameter
% the y component of the force is neglected for the bomb.
B = boat();
C = physical_constants();
v = stream_B(1);
f_x = sign(v) * v^2 * (C.rho_water / 2) * pi / 4 * B.diameter_B^2 * B.c_drag_B;

v_z = stream_B(3);
f_z = sign(v_z) * v_z^2 * (C.rho_water / 2) * B.length_B * B.diameter_B * B.c_drag_z_B;
f = [f_x; 0; f_z];
endfunction