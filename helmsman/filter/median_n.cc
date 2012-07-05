// Copyright 2006 Steffen Grundmann. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

double Median3 (const double& v1, const double& v2, const double& v3) {
  double c, a;
  if (v2 < v1) {
    c   = v1;
    a = v2;
  } else {
    c   = v2;
    a = v1;
  }
  if (c < v3) {
    return(c);         // v1 v2 v3 or v2 v1 v3
  }
  if (v3 < a) {
    return(a);       // v3 v1 v2 or v3 v2 v1
  }
  return(v3);            // v1 v3 v2 or v2 v3 v1
}

double Median5 (const double& v1, const double& v2, const double& v3,
                const double& v4, const double& v5) {
  double a, b, c;  // First we order v1, v2 and v3 into a, b, c
  if (v2 < v1) {
    a = v2;
    c   = v1;
  } else {
    a = v1;
    c   = v2;
  }
  if (c < v3) {
    // a c v3
    b = c;
    c = v3;
  } else {
    if (v3 < a) {
      // v3 a c
      b = a;
      a = v3;
    } else {
      // a v3 c
      b = v3;
    }
  }
  // so we've got  a b c  so far
  double x, y;     // then v4 and v5 are sorted into x and y and
  if (v4 < v5) {
    x = v4;
    y = v5;
  } else {
    x = v5;
    y = v4;
  }

  // finally we pick the median according to the following case table.
  // Case  a       b     c       median5
  // 1 x y                          a
  // 2 x     y                      y
  // 3 x               y            b
  // 4 x                     y      b
  // 5         x y                  y
  // 6         x       y            b
  // 7         x             y      b
  // 8               x y            x
  // 9               x       y      x
  // 10                    x y      c
  //     ------------------------> value

  if (x < a) {
    // Cases 1-4
    if ( b < y) {
      return(b);
    } else {
      return(y < a ? a : y);
    }
  } else {
    // Cases 5-10
    if (x < b) {
      // Cases 5-7
      return(y < b ? y : b);
    } else {
      // Cases 8-10
      return(c < x ? c : x);
    }
  }
}

