% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function result = forces_test()

result = 1;
S = simulation_options();
debug_switch_init();
state = zeros(S.no_of_states, 1);
inputs = zeros(S.no_of_inputs, 1);

% dead calm
"If the boat stays in the initial point then there are no forces nor torques."
state = zeros(S.no_of_states, 1);
[F, N, omega_drives] = forces(state, inputs, 0)
assert_eq(zeros(3, 1), F);
assert_eq(zeros(3, 1), N);
assert_eq(zeros(3, 1), omega_drives);

% x, y, z
"If the boat is at a certain point on the x-axis then there are no forces nor torques."
state = zeros(S.no_of_states, 1);
unpack_state_script;
x = 2;
pack_state_script;
[F, N, omega_drives] = forces(state, inputs, 0)
assert_eq([0; 0; 0], F, 1);
assert_eq(zeros(3, 1), N);
assert_eq(zeros(3, 1), omega_drives);

"If the boat is at a certain point on the y-axis then there are no forces nor torques."
state = zeros(S.no_of_states, 1);
unpack_state_script;
y = 2;
pack_state_script;
[F, N, omega_drives] = forces(state, inputs, 0)
assert_eq([0; 0; 0], F, 1);
assert_eq(zeros(3, 1), N);
assert_eq(zeros(3, 1), omega_drives);

"If the boat is at a certain point above the water level then there is a downward z - force."
state = zeros(S.no_of_states, 1);
unpack_state_script;
z = -0.1;
pack_state_script;
[F, N, omega_drives] = forces(state, inputs, 0)
assert_eq([0; 0; 4000], F, 20);  % WSA is about 4m^2. 4m^2 * 0.1m * 1030kg/m^3 * 9.81m/s^2 = 4000
assert_eq(zeros(3, 1), N);
assert_eq(zeros(3, 1), omega_drives);

"If the boat is at a certain point below the water level then there is an upward z - force."
state = zeros(S.no_of_states, 1);
unpack_state_script;
z = 0.1;
pack_state_script;
[F, N, omega_drives] = forces(state, inputs, 0)
assert_eq([0; 0; -4000], F, 20);  % WSA is about 4m^2. 4m^2 * 0.1m * 1030kg/m^3 * 9.81m/s^2 = 4000
assert_eq(zeros(3, 1), N);
assert_eq(zeros(3, 1), omega_drives);

% v_x, v_y, v_z
"If the boat moves forward there is an opposing resistance force."
state = zeros(S.no_of_states, 1);
unpack_state_script;
v_x = 2;
pack_state_script;
[F, N, omega_drives] = forces(state, inputs, 0)
assert_eq([-62; 0; 0], F, 1);
assert_eq(zeros(3, 1), N, 3);
assert_eq(zeros(3, 1), omega_drives);

"If the boat moves backwards there is an opposing resistance force."
% Bigger magnitude because of the asymmetric rudders and the keel resistance.
state = zeros(S.no_of_states, 1);
unpack_state_script;
v_x = -2;
pack_state_script;
[F, N, omega_drives] = forces(state, inputs, 0)
assert_eq([177; 0; 0], F, 1);
assert_eq([0; 50; 0], N, 6);
assert_eq(zeros(3, 1), omega_drives);

"If the boat moves in the y direction there is a big opposing resistance force, a positive z-torque (rudder resistance),"
"and an heeling torque in the positive phi_x direction (because the water resistance forces are much bigger than the sail resistance)"
state = zeros(S.no_of_states, 1);
unpack_state_script;
v_y = 2;
pack_state_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0)
assert_eq([0; -2900; 0], F, 100);
assert_eq([1045; 0; 600], N, 50);
assert_eq(zeros(3, 1), omega_drives);

"If the boat moves in the negative y direction then there is a big opposing resistance force, a negative z-torque (rudder resistance)"
"and a negative x torque"
state = zeros(S.no_of_states, 1);
unpack_state_script;
v_y = -2;
pack_state_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0)
assert_eq([0; 2900; 0], F, 100);
assert_eq([-1045; 0; -600], N, 50);
assert_eq(zeros(3, 1), omega_drives);
debug_print_forces(0);

"If the boat moves in the z direction there is a big opposing resistance force."
state = zeros(S.no_of_states, 1);
unpack_state_script;
v_z = 2;
pack_state_script;
[F, N, omega_drives] = forces(state, inputs, 0)
% compare with a plate of 4m^2 and a c_d of 1
% F = - 4m^2 / 2 * (2m/s)^2 * 1030 kg/m^3 * 1.0
% F = 8000N 
assert_eq([0; 0; -8200], F, 100);
assert_eq([0; 300; 0], N, 50);  % hull center is forward of center of gravity
assert_eq(zeros(3, 1), omega_drives);

