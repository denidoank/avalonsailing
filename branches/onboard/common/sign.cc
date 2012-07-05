// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Steffen Grundmann, May 2011

double Sign(double x) {
  if (x < 0)
    return -1;
  else if (x == 0)
    return 0;
  else
    return 1;
}

double SignNotZero(double x) {
  if (x < 0)
    return -1;
  else
    return 1;
}
