% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function lambda_factor = lambda_factor_of(part)
% function lambda_factor = lambda_factor_of(part)
% The polar diagram calculation for an airfoil assume a wind of infinite length.
% But in reality the wing has a finite length, air flows around the tip
% of the wing and the lift factor has to be reduced by the lamda-factor.
% http://de.wikipedia.org/wiki/Auftriebsbeiwert (the translations miss
% the specifics of this factor).
% part is "keel", "rudder" or "sail"

lambda = lambda_of(part);
lambda_factor = lambda / (lambda + 2);
endfunction
