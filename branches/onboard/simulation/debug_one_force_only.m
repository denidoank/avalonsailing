% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function debug_one_force_only(part)
% function debug_one_force_only(part)
% For debugging purposes one may enter a part out of
% "R", "K", "H", "S", "B", "D", "RX", "RY" or 
% "Rudder", "Keel", "Hull", "Sail", "Bomb", "Deck", 
% "RightingX", "RightingY"
% to suppress all other forces.
% debug_one_force_only("Deck") switches the forces function into a mode producing
% the aerodynamic resistance of the deck only.
  
global debug_switches;
debug_switches.R = 0;
debug_switches.K = 0;
debug_switches.H = 0;
debug_switches.S = 0;
debug_switches.B = 0;
debug_switches.D = 0;
debug_switches.RX = 0;
debug_switches.RY = 0;

disp ([part, " only!"]); 
switch(part) 
  case {"R" "Rudder" "rudder"}
    debug_switches.R = 1;
  case {"K" "Keel" "keel"}
      debug_switches.K = 1;
  case {"H" "Hull" "hull"}
      debug_switches.H = 1;
  case {"S" "Sail" "sail"}
      debug_switches.S = 1;
  case {"B" "Bomb" "bomb"}
      debug_switches.B = 1;
  case {"D" "Deck" "deck"}
      debug_switches.D = 1;
  case {"RX" "RightingX" "rightingX"}
      debug_switches.RX = 1;
  case {"RY" "RightingY" "rightingY"}
      debug_switches.RY = 1;
    otherwise
      error(["Unknown part ", part]);
  endswitch
endfunction