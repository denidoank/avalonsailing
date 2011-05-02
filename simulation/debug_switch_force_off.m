% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function debug_switch_force_off(varargin)
% function debug_switch_force_off(varargin)
% For debugging purposes one may enter a list of strings from
% "R", "K", "H", "S", "B", "D", "G" or 
% "Rudder", "Keel", "Hull", "Sail", "Bomb", "Deck", 
% "Gravitation"
% to switch off the contributions of these parts to the total forces and torques.
% force_debug_switch_off("Deck") switches off the aerodynamic resistance of the deck and
% force_debug_switch_off("Bomb", "Gravitation") switches our boat into a configuration
% where it just has lost the keel bomb hydrodynamic resistance and both righting forces,
% i.e. acts like the keel bomb broke off ;-(.  
global debug_switches;
debug_switches.R = 1;
debug_switches.K = 1;
debug_switches.H = 1;
debug_switches.S = 1;
debug_switches.B = 1;
debug_switches.D = 1;
debug_switches.G = 1;

for i = 1:length(varargin)  disp (["Switching off ", varargin{i}]); 
  switch(varargin{i}) 
    case {"R" "Rudder" "rudder"}
      debug_switches.R = 0;
    case {"K" "Keel" "keel"}
      debug_switches.K = 0;
    case {"H" "Hull" "hull"}
      debug_switches.H = 0;
    case {"S" "Sail" "sail"}
      debug_switches.S = 0;
    case {"B" "Bomb" "bomb"}
      debug_switches.B = 0;
    case {"D" "Deck" "deck"}
      debug_switches.D = 0;
    case {"G" "Gravitation"}
      debug_switches.G = 0;
    otherwise
      error(["Unknown part ", varargin{i}]);
  endswitch
endforendfunction