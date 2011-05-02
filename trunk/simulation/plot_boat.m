% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function plot_boat(pos, theta, gamma_sail, gamma_rudder)
% zero is at COG
% angles are in radians and have the mathematical orientation, when seen from below the boat
% (under water, or from the center of the earth.
% pos : position []
% h: heading (as see from the center of the earth, mathematical sense of orientation) 
% gamma_sail is angle from bow to sail mast (sail leech at portside is pi/2 or 90 degree)
% gamma_rudder is angle from bow to rudder front (straight is 0 radians, 
% a rudder for a starboard turn is -5 degree) 
% dimensions in m

rudder_off_middle = 0.3;
rudder_length = 0.5;   % for better visibility
rudder_x = -2;

rudders = [ rudder_x rudder_x-rudder_length*cos(gamma_rudder) rudder_x ...
            rudder_x rudder_x-rudder_length*cos(gamma_rudder) rudder_x;
            rudder_off_middle rudder_off_middle-rudder_length*sin(gamma_rudder) rudder_off_middle ... 
            -rudder_off_middle -rudder_off_middle-rudder_length*sin(gamma_rudder) -rudder_off_middle ];

shape_l = [ 2    1.2    0    -2;
            0    0.3    0.45 0.45 ];
shape_r = [-2    0      1.2  2;
           -0.45 -0.45  -0.3 0 ];
shape = [shape_l rudders shape_r]; 

sail = [ 0.4 0 0     0       0 -1.5 ;
         0   0 0.075 -0.0750 0 0];
rot_sail = theta + gamma_sail;

rot_matrix_sail = [cos(rot_sail) -sin(rot_sail)  ;
                   sin(rot_sail) cos(rot_sail)  ];
rotated_sail = pos * ones(1, columns(sail)) + rot_matrix_sail * sail;

rot_matrix=[cos(theta) -sin(theta);
            sin(theta) cos(theta)];

shifted_shape = pos * ones(1, columns(shape));

rotated = shifted_shape + rot_matrix * shape;

plot(rotated(2,:), rotated(1, :), "b", rotated_sail(2,:), rotated_sail(1,:), "b")

end
