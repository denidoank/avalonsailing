% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, May 2011

function generate_gamma_sail_table(alpha_wind, gamma_S_opt, F_x_opt, heeling_opt, wind_speed)
% function generate_gamma_sail_table(alpha_wind, gamma_S_opt, F_x_opt, heeling_opt, wind_speed)
% Script to make const table header files for the optimal sail angle gamma_S_opt 
% in dependency of the apparent wind direction.
% for saill control in C++.
% Produces a file named "gamma_sail_tab.h" in the current directory.
% Inputs in rad, table in degrees.


filename = "gamma_sail_tab_common.h"
[fid, msg] = fopen(filename, "w");
if fid == -1
  msg
  error(msg);
  return;
endif
fprintf(fid, "// Copyright 2011 The Avalon Project Authors. All rights reserved.\n");
fprintf(fid, "// Use of this source code is governed by the Apache License 2.0\n");
fprintf(fid, "// that can be found in the LICENSE file.\n");
fprintf(fid, "// Steffen Grundmann, May 2011\n");
fprintf(fid, "const double alpha_wind_step = %g;  // rad\n", alpha_wind(2) - alpha_wind(1));
fprintf(fid, "const double alpha_wind_min = %g;   // rad\n", min(alpha_wind));
fprintf(fid, "const double alpha_wind_max = %g;   // rad\n", max(alpha_wind));
fclose(fid);
filename
"Done."

speed_int = round(wind_speed);
filename = ["gamma_sail_tab_", num2str(speed_int), ".h"];
[fid, msg] = fopen(filename, "w");
if fid == -1
  msg
  error(msg);
  return;
endif
assert(all(alpha_wind >= 0));

fprintf(fid, "// Copyright 2011 The Avalon Project Authors. All rights reserved.\n");
fprintf(fid, "// Use of this source code is governed by the Apache License 2.0\n");
fprintf(fid, "// that can be found in the LICENSE file.\n");
fprintf(fid, "// Steffen Grundmann, May 2011\n");

fprintf(fid, "// alpha_wind/rad, gamma_sail_opt/rad, F_x_opt/N, heeling_opt/rad\n");
fprintf(fid, "// last row in each table is filled with zeros\n");


fprintf(fid, "// For wind_magnitude = %g m/s.\n", wind_speed);

fprintf(fid, "const double alpha_wind_tab_%d[] = {\n", speed_int);
fprintf(fid, "%g,\n", alpha_wind)
fprintf(fid, "0};\n")

fprintf(fid, "const double gamma_sail_tab_%d[] = {\n", speed_int);
fprintf(fid, "%g,\n", gamma_S_opt)
fprintf(fid, "0};\n")

fprintf(fid, "const double force_x_tab_%d[] = {\n", speed_int);
fprintf(fid, "%g,\n", F_x_opt)
fprintf(fid, "0};\n")

fprintf(fid, "const double heeling_tab_%d[] = {\n", speed_int);
fprintf(fid, "%g,\n", heeling_opt)
fprintf(fid, "0};\n")

fclose(fid);
"Done"
endfunction