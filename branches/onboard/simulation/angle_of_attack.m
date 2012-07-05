% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function [angle_of_attac, magnitude] = angle_of_attack(wind, gamma);
% [angle_of_attac, magnitude] = angle_of_attack(wind, gamma);
% For a given air/water flow vector wind and a given additional rotation around
% the z-axis gamma calculate the angle of attack and the magnitude of the flows
% projection on the cross section of the profile.
% The z-component along the nose line of the air foil is supposed to have no effect.
assert(size(wind) == [3, 1], "wind is expected to be a 3D-column-vector.");
assert(size(gamma) == [1, 1], "gamma is expected to be a scalar.");

if sumsq(wind(1:2)) > 0
  angle_wind = atan2(-wind(2), -wind(1));
else
  angle_wind = 0;
endif

angle_of_attac = symmetric_normalize(gamma - angle_wind);
magnitude = sqrt(sumsq([wind(1), wind(2)]));
if abs(magnitude) < 1E-3
  angle_of_attac = 0;
endif 

endfunction