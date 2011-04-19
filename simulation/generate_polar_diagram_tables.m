% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.function plot_all_polar_diagrams()
% Steffen Grundmann, April 2011
% Script to make const table header files for the simulation of sail,
% rudder and keel in C++.

function generate_polar_diagram_tables.m
alpha = zeros(361,1);
c_lift = alpha;
c_drag = alpha;
c_arm  = alpha;

[fid, msg] = fopen("polar_diagrams_tab.h", "w");
if fid == -1
  msg
  error(msg);
  return;
endif

fprintf(fid, "// c_lift, c_drag, arm for angles of attack from 0 to 180 degree");
fprintf(fid, "// separate tables for rudder and sail speeds in m/s");
fprintf(fid, "// last row in each table is filled with zeros");
fprintf(fid, "const double angle_step = 1; \nconst double speed_step = 1;\n");
fprintf(fid, "const double angle_max = 180; \nconst double speed_max_rudder = 5;\n");
fprintf(fid, "const double speed_max_sail = 20;\n");

part = "rudder";
speeds = 0:1:5;
for v = speeds
  fprintf(fid, "const double polar_diagram_rudder_tab%g[] = {\n ", v)
  i=1;
  for a = 0:1:180
    alpha(i) = a;
    [c_lift(i), c_drag(i), c_arm(i)] = c_aero_of(part, 2, deg2rad(a));
    fprintf(fid, "%g,%g,%g,\n", c_lift(i), c_drag(i), c_arm(i))
    i += 1;   
  endfor
  fprintf(fid, "0,0,0};\n")
endfor

part = "keel";
speeds = 0:1:5;
for v = speeds
  fprintf(fid, "const double polar_diagram_rudder_tab%g[] = {\n ", v)
  i=1;
  for a = 0:1:180
    alpha(i) = a;
    [c_lift(i), c_drag(i), c_arm(i)] = c_aero_of(part, 2, deg2rad(a));
    fprintf(fid, "%g,%g,%g,\n", c_lift(i), c_drag(i), c_arm(i))
    i += 1;   
  endfor
  fprintf(fid, "0,0,0};\n")
endfor

part = "sail";
speeds = 0:1:20;
for v = speeds   
  fprintf(fid, "double polar_diagram_sail_tab%g[] = {\n ", v)
  i=1;
  for a = 0:1:180
    alpha(i) = a;
    [c_lift(i), c_drag(i), c_arm(i)] = c_aero_of(part, 2, deg2rad(a));
    fprintf(fid, "%g,%g,%g,\n", c_lift(i), c_drag(i), c_arm(i))
    i += 1; 
  endfor
  fprintf(fid, "0,0,0};\n")
endfor

fclose(fid);
endfunction