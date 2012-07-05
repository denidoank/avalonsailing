% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function phi_x = heeling(N_x)
% Calculates the average heeling for a given torque around the x-axis.
% Returns NaN if the torque would be too big.
B = boat();
C = physical_constants();
lever_x = N_x / (B.mass * C.gravity_acceleration);
phi_x = -inv_gz_x(lever_x);
endfunction
