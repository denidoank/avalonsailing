% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function coa = coa_trapez_of(part)
% coa = coa_trapez_of(part)
% center of area for  atrapez with parallel bottom and top sides
% a straight let edge and a tilted right edge.
% For a rectangle the center of area is in the middle, at 0.5
% of the depth. For trapezoidal shapes we have a shift to the thick side,
% i.e. a coa_fraction > 0.5 and coa_z
% x_coa and coa_z are measured from the corner of straight edge
% with the long side of the trapeze. 

% part = "keel", "rudder" or "sail"

if strcmp(part, "keel")
  short = 0.307;
  long = 0.397;  d = 1.5;
elseif strcmp(part, "rudder")
  short = 0.13;
  long = 0.2;
  d = 0.5;
elseif strcmp(part, "sail")
  short = 2;  % We assume here that the upper part of the sail is more effective because of the stronger wind speed
  long = 2;
  d = 5;
else
  part
  error("part undefined");
endif

A_rect = d * short;
A_tri = d * (long - short) / 2;
x_rect = short / 2;
z_rect = d / 2;
x_tri = (short + long)  / 2;
z_tri = d / 3;

coa_x = (x_rect * A_rect + x_tri * A_tri) / (A_rect + A_tri);
coa_z = (z_rect * A_rect + z_tri * A_tri) / (A_rect + A_tri);
coa = [coa_x; 0; coa_z];

endfunction