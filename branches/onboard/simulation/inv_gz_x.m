% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function phi_x = inv_gz_x(lever_x)
% function phi_x = inv_gz_x(lever_x)
% inverse of gz_x(), calculates the angle phi_x from
% the righting moment lever length in meters.
% There are 2 solutions, the one with the smaller magnitude is returned.
% phi_x in radians.
% If the torque would turn the boat around, +-angle_max will be returned. 
% see gz_x.m and endbericht, graph on p. 66
lever_max = 0.969;
angle_max = 76;

signum = sign(lever_x);
lever_x = abs(lever_x);
lever_x = min(lever_x, lever_max * ones(size(lever_x)));

C = lever_max / angle_max^2;
deg = angle_max - sqrt((lever_max - lever_x) / C);
phi_x = -signum .* deg2rad(deg);
endfunction
