// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, June 2011
#ifndef HELMSMAN_WIND_STRENGTH_H
#define HELMSMAN_WIND_STRENGTH_H

enum WindStrengthRange {
  kCalmWind = 0,
  kNormalWind,
  kStormWind
};

// enum to string conversion
const char* WindToString(WindStrengthRange range);


WindStrengthRange WindStrength(WindStrengthRange previous_range,
                               double wind_m_s);
#endif   // HELMSMAN_WIND_STRENGTH_H

  
  






