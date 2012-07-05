% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011
 
function r  = rotation_matrix(rz1, ry2, rx3)

% From earth system E to boat system B
% See Euler-angles http://mathworld.wolfram.com/EulerAngles.html
% http://planning.cs.uiuc.edu/node102.html
% and all 12 different flavours here: http://en.wikipedia.org/wiki/Euler_angles
% Yaw-Pitch-Roll convention, i.e. from the global coordinate system,
% heading or trim , heel, roll
% first turn the boat around its vertical axis (mast axis), heading
% then turn it around the y axis (pitch)
% then turn it around its x axis. (heel, roll)
% or also the German standard Luftfahrtnorm (DIN 9300) (Yaw-Pitch-Roll, Z, Y’, X’’)
% http://www.basiliscus.com/CaseStudy/Axes.html
% 
% Given a vector v in earth coordinates and a boat
% which is rotated by the Euler angles yaw, pitch, and roll
% (in that sequence) we calculate the orientation of the vector v in the 
% boats system by v_boat = rotation_matrix(yaw, pitch, roll) * v .

% earth to system1 
% x1 = M1 * x_E
% 
M1 = [ ...
cos(rz1) sin(rz1) 0
-sin(rz1) cos(rz1) 0
0               0      1];
%system1 to system2
M2 = [ ...
cos(ry2) 0  -sin(ry2)
0        1     0
sin(ry2) 0  cos(ry2)];
% system 2 to boat
M3 = [ ...
1         0       0
0     cos(rx3) sin(rx3)
0    -sin(rx3) cos(rx3)];

r = M3 * M2 * M1;	

% alternatively
c1 = cos(rz1);
s1 = sin(rz1);
c2 = cos(ry2);
s2 = sin(ry2);
c3 = cos(rx3);
s3 = sin(rx3);

% Luftfahrtnorm (DIN 9300) (Yaw-Pitch-Roll, Z, Y’, X’’)
% http://de.wikipedia.org/wiki/Eulersche_Winkel
% mapping angle to angle index:
% phi 3
% theta 2
% psi 1
r_new = [ ...
c2*c1            c2*s1        -s2;
s3*s2*c1-c3*s1 s3*s2*s1+c3*c1 s3*c2;
c3*s2*c1+s3*s1 c3*s2*s1-s3*c1 c3*c2];

delta2 = r - r_new;
assert(norm(delta2) < 1E-6);