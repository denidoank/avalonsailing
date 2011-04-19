% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function cof = center_of_force_of(part, gamma, arm)
% function cof = center_of_force_of(part, gamma, arm)
% part = "keel", "rudder" or "sail"
% gamma

% Where are the centers of force for sail, keel and rudder?
% The hydro- or aerodynamic part, e.g. the rudder, is rotating around an axis.
% On that axis we define an "axis point" such that the
% center of forces of the part are at the same z-coordinate of the boat.
% If the rudder was rectangular, the axis point would be in the middle of the 
% rudder depth.
% Knowing the length of the profile cord in front of the axis (front) and
% the length of the rear part (rear) together with the rotation angle gamma 
% around the rudder axis we can calculate the position of the nose and tail point
% of the profile in boat coordinates. Our center of force lays on the connecting line
% between these points.
% Then we take the moment arm value from the profile characteristics
% to find out where that point is exactly. For small angles of attack on the rudder profile
% arm is 0.25. This means that the COF is at
% (1-0.25)*nose + 0.25*tail
% If the angle of attack is 90 degrees then arm is 50, the COF is
% (1-0.5)*nose+0.5*tail, i.e. the COF is in the middle of the profile length. 
%
%    |front|     rear      | 
%
%       _________________
%     (    o              >
%       -----------------
%
%  nose  axis             tail
B = boat();
if strcmp(part, "keel")
  axis_point = B.K;
  front = B.front_K;
  rear = B.rear_K;
elseif strcmp(part, "rudder")
  axis_point = B.R;
  front = B.front_R;
  rear = B.rear_R;
elseif strcmp(part, "sail")
  axis_point = B.S;
  front = B.front_S;
  rear = B.rear_S;
else
  part
  error("part undefined");
endif

% The nose of the sail profile in the middle of the sail height
nose = front * [cos(gamma);
                sin(gamma);
                0];
% The tail point of the sail profile in the middle of the sail height
tail = rear * [-cos(gamma);
               -sin(gamma);
               0];

cof = (1-arm) * nose + arm * tail + axis_point;
endfunction