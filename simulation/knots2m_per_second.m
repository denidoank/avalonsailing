% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function m_per_second = knots2m_per_second(knot)
% function m_per_second = knots2m_per_second(knot)
m_per_second = knot * 1852.0 / 3600;
end
