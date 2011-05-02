% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011
 
function rotation_matrix_test()

% From earth system E to boat system B
% See Euler-angles http://mathworld.wolfram.com/EulerAngles.html
% http://planning.cs.uiuc.edu/node102.html
% and all 12 different flavours here: http://en.wikipedia.org/wiki/Euler_angles
% Yaw-Pitch-Roll convention, i.e. from the global coordinate system,
% heading or trim , heel, roll
% first turn the boat around its vertical axis (mast axis), heading
% then turn it around the y axis (pitch)
% then turn it around its x axis. (heel, roll)
% http://www.basiliscus.com/CaseStudy/Axes.html
% 
% Given a vector v in earth coordinates and a boat
% which is rotated by the Euler angles yaw, pitch, and roll
% (in that sequence) we calculate the orientation of the vector v in the 
% boats system by v_boat = rotation_matrix(yaw, pitch, roll) * v .
assert(equal([1 0 0; 0 1 0; 0 0 1], rotation_matrix(0, 0, 0))); 

assert(equal([-1 0 0; 0 -1 0; 0 0 1], rotation_matrix(pi, 0, 0)))   % ok
assert(equal([-1 0 0; 0 1 0; 0 0 -1], rotation_matrix(0, pi, 0)))   % ok
assert(equal([1 0 0; 0 -1 0; 0 0 -1], rotation_matrix(0, 0, pi)))   % ok

assert(equal([0  1 0; -1 0 0; 0 0 1], rotation_matrix( pi/2, 0, 0)))  
assert(equal([0 -1 0;  1 0 0; 0 0 1], rotation_matrix(-pi/2, 0, 0)))

assert(equal([0 0 -1; 0 1 0; 1 0 0], rotation_matrix(0, pi/2, 0)))  % ok

small = deg2rad(1);
expected = [ ...
   1      small -small;
  -small   1     small;
   small -small   1];
assert(equal(expected, rotation_matrix(small, small, small), 0.001));

endfunction