% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function C = physical_constants()
% function C = physical_constants()
% These are physical constants (as long as the temperature does not change)

% caching
persistent cached_const_structure;
persistent counter = 0;
if counter > 0
  C = cached_const_structure;
  counter++;
  return
endif
counter++;

% Water
% density (sea water): 
C.rho_water = 1030;   % kg/m3
% kinematic viscosity v (ny) (sea water at 20 degrees C):
C.ny_water =  1.034E-6;  % m2/s
% dynamic viscosity 
C.my_water =  1.002E-3;  %  N s/m2

% Air
% http://www3.fh-swf.de/fbtbw/klehr/download/Dichte_feuchter_Luft.pdf
% wet air has a lower density
% http://www3.fh-swf.de/fbtbw/klehr/klehr_Klehr-Dichte-feuchter-Luft.htm
% at 25 degree and 50% relative humdity
C.rho_air = 1.184;   % kg/m3
% dynamic viscosity
C.my_air = 18.27E-6;  % Pa s
% kinematic viscosity v (ny) (dry air at 20 degrees C): (+-5%)
C.ny_air = C.my_air / C.rho_air;

% Gravity
C.gravity_acceleration = 9.81; % m/s^2

cached_const_structure = C;
