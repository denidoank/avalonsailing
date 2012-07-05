% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function result = angle_of_attack_test() 

gamma = 0;

wind = [0; 0; 0];
[alpha, magnitude] = angle_of_attack(wind, gamma);
% If the z component of wind is zero the following is always true:
if wind(3) == 0
  assert_eq(sqrt(sumsq(wind)), magnitude);
endif
% No wind so no angle of attack can be calculated.

wind = [-1; 0; 0];
[alpha, magnitude] = angle_of_attack(wind, gamma);
% If the z component of wind is zero the following is always true:
if wind(3) == 0
  assert_eq(sqrt(sumsq(wind)), magnitude);
endif
assert_eq(0, alpha);
assert_eq(1, magnitude);

wind = [1; 0; 0];
[alpha, magnitude] = angle_of_attack(wind, gamma);
assert_eq(pi, alpha);
assert_eq(1, magnitude);

wind = [-1; 1; 0];
[alpha, magnitude] = angle_of_attack(wind, gamma);
assert(alpha == pi / 4 );
assert_eq(sqrt(2) * 1, magnitude);

wind = [1; 1; 0];
[alpha, magnitude] = angle_of_attack(wind, gamma);
assert(alpha == 3 * pi / 4 );
assert_eq(sqrt(2) * 1, magnitude);


gamma = pi / 4;

wind = [3; 3; 0];
[alpha, magnitude] = angle_of_attack(wind, gamma);
% If the z component of wind is zero the following is always true:
if wind(3) == 0
  assert_eq(sqrt(sumsq(wind)), magnitude);
endif
assert_eq(pi, alpha);
assert_eq(sqrt(2) * 3, magnitude);

wind = [-1; 0; 0];
[alpha, magnitude] = angle_of_attack(wind, gamma);
assert_eq(pi / 4, alpha);
assert_eq(1, magnitude);

wind = [1; 0; 0];
[alpha, magnitude] = angle_of_attack(wind, gamma);
assert_eq(-3 * pi / 4, alpha);
assert_eq(1, magnitude);

result = 1;
endfunction