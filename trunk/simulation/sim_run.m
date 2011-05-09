% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function [result, t_sim, x_sim, i_sim] = sim_run(trial_number, t_end, ...
    make_inputs0_function=@make_inputs0_default, ...
    make_x0_function=@make_x0_default, ...
    make_inputs_function=@make_inputs_default, ...
    check_result_function=@check_result_default)
% function [result, t_sim, x_sim, i_sim] = sim_run(trial_number, t_end, ...
%    make_inputs0_function=@make_inputs0_default, ...
%    make_x0_function=@make_x0_default, ...
%    make_inputs_function=@make_inputs_default, ...
%    check_result_function=@check_result_default)
% Run a simulation.
% trial_number: an integral number to control the make_X functions for series of simulations
% t_end: the end time of the simulation
% make_inputs0_function: a function handle producing an initial inputs vector (of dim (S.no_of_inputs, 1))
%   signature inputs0 = <make_inputs_function>(trial_number)
% make_x0_function: a function handle producing the initial state vector (of dim (S.no_of_states, 1))
%   signature x0 = <make_x0_function>(trial_number, inputs0)
% make_inputs_function: a function handle producing a inputs vector (of dim (S.no_of_inputs, 1))
%   signature inputs = <make_inputs_vector>(trial_number, tt, current_state_vector)
%   which models external conditions (wind, stream, waves) and control loops.
%   tt is the current simulation time.
% check_result_function: a function handle checking the resulting state time series x_sim for success of the test
%   signature: success = <check_result_function>(trial_number, t, x_sim)
%   t is the time vector
%
% result: 1 for a successful simulation, 0 otherwise
% t_sim: vector of time, corresponding to the rows in x_sim and i_sim
% x_sim: matrix of simulation results, each row is a transposed state vector
% i_sim: matrix of simulation results, each row is a transposed inputs vector

"Using the function handles:"
functions(make_inputs0_function)
functions(make_x0_function)
functions(make_inputs_function)
functions(check_result_function)

S = simulation_options();
inputs = make_inputs0_function(trial_number)
x0 = make_x0_function(trial_number, inputs);

% S.T is the sampling period of the innermost control loop and the period of the saves state time series.
t_sim = 0:S.T:t_end;
x_sim = zeros(length(t_sim), S.no_of_states);
i_sim = zeros(length(t_sim), S.no_of_inputs);

x_sim(1, :) = x0';
i_sim(1, :) = inputs';

i = 2;
for tt = t_sim(2:length(t_sim));
  saved_inputs(inputs);

  % simulate physics
  t_one_step = tt + (0:S.T:S.T);
  [x_sim_one_step, should_be_2, msg] = lsode("derivatives", x0, t_one_step)
  if should_be_2 != 2
    disp(["lsode failed with: ", msg]);
    error("sim_run failed!");
  endif  
  x_current = x_sim_one_step(2, :);  % the first row is x0, the second row our new x.

  x_sim(i, :) = x_current;
  i_sim(i, :) = inputs';

  % Simulate controller and external influences.
  % It is assumed that the control software has a runtime smaller
  % than the sampling period of S.T . 
  inputs = make_inputs_function(trial_number, tt, x_current');
  x0 = x_current';
  i += 1;
endfor
result = check_result_function(trial_number, t_sim, x_sim);

endfunction