% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function [apparent_wind_angles, gamma_sail_opt, F_x_opt, heeling_opt, wind_magnitude] = ...
  optimize_gamma_sail(wind_magnitude_in, apparent_wind_angles, plot_on)
% function [apparent_wind_angles, gamma_sail_opt, F_x_opt, heeling_opt, wind_magnitude] = 
%   optimize_gamma_sail(wind_magnitude_in, apparent_wind_angles, plot_on)
% Produces the optimal angle of the sail gamma_sail_opt for every apparent wind angle.
% apparent_wind_angles: The apparent wind angle is the direction of the wind vector related to the x-axis of the boat (i.e. 0
% degree means wind directly from behind), in rad
% gamma_sail_opt: vector of best sail anges at these conditions in radians
% F_x_opt: Forward force component of sail in N
% heeling_opt: heel angle in radians
% wind_magnitude: wind speed in m/s

% default parameters
if nargin < 2
  apparent_wind_angles = deg2rad(0:5:180);
endif
if nargin < 3
  plot_on = 0;
endif

debug_switch_init();
inputs = make_inputs0_default(0);
state = make_x0_default(0, inputs);

F_x_opt = zeros(size(apparent_wind_angles));
gamma_sail_opt = zeros(size(apparent_wind_angles));
heeling_opt = zeros(size(apparent_wind_angles));
alpha_index = 1;
for wind_alpha_in = apparent_wind_angles
  unpack_inputs_script;
  wind_alpha = wind_alpha_in;
  wind_magnitude = wind_magnitude_in;
  pack_inputs_script;
  % Maximize F_x in 2 steps, first roughly (5 degree steps), then exactly
  % (with 0.2 degree steps) in the vicinity of the maximum.  
  % There is no case where the optimal sail angle is not in that range. 
  all_gamma_degrees = 20:-5:-110;
  focus_area = -5:0.2:5;
  f_y_h       = zeros(1, length(all_gamma_degrees) + length(focus_area));
  f_x_h       = f_y_h - 1E9;
  heeling_rad = f_y_h;
  i = 1;
  for gamma = all_gamma_degrees
    unpack_state_script;
    gamma_S = deg2rad(gamma);
    pack_state_script;
    heel = balance_heel(state, inputs);
    unpack_state_script;
    phi_x = heel;
    pack_state_script;
    [F, N, omega_drives] = forces(state, inputs, 0);
    f_x_h(i) = F(1, 1);
    f_y_h(i) = F(2, 1);
    heeling_rad(i) = heel;
    % heel
    i += 1;
  endfor

  f_x_h
  [F_x_max, index_max] = max(f_x_h);
  "approximate optimum"
  F_x_max
  index_max
  gamma_sail_opt_temp = index_max(1)
  all_gamma_degrees = [all_gamma_degrees gamma_sail_opt_temp+focus_area]
  for gamma = gamma_sail_opt_temp + focus_area
    unpack_state_script;
    gamma_S = deg2rad(gamma);
    pack_state_script;
    heel = balance_heel(state, inputs);  
    unpack_state_script;
    phi_x = heel;
    pack_state_script;
    [F, N, omega_drives] = forces(state, inputs, 0);
    f_x_h(i) = F(1);
    f_y_h(i) = F(2);
    heeling_rad(i) = heel;
    i += 1;
  endfor

  if plot_on
    figure
    hold on
    plot(rad2deg(heeling_rad), f_x_h);
    title(["F_x = f(heel angle) for alpha_{wind}=", num2str(rad2deg(wind_alpha)), " degrees"]);
    xlabel("heeling / degree");
    ylabel("F_x / N");
    for jjj = 1:length(all_gamma_degrees)
      text(rad2deg(heeling_rad(jjj)), f_x_h(jjj), num2str(all_gamma_degrees(jjj)));
    endfor
    hold off
  endif

  [F_x_max, index_max] = max(f_x_h);
  index_max = index_max(1);
  F_x_opt(alpha_index) = F_x_max(1);
  gamma_sail_opt(alpha_index) = all_gamma_degrees(index_max);
  "exact optimum"
  gamma_sail_opt(alpha_index)
  heeling_opt(alpha_index) = heeling_rad(index_max);
  alpha_index += 1;
endfor   % wind angles alpha

figure
plot(rad2deg(apparent_wind_angles), F_x_opt)
title(["Optimal F_x = f(alpha_{wind}) for ", num2str(wind_magnitude), " m/s"]);
xlabel("alpha_{wind} / degree");
ylabel("F_{xopt} / N");

figure
plot(rad2deg(apparent_wind_angles), gamma_sail_opt)
title(["Optimal gamma_S = f(alpha_{wind}) for ", num2str(wind_magnitude), " m/s"]);
xlabel("alpha_{wind} / degree");
ylabel("gamma_{opt} / degree");

figure
plot(rad2deg(apparent_wind_angles), rad2deg(heeling_opt))
title(["Optimal heeling = f(alpha_{wind}) for ", num2str(wind_magnitude), " m/s"]);
xlabel("alpha_{wind} / degree");
ylabel("heel_{opt} / degree");

gamma_sail_opt = deg2rad(gamma_sail_opt);
