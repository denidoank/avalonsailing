// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, July 2011
#ifndef SKIPPER_PLANS_H
#define SKIPPER_PLANS_H

#include "skipper/target_circle_cascade.h"

/* input for plan generation:
% The Caribbean Plan
points = [ ...
16.877  -61.7721 0.09  % 15 km south of Antiguas coast is the final target point
16.877  -61.2887 0.1
17   -60   0.5
18   -58   1
23.5 -25   3        % Northern Wendekreis
33   -20   2        % west of Madeira
43   -15   1
45   -13   1        % NW of La Coruna
48.2 -5    0.25 ];  % W of Brest
*/

typedef const TargetCirclePoint TCP;

const TargetCirclePoint caribbean_plan[] = {
TCP( 16.877, -61.7721,     0.09),
TCP( 16.877, -61.7144,  0.09125),
TCP( 16.877,  -61.656,   0.0925),
TCP( 16.877, -61.5967,  0.09375),
TCP( 16.877, -61.5367,    0.095),
TCP( 16.877, -61.4759,  0.09625),
TCP( 16.877, -61.4143,   0.0975),
TCP( 16.877, -61.3519,  0.09875),
TCP( 16.877, -61.2887,      0.1),
TCP(16.8854, -61.2007, 0.157143),
TCP(16.8969, -61.0806, 0.214286),
TCP(16.9114, -60.9285, 0.271429),
TCP( 16.929, -60.7444, 0.328571),
TCP(16.9496, -60.5283, 0.385714),
TCP(16.9733, -60.2802, 0.442857),
TCP(     17,      -60,      0.5),
TCP(  17.15,    -59.7,      0.6),
TCP( 17.325,   -59.35,      0.7),
TCP( 17.525,   -58.95,      0.8),
TCP(  17.75,    -58.5,      0.9),
TCP(     18,      -58,        1),
TCP(18.1272, -57.2368,  1.08696),
TCP(18.2646, -56.4126,  1.17391),
TCP(18.4121, -55.5273,  1.26087),
TCP(18.5698, -54.5809,  1.34783),
TCP(18.7377, -53.5735,  1.43478),
TCP(18.9158, -52.5051,  1.52174),
TCP(19.1041, -51.3756,   1.6087),
TCP(19.3025,  -50.185,  1.69565),
TCP(19.5111, -48.9334,  1.78261),
TCP(19.7299, -47.6207,  1.86957),
TCP(19.9588,  -46.247,  1.95652),
TCP( 20.198, -44.8122,  2.04348),
TCP(20.4473, -43.3164,  2.13043),
TCP(20.7068, -41.7595,  2.21739),
TCP(20.9764, -40.1415,  2.30435),
TCP(21.2562, -38.4625,   2.3913),
TCP(21.5463, -36.7225,  2.47826),
TCP(21.8464, -34.9214,  2.56522),
TCP(22.1568, -33.0592,  2.65217),
TCP(22.4773,  -31.136,  2.73913),
TCP( 22.808, -29.1517,  2.82609),
TCP(23.1489, -27.1064,  2.91304),
TCP(   23.5,      -25,        3),
TCP(25.0966, -24.1597,  2.85714),
TCP(26.6134, -23.3613,  2.71429),
TCP(28.0504,  -22.605,  2.57143),
TCP(29.4076, -21.8908,  2.42857),
TCP(30.6849, -21.2185,  2.28571),
TCP(31.8824, -20.5882,  2.14286),
TCP(     33,      -20,        2),
TCP(34.1932, -19.4034,  1.90909),
TCP(35.3295, -18.8352,  1.81818),
TCP(36.4091, -18.2955,  1.72727),
TCP(37.4318, -17.7841,  1.63636),
TCP(38.3977, -17.3011,  1.54545),
TCP(39.3068, -16.8466,  1.45455),
TCP(40.1591, -16.4205,  1.36364),
TCP(40.9545, -16.0227,  1.27273),
TCP(41.6932, -15.6534,  1.18182),
TCP( 42.375, -15.3125,  1.09091),
TCP(     43,      -15,        1),
TCP(   43.4,    -14.6,        1),
TCP(   43.8,    -14.2,        1),
TCP(   44.2,    -13.8,        1),
TCP(   44.6,    -13.4,        1),
TCP(     45,      -13,        1),
TCP(45.2673, -12.3318, 0.960526),
TCP(45.5236, -11.6911, 0.921053),
TCP(45.7689, -11.0778, 0.881579),
TCP(46.0032,  -10.492, 0.842105),
TCP(46.2265, -9.93364, 0.802632),
TCP(46.4389, -9.40275, 0.763158),
TCP(46.6403, -8.89931, 0.723684),
TCP(46.8307, -8.42334, 0.684211),
TCP(47.0101, -7.97483, 0.644737),
TCP(47.1785, -7.55378, 0.605263),
TCP(47.3359, -7.16018, 0.565789),
TCP(47.4824, -6.79405, 0.526316),
TCP(47.6178, -6.45538, 0.486842),
TCP(47.7423, -6.14416, 0.447368),
TCP(47.8558, -5.86041, 0.407895),
TCP(47.9584, -5.60412, 0.368421),
TCP(48.0499, -5.37529, 0.328947),
TCP(48.1304, -5.17391, 0.289474),
TCP(   48.2,       -5,     0.25),
TCP(0, 0, 0)                      // end marker
};
const TargetCircle caribbean_final(caribbean_plan[0].lat_lon, 0.09);


// Lake Zuerich plans, r = 550m for the plan, r=2000m to find the correct location initially
// kilchberg thalwil horgen au waedenswil wollerau

const TargetCirclePoint sukku_plan[] = {
TCP(47.3477, 8.5431, 0.005),
TCP(0, 0, 0)}; // end marker
const TargetCircle sukku(sukku_plan[0].lat_lon, 0.02);

const TargetCirclePoint kilchberg_plan[] = {
TCP(47.3250, 8.5623, 0.005),
TCP(0, 0, 0)}; // end marker
const TargetCircle kilchberg(kilchberg_plan[0].lat_lon, 0.02);

const TargetCirclePoint thalwil_plan[] = {
TCP(47.2962, 8.5812, 0.005),
TCP(0, 0, 0)}; // end marker
const TargetCircle thalwil(thalwil_plan[0].lat_lon, 0.02);

const TargetCirclePoint horgen_plan[] = {
TCP(47.2699, 8.6052, 0.005),
TCP(0, 0, 0)}; // end marker
const TargetCircle horgen(horgen_plan[0].lat_lon, 0.02);

const TargetCirclePoint au_plan[] = {
TCP(47.2584, 8.6468, 0.005),
TCP(0, 0, 0)}; // end marker
const TargetCircle au(au_plan[0].lat_lon, 0.02);

const TargetCirclePoint waedenswil_plan[] = {
TCP(47.2379, 8.6862, 0.005),
TCP(0, 0, 0)}; // end marker
const TargetCircle waedenswil(waedenswil_plan[0].lat_lon, 0.02);

const TargetCirclePoint wollerau_plan[] = {
TCP(47.2207, 8.7220, 0.005),
TCP(0, 0, 0)}; // end marker
const TargetCircle wollerau(wollerau_plan[0].lat_lon, 0.02);


// for checks of the correct initial GPS data
const TargetCirclePoint brest_check[] = {
TargetCirclePoint( 48.2390, -4.7698, 1.0),
TargetCirclePoint(0, 0, 0)}; // end marker
const TargetCircle brest(brest_check[0].lat_lon, brest_check[0].radius_deg);




#endif  // SKIPPER_PLANS_H
