% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% make tables for the simulation in C++

function generate_boat_h()
% function generate_boat_h()
% Generate C++ header file "boat.h" in the current directory.
[fid, msg] = fopen("boat.h", "w");
if fid == -1
  msg
  error(msg);
  return;
endif

B = boat();

fprintf(fid, [...
"// Copyright 2011 The Avalon Project Authors. All rights reserved.\n", ...
"// Use of this source code is governed by the Apache License 2.0\n", ...
"// that can be found in the LICENSE file.\n" ...
"#ifndef HELMSMAN_BOAT_H_\n" ...
"#define HELMSMAN_BOAT_H_\n"]);
ts = strftime ("%R (%Z) %A %e %B %Y", localtime (time ()));
fprintf(fid, "// Created by simulation/generate_boat_h.m on \n// %s\n", ts);

fprintf(fid, "namespace {\n");

fprintf(fid, "const double kMass = %g;      // kg \n", B.mass);
fprintf(fid, "const double kInertiaZ = %g;  // kg m^2\n", B.inertia_z);
fprintf(fid, "const double kAreaR = %g;     // m^2\n", B.area_R);
fprintf(fid, "const double kNumberR = %g;   // number of rudders\n", B.number_R);
% left and right rudder axis from COG, in x-y plane
radius = sqrt(B.R(1, 1)^2 + (B.distance_lr_R / 2)^2);
fprintf(fid, "const double kLeverR = %g;    // m, COG to rudder axis lever\n", radius); 
fprintf(fid, "const double kWindSensorOffsetRad = %g;   // rad\n", B.offset_rad_W);

C = physical_constants();

fprintf(fid, "const double kRhoWater = %g;  // kg/m^3\n", C.rho_water);

fprintf(fid, "const double kOmegaMaxSail = %g;  // rad/s\n", B.omega_max_S);
fprintf(fid, "const double kOmegaMaxRudder = %g;  // rad/s\n", B.omega_max_R);


fprintf(fid, "}  // namespace\n");
fprintf(fid, "#endif  // HELMSMAN_BOAT_H_\n");

fclose(fid);
endfunction
