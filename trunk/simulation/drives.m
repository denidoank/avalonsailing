% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function omega_drives = drives(gamma_star, gamma_actual)
% function omega_drives = drives(gamma_star, gamma_actual)
% Simulate the 3 drive controls as P-controlled, with  a certain rate limitation. 
B = boat();
assert(B.omega_max_S > 0);
assert(B.omega_max_R > 0);
assert(B.gain_prop_S > 0);
assert(B.gain_prop_R > 0);

gain = [B.gain_prop_S B.gain_prop_R B.gain_prop_R];
omega_max = [B.omega_max_S B.omega_max_R B.omega_max_R];
% min and max work columnwise so we arrange each drive in its own column.
unlimited = gain .* (gamma_star - gamma_actual)';
omega_drives = max(min(unlimited, omega_max), -omega_max);
omega_drives = omega_drives';     % Turn it back into a column vector.

endfunction

   