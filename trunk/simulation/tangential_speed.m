% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function tangential_speed = tangential_speed(position, omega)
% function tangential_speed = tangential_speed(position, omega)
% A point on the boat at the given position will move wth the
% computed speed vector due to the rotation vector omega.
% http://de.wikipedia.org/wiki/Winkelgeschwindigkeit
% http://en.wikipedia.org/wiki/Angular_velocity
assert([3, 1] == size(omega));
assert([3, 1] == size(position));

omega_tensor = [ ...
    0      -omega(3)  omega(2);
 omega(3)     0      -omega(1);
-omega(2)   omega(1)     0];  

tangential_speed = omega_tensor * position;
endfunction