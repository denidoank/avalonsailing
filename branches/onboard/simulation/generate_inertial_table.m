% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% make tables for the simulation in C++

function generate_inertial_table()
% function generate_inertial_table()
% Generate C++ header file "inertial_table.h" in the current directory.
% See report 123, p. 25 please.
[fid, msg] = fopen("inertial_table.h", "w");
if fid == -1
  msg
  error(msg);
  return;
endif
B = boat();
M_inv = B.mass_matrix_inverted;

fprintf(fid, "// Inertial table M^-1 (see report 123, p.25, eq.3.3 \n");
fprintf(fid, "const int m_inv_rows = %d; \n", rows(M_inv));
fprintf(fid, "const int m_inv_columns = %d; \n", columns(M_inv));
fprintf(fid, "const double inertial_table[] = {\n ")
for i = 1:rows(M_inv)
  for j = 1:columns(M_inv)
    fprintf(fid, " %g,", M_inv(i, j));
  endfor
  fprintf(fid, "\n");
endfor
fprintf(fid, "-1E9};\n");
fclose(fid);
endfunction