% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function [rey_low, tab_low, rey_high, tab_high] = polar_tab_sail1(reynolds_number);
% function [rey_low, tab_low, rey_high, tab_high] = polar_tab_sail1(reynolds_number);
% Aerodynamic data of the a sail profile. 
% The tables found here are a combination of JavaFoil output and , for alpha > 20 degree experimental data.
% Data about the position of the center of forces from http://www2.foi.se/rapp/foir1305.pdf .
  
% Name = Sail1;
% strongly adapted for |alpha| > 20 degree after seeing:
% http://www.aerospaceweb.org/question/airfoils/q0150b.shtml
% Arm from http://www2.foi.se/rapp/foir1305.pdf 
assert (reynolds_number >= 0, num2str(reynolds_number))

% tables for different, equidistant Reynolds numbers.
% JavaFoil parameters: Mach = 0; Re = 75000; T.U. = 1.0; T.L. = 1.0;
% Surface Finish = 0; Stall model = 0; Transition model = 1; Aspect Ratio = 0; ground effect = 0;
% alpha Cl  Cd     arm
% [degree] [-] [-] [-]
x=[...
0.0  0.000  0.02   0.25
1.0  0.000  0.02   0.25
2.0  0.000  0.02   0.25
3.0  0.1    0.02   0.25
4.0  0.4    0.028  0.25
15   1.0314 0.0577 0.25
17   0.58   0.216  0.30
23   0.575  0.34   0.33
33   0.882  0.697  0.4
45   0.9620 1.083  0.425
55   0.858  1.421  0.45
70   0.56   1.658  0.47
80   0.327  1.8    0.485
90   0.07   1.83   0.5
100 -0.183  1.758  0.52
110 -0.426  1.636  0.53
120 -0.63   1.503  0.54
130  -0.813 1.26   0.55
140  -0.897 0.9425 0.575
150  -0.704 0.604  0.65
160  -0.58  0.31   0.7
170  -0.813 0.13   0.73  
180  0      0.02   0.75

% Mach = 0; Re = 1E6;
0.0  0.000  0.02   0.25
1.0  0.000  0.02   0.25
2.0  0.000  0.02   0.25
3.0  0.1    0.02   0.25
4.0  0.4    0.028  0.25
15   1.0314 0.0577 0.25
17   0.58   0.216  0.30
23   0.575  0.34   0.33
33   0.882  0.697  0.4
45   0.9620 1.083  0.425
55   0.858  1.421  0.45
70   0.56   1.658  0.47
80   0.327  1.8    0.485
90   0.07   1.83   0.5
100 -0.183  1.758  0.52
110 -0.426  1.636  0.53
120 -0.63   1.503  0.54
130  -0.813 1.26   0.55
140  -0.897 0.9425 0.575
150  -0.704 0.604  0.65
160  -0.58  0.31   0.7
170  -0.813 0.13   0.73  
180  0      0.02   0.75];

rey_max = 2;
lines_in_each = 23;
rey_tab = [75000 1E6];
assert(rows(x) == rey_max * lines_in_each, num2str(rows(x)))
assert(length(rey_tab) == rey_max)

index_float = 1.0 + (reynolds_number - min(rey_tab)) / ...
                    (max(rey_tab) - min(rey_tab));

index_low  = max(1, min(rey_max, max(1,       floor(index_float))));
index_high = max(1,              min(rey_max, ceil( index_float))); 

first_low = 1 + (index_low-1) * lines_in_each;
last_low =      (index_low  ) * lines_in_each;
first_high = 1 + (index_high-1) * lines_in_each;
last_high =      (index_high  ) * lines_in_each;

rey_low = rey_tab(index_low);
tab_low = x(first_low:last_low,:);
rey_high = rey_tab(index_high);
tab_high = x(first_high:last_high,:);
endfunction
