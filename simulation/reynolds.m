% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function rey = reynolds(speed_m_per_s, length_meter, ny)
% Reynolds number
% function rey = reynolds(speed_m_per_s, length_meter, ny)
rey = abs(speed_m_per_s * length_meter / ny);
endfunction