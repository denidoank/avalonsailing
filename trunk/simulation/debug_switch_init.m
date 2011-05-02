% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function debug_switch_init()
% function debug_switch_init()
% Use all forces in the forces calculation.
% Switch printing of forces off. 

debug_switch_force_off();
global debug_switches;
debug_switches.print_forces = 0;
endfunction