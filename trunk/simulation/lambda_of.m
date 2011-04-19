% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function lambda = lambda_of(part)
% function lambda = lambda_of(part)
% lambda is the ratio of wingspan to average wing width,
% i.e. expresses the leanness of a wing.
% See aspect ratio of a wing: http://en.wikipedia.org/wiki/Aspect_ratio_(wing)
% http://de.wikipedia.org/wiki/Streckung_(Tragfl%C3%A4che)
% part is "keel", "rudder" or "sail"

Boat = boat();
if strcmp(part, "keel")
  lambda = 2 * Boat.area_K / (Boat.length_K + Boat.diameter_B)^2; 
elseif strcmp(part, "rudder")
  lambda = 2 * Boat.area_R / Boat.length_R^2;
elseif strcmp(part, "sail")
  lambda = 2 * Boat.area_S / Boat.length_S^2;
else
  part
  error("part undefined");
endif
endfunction
