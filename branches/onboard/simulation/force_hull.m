% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function f = force_hull(stream_H)
% function f = force_hull(stream_H)
% hydrodynamic force of the boat hull.
% Normally we would calculate the hull resistance as we do
% for an airfoil. But as we have no geometry definition and no
% 3D-windchannel simulation we make do with a superposition of the
% resistance forces in 3 axis (which is wrong, but acceptable
% under the circumstances).
% stream_H, 3 dimensional stream vector
B = boat();
f = sign(stream_H) .* stream_H.^2 .* B.damping_trans_H;
endfunction