% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function [rey_low, tab_low, rey_high, tab_high] = polar_tab_naca0010(reynolds_number);
% function [rey_low, tab_low, rey_high, tab_high] = polar_tab_naca0010(reynolds_number);
% Aerodynamic data of the symmetric NACA0010 profile. 
% The tables found here are a combination of JavaFoil output and , for alpha > 20 degree experimental data.
% Data about the position of the center of forces from http://www2.foi.se/rapp/foir1305.pdf .
  
% strongly adapted for |alpha| > 20 degree after seeing:
% http://www.aerospaceweb.org/question/airfoils/q0150b.shtml
% Arm from http://www2.foi.se/rapp/foir1305.pdf 
assert (reynolds_number >= 0)

% JavaFoil parameters: Name = NACA 0010;
% Mach = 0; Re = 75000; T.U. = 1.0; T.L. = 1.0;
% Surface Finish = 0; Stall model = 0; Transition model = 1; Aspect Ratio = 0; ground effect = 0;
% alpha Cl Cd       arm
% [degree] [-] [-]  [-]
x=[...
0   0.000  0.01258  0.25
1   0.118  0.01323  0.25
2   0.236  0.01397  0.25
3   0.352  0.01482  0.25
4   0.468  0.01589  0.25
5   0.579  0.01754  0.25
6   0.683  0.01862  0.25
7   0.632  0.05247  0.25
8   0.702  0.06057  0.25
9   0.763  0.07001  0.25
10  0.812  0.08025  0.25
11  0.849  0.0941   0.25
12  0.873  0.11105  0.25
13  0.883  0.12844  0.25
14  0.882  0.14917  0.26
15  0.867  0.16875  0.27
16  0.827  0.1957   0.28
17  0.78   0.22538  0.3
18  0.729  0.25912  0.32
19  0.636  0.29309  0.335
20  0.6    0.3      0.35
30  0.85   0.6      0.4
40  1.03   0.95     0.425
50  1.02   1.25     0.45
90  0.1    1.8      0.5
130 -0.85  1.25     0.55
140 -0.95  0.96     0.575
150 -0.8   0.6      0.65
160 -0.62  0.3      0.7
170 -0.9   0.11     0.73  
180 0      0.09     0.75

% Mach = 0; Re = 750000; T.U. = 1.0; T.L. = 1.0;
0  0       0.00917  0.25
1  0.118   0.00929  0.25
2  0.236   0.00949  0.25
3  0.353   0.00984  0.25
4  0.468   0.01089  0.25
5  0.58    0.01191  0.25
6  0.686   0.01183  0.25
7  0.784   0.01202  0.25
8  0.871   0.01312  0.25
9  0.944   0.0146   0.25
10  0.991  0.01685  0.25
11  0.871  0.06418  0.25
12  0.879  0.08052  0.25
13  0.887  0.09268  0.25
14  0.884  0.10543  0.26
15  0.868  0.11769  0.27
16  0.828  0.13275  0.28
17  0.781  0.14813  0.3
18  0.73   0.16838  0.32
19  0.637  0.18606  0.335
20  0.6    0.3      0.35
30  0.85   0.6      0.4
40  1.03   0.95     0.425
50  1.02   1.25     0.45
90  0.1    1.8      0.5
130 -0.85  1.25     0.55
140 -0.95  0.96     0.575
150 -0.8   0.6      0.65
160 -0.62  0.3      0.7
170 -0.9   0.11     0.73  
180 0      0.09     0.75];

rey_max = 2; % 750000 max
lines_in_each = 31;
assert(rows(x) == rey_max * lines_in_each)
rey_tab = [75000 750000];
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
