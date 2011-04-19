% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function c_damp = c_damp_rot_water(area, length)
% function c_damp = c_damp_rot_water(area, length)
% for M = -c_damp * omega^2
% area is section area * c_drag!

C = physical_constants();
% F = C.rho_water / 2 * A/2 * v^2
radius = length/2*0.707;
c_damp = C.rho_water * area / 2 * radius^3;
endfunction