% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function c_induced = c_induced_of(part, c_lift_2d)
% function c_induced = c_induced_of(part, c_lift_2d)
% Additional term to the drag coefficient representing
% the drag caused by the wing tips.
% See: http://en.wikipedia.org/wiki/Aspect_ratio_(wing)
% part = "keel", "rudder" or "sail"
% c_lift_2d: lift coefficient (2D)

lambda = lambda_of(part);
% Oswald efficiency number e
% http://en.wikipedia.org/wiki/Oswald_efficiency_number
% http://en.wikipedia.org/wiki/Elliptical_wing
% http://en.wikipedia.org/wiki/Induced_drag#Calculation_of_Induced_drag
e = 0.9;  %  "typically 0.85 to 0.95" 
c_induced = c_lift_2d^2 / (pi * lambda * e);
endfunction