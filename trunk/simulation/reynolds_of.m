% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function rey = reynolds_of(part, speed_m_per_s)
% rey = reynolds_of(part, speed_m_per_s)
% Reynolds number of a certain part at a certain speed.
% part = "keel", "rudder" or "sail"
% speed in m/s

Const = physical_constants();
Boat = boat();
if strcmp(part, "keel")
  rey = speed_m_per_s * Boat.length_K / Const.ny_water;
elseif strcmp(part, "rudder")
  rey = speed_m_per_s * Boat.length_R / Const.ny_water;
elseif strcmp(part, "sail")
  rey = speed_m_per_s * Boat.length_S / Const.ny_air;
else
  part
  error("part undefined");
endif

rey = abs(rey);
endfunction