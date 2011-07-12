// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011
#ifndef HELMSMAN_RUDDER_CONTROLLER_CONST_H
#define HELMSMAN_RUDDER_CONTROLLER_CONST_H

// See simulation/design_controller/design_controller.m
// Closed loop 5s response time. 
const double kStateFeedback1 = 452.39;
const double kStateFeedback2 = 563.75;
const double kStateFeedback3 = 291.71;


// Closed loop 10s response time. 
// const double kStateFeedback1 = 226.195;
// const double kStateFeedback2 = 140.938;
// const double kStateFeedback3 = 36.463;

// Closed loop 10s response time, energy-saving (one could say mini-phlegmatic) flavour. 
// const double kStateFeedback1 = 226.195;
// const double kStateFeedback2 = 114.290;
// const double kStateFeedback3 = 19.720;

// Closed loop 25s response time, very energy-saving (one could say phlegmatic) flavour. 
// const double kStateFeedback1 = 179.0708;
// const double kStateFeedback2 = 72.8377;
// const double kStateFeedback3 = 9.8600;

// Closed loop 45s response time, absolutely energy-saving (one could say mega-phlegmatic) flavour. 
// const double kStateFeedback1 = 89.5354;
// const double kStateFeedback2 = 18.2094;
// const double kStateFeedback3 = 1.2325;

#endif  // HELMSMAN_RUDDER_CONTROLLER_CONST_H