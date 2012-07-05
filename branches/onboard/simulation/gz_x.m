% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function gz_x = gz_x(phi_x)
% function gz_x(phi_x)
% phi_x in rad, result is the righting moment lever length in meters
% see endbericht, graph on p. 66

deg = rad2deg(phi_x);

% 0.969 76 degree
% 0.589 at 30 degree (see endbericht p. 65) (approximation: 0.61)
% 0 at 0 degree
% negative sign because it turns the boat back into the vertical direction
% no good estimation beyond 120 degree

signum = sign(deg);
deg = abs(deg);

C = 0.969 / 76^2;
gz_x = -signum .* (0.969 - C * (deg - 76) .^ 2 );
endfunction
