% speed in m/s, (6 to 24 knots)
for wind_speed = [3, 6, 9, 12]
  [alpha_wind, gamma_sail_opt, F_x_opt, heeling_opt, speed_w] = ...
      optimize_gamma_sail(wind_speed);
  generate_gamma_sail_table(alpha_wind, gamma_sail_opt, ...
                            F_x_opt, heeling_opt, speed_w);
  disp("wrote file.");
  mat_file = ["optimal_gamma_data_", num2str(speed_w) ];
  eval(["save optimal_gamma_data_", num2str(speed_w), " alpha_wind gamma_sail_opt F_x_opt heeling_opt speed_w"  ]);
endfor
ls "*.h"