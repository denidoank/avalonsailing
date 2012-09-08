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


#endif  // HELMSMAN_RUDDER_CONTROLLER_CONST_H