% phi_x, phi_y, phi_z
"Initial heeling > 0 -> negative righting torque."
state = zeros(S.no_of_states, 1);
unpack_state_script;
phi_x = deg2rad(30);
pack_state_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0);
% see endreport p. 65
assert_eq([0; 0; 0], F, 10);
assert_eq([-3240; 0; 0], N, 20);
assert_eq(zeros(3, 1), omega_drives);

"Initial nick (bow under water)  -> positive righting torque on z axis."
state = zeros(S.no_of_states, 1);
unpack_state_script;
phi_y = deg2rad(-30);
pack_state_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0);
assert_eq([0; 0; 0], F, 10);
assert_eq([0; 10600; 0], N, 200);
assert_eq(zeros(3, 1), omega_drives);

"Initially turned around the vertical axis  -> no force."
state = zeros(S.no_of_states, 1);
unpack_state_script;
phi_z = deg2rad(30);
pack_state_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0);
assert_eq([0; 0; 0], F, 5);
assert_eq([0; 0; 0], N, 5);
assert_eq(zeros(3, 1), omega_drives);

"Initially turned around the vertical axis by 180 degree -> no force."
state = zeros(S.no_of_states, 1);
unpack_state_script;
phi_z = deg2rad(180);
pack_state_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0);
assert_eq([0; 0; 0], F, 5);
assert_eq([0; 0; 0], N, 5);
assert_eq(zeros(3, 1), omega_drives);


"Initial x rotation -> opposing torque from sail, deck, hull, rudders and keel."
state = zeros(S.no_of_states, 1);
unpack_state_script;
omega_x = deg2rad(20);  % degrees per second
pack_state_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0);
assert_eq([0; 10; 0], F, 5);  % keel pushes boat to starboard
assert_eq([-71; 0; 0], N, 5);
assert_eq(zeros(3, 1), omega_drives);

"Initial y rotation -> very small opposing torque from sail, hull, rudders, bomb and keel ."
state = zeros(S.no_of_states, 1);
unpack_state_script;
omega_y = deg2rad(20);  % degrees per second
pack_state_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0);
assert_eq([0; 0; 0], F, 5);
assert_eq([0; -0.95; 0], N, 5);
assert_eq(zeros(3, 1), omega_drives);

"Initial z rotation -> opposing torque from hull and rudders."
state = zeros(S.no_of_states, 1);
unpack_state_script;
omega_z = deg2rad(20);  % degrees per second
pack_state_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0);
assert_eq([0; 33; 0], F, 5);  % rudder push to starboard.
assert_eq([4; 0; -53], N, 5);  % rudders cause heel.
assert_eq(zeros(3, 1), omega_drives);

% Sail
"Sail at 15 degrees and wind from front leads to force towards y axis plus drag into negative x direction."
state = zeros(S.no_of_states, 1);
inputs = zeros(S.no_of_inputs, 1);
unpack_state_script;
gamma_S = deg2rad(15);
pack_state_script;
unpack_inputs_script;
wind_alpha = deg2rad(180);
wind_magnitude = 5;
gamma_S_star = deg2rad(15);
pack_inputs_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0);
assert_eq([-38; 104; 0], F, 5);
assert_eq([378; 136; 67], N, 5);  % strong heel
assert_eq(zeros(3, 1), omega_drives);

% rudder
"rudders at 15 degrees and 2 m/s speed."
state = zeros(S.no_of_states, 1);
inputs = zeros(S.no_of_inputs, 1);
unpack_state_script;
v_x = 2;
gamma_R1 = deg2rad(15);
gamma_R2 = deg2rad(15);
pack_state_script;
unpack_inputs_script;
wind_alpha = deg2rad(180);
wind_magnitude = 5;
gamma_R1_star = deg2rad(15);
gamma_R2_star = deg2rad(15);
pack_inputs_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0);
assert_eq([-132; 230; 0], F, 5);  % rudder push to starboard.
assert_eq([17; 24; -320], N, 5);  % boat turns towards port side
assert_eq(zeros(3, 1), omega_drives);

% rudder drives
"rudders reference value at 15 degrees."
state = zeros(S.no_of_states, 1);
inputs = zeros(S.no_of_inputs, 1);
unpack_inputs_script;
gamma_R1_star = deg2rad(15);
gamma_R2_star = deg2rad(15);
pack_inputs_script;
% debug_print_forces(1);
[F, N, omega_drives] = forces(state, inputs, 0);
assert_eq([0; 0; 0], F);
assert_eq([0; 0; 0], N);
assert_eq([0; 0.52; 0.52], omega_drives, 0.01);













endfunction