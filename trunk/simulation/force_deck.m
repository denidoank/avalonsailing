% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function f = force_deck(wind_D)
% function f = force_deck(wind_D)
% aerodynamic force of the boat deck.
% Normally we would calculate the decks resistance as we do
% for an airfoil. But as we have no geometry definition and no
% 3D-windchannel simulation we make do with a superposition of the
% resistance forces in 3 axis (which is wrong, but acceptable
% under the circumstances , forces are small).
B = boat();
f = sign(wind_D) .* wind_D.^2 .* B.damping_trans_D;
endfunction