// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011

#ifndef HELMSMAN_CLIFT_TO_RUDDER_ANGLE_H_
#define HELMSMAN_CLIFT_TO_RUDDER_ANGLE_H_

// limited is
// -1 for limitation at the lower limit of CLift
//  0 for the unlimited case
// +1 for a limitation at the positive limit (i.e. the produced alpha
//    makes a smaller than the desired CLift).

void CLiftToRudderAngle(double CLift,
                        double speed,
                        double* alpha,
                        int* limited);

#endif // HELMSMAN_CLIFT_TO_RUDDER_ANGLE_H_

