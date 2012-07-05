% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function [F, N, omega_drives, wind_W] = forces(state, inputs, t)
% function [F, N, omega_drives] = forces(state, inputs, t) 
% Forces, torques and drive speeds for the current state, inputs and time.
% state: the current state column vector (for definition see below)
% inputs: the current inputs column vector (for definition see below
% t: the current time (as usual in s)
% Return values
% F: Force vector (definition see below)
% N: Torque vector (definition see below)
% omega_drives: current drive speeds vector (definition see below)
% wind_W: the wind at the wind sensor, in 3 dimensions (we measure x and y only)  
%
% position and angles of the boats body coordinate system
% state = [... 
% x;
% y;
% z;
% phi_x;
% phi_y;
% phi_z;
% v_x;
% v_y;
% v_z;
% omega_x;
% omega_y;
% omega_z;
% gamma_S;
% gamma_R1;
% gamma_R2];
%
% inputs=[ ...
% wind_alpha;
% wind_magnitude;
% stream_alpha;
% stream_magnitude;
% z_0;         % z_0 for waves
% gamma_S_star;
% gamma_R1_star;
% gamma_R2_star];
%
% Return values
% F =[ ...
% F_x;
% F_y;
% F_z;
% N = [ ... 
% N_x;
% N_y;
% N_z];
% omega_drives = [ ...
% omega_S;
% omega_R1;
% omega_R2];

S = simulation_options();
assert(size(state) == [S.no_of_states, 1]);
assert(size(inputs) == [S.no_of_inputs, 1]);

unpack_state_script;
unpack_inputs_script;
% all elements from the state and inputs vector are scalar variables now.

B=boat();
C=physical_constants();
global debug_switches;

% Index E indicates the global reference system.
wind_E = [...
  wind_magnitude * cos(wind_alpha);
  wind_magnitude * sin(wind_alpha);
  0];
stream_E = [...
  stream_magnitude * cos(stream_alpha);
  stream_magnitude * sin(stream_alpha);
  0];
gravitation_E = [ ...
  0
  0
  C.gravity_acceleration];

earth_to_boat = rotation_matrix(phi_z, phi_y, phi_x);

wind_boat = earth_to_boat * wind_E;
stream_boat = earth_to_boat * stream_E;
gravitation_boat = earth_to_boat * gravitation_E;

%  
position = [x; y; z];
speed = [v_x; v_y; v_z];
phi = [phi_x; phi_y; phi_z];
omega = [omega_x; omega_y; omega_z];
gamma_drives_star = [gamma_S_star; gamma_R1_star; gamma_R2_star];
gamma_actual = [gamma_S; gamma_R1; gamma_R2];

% Hydrostatic buoyancy forces and torques
% =======================================
% Buoancy-gravity force equlibrium
% z_E is the z coordinate in earth coordinates

z_E = z * cos(phi_y) * cos(phi_x) - z_0;

force_G = [0;
           0;
           B.WSA * -z_E * C.rho_water * C.gravity_acceleration];
% This is not totally correct.
% I need the boat to earth transformation here!

% Righting torques around x and y axis.
% Can we take phi_x directly?
phi_x_grav = atan2(gravitation_boat(2), gravitation_boat(3));
phi_y_grav = atan2(gravitation_boat(1), gravitation_boat(3));

N_RF_x = gz_x(phi_x_grav) * B.mass * C.gravity_acceleration * debug_switches.G;
N_RF_y = -1 * gz_y(phi_y_grav) * B.mass * C.gravity_acceleration * debug_switches.G;
N_G = [ N_RF_x;
        N_RF_y;
        0];
if debug_switches.print_forces
  force_G
  N_G
endif


% Dynamic forces from wind and stream
% ===================================
% Over water components
%=======================
% Sail
wind_S = wind_boat - (speed + tangential_speed(B.S, omega));
[aoa_S, magnitude_S] = angle_of_attack(wind_S, gamma_S);
[force_S, pos_S] = force_of("sail", aoa_S, magnitude_S, gamma_S);

if debug_switches.print_forces
  force_S
endif

% relative wind at Wind sensor
wind_W = wind_boat - (speed + tangential_speed(B.W, omega));

% Deck
wind_D = wind_boat - (speed + tangential_speed(B.D, omega));
force_D = force_deck(wind_D);
pos_D = B.D;
if debug_switches.print_forces
  force_D
endif

% Under water components
%========================

% Rudders
stream_R = stream_boat - (speed + tangential_speed(B.R, omega));
[aoa_R1, magnitude_R1] = angle_of_attack(stream_R, gamma_R1);
[force_R1, pos_R1] = force_of("rudder", aoa_R1, magnitude_R1, gamma_R1);
if gamma_R2 != gamma_R1
  [aoa_R2, magnitude_R2] = angle_of_attack(stream_R, gamma_R2);
  [force_R2, pos_R2] = force_of("rudder", aoa_R2, magnitude_R2, gamma_R2);
else
  force_R2 = force_R1;
  pos_R2 = pos_R1;
endif
if debug_switches.print_forces
  force_R1
  force_R2
endif

% Hull
stream_H = stream_boat - (speed + tangential_speed(B.H, omega));
force_H = force_hull(stream_H);
pos_H = B.H;
if debug_switches.print_forces
  force_H
endif

% Keel
stream_K = stream_boat - (speed + tangential_speed(B.K, omega));
[aoa_K, magnitude_K] = angle_of_attack(stream_K, 0);
[force_K, pos_K] = force_of("keel", aoa_K, magnitude_K, 0);   % keel cannot be turned
if debug_switches.print_forces
  force_K
endif

% keel Bomb
stream_B = stream_boat - (speed + tangential_speed(B.B, omega));
force_B = force_bomb(stream_B);
pos_B = B.B;
if debug_switches.print_forces
  force_B
endif

F = ...
  force_S  * debug_switches.S + ...
  force_D  * debug_switches.D + ...
  force_R1 * debug_switches.R + ...
  force_R2 * debug_switches.R + ...
  force_H  * debug_switches.H + ...
  force_K  * debug_switches.K + ...
  force_B  * debug_switches.B + ...
  force_G * debug_switches.G;

N = ...
  cross(pos_S,  force_S)  * debug_switches.S + ...
  cross(pos_D,  force_D)  * debug_switches.D + ...
  cross(pos_R1, force_R1) * debug_switches.R + ...
  cross(pos_R2, force_R2) * debug_switches.R + ...
  cross(pos_H,  force_H)  * debug_switches.H + ...
  cross(pos_K,  force_K)  * debug_switches.K + ...
  cross(pos_B,  force_B)  * debug_switches.B + ...
  N_G * debug_switches.G;

omega_drives = drives(gamma_drives_star, gamma_actual);
