% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function phi_y = inv_gz_y(lever_y)
% function phi_y = inv_gz_y(lever_y)
% Calculates phi_y in rad
% from the righting moment lever length in meters
% around the y axis (bow up, stern down).
% data missing from endbericht, graph on p. 66 for the
% corresponding  characteristic around the x-axis
% own estimation by grundmann
% length of the boat is 3.95m, width is 1.2m
% lever is multiplied by 3.95/1.2  
B = boat();
lever_x = lever_y * B.width_H / B.length_H;
phi_y = inv_gz_x(lever_x);
endfunction
