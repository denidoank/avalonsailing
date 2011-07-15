% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function B = boat()
% Constants of the boat geometry, masses and inertias.
% function B = boat()

% caching
persistent cached_boat_structure;
persistent counter = 0;
if counter > 0
  B = cached_boat_structure;
  counter++;
  return
endif
counter++;


%        x Windsensor
%        |
%        |
%        |
%        |
%        X Sail
%        |
%        |
%        |
%   _____|_____
%   |    X    |
%   |         |
%    \__ X __/   <- Gravity (center of)
%       \ /
%        |
%        X Keel
%        |
%       (O) Bomb
%
%
%
%                        ______X Wind sensor
%                       /      |
%                      /       |
%                     /        | 
%                    /         |
%                   /          |
%                  /           |
%                 /            |
%                /             |
%               /              |
%              /               |
%             /                |
%            /                 | 
%           /                  |
%          /                   |
%         /                    | 
%        /         X Sail | z_S| |                              A  z
%       /                      | V                              |
%      |                       |                                |
%      |                       |                   CAD system:  -----> x
%      |                       |
%      L__________________|____|
%                         | x_axis_S
%   x_____________________|_____________
%   |              X Deck             /
% ~~|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ /~~~~~~~
%   |              X Hull           /
%   |                              /
%   |_____________________________/
%     |  |        | X | center of Gravity
%     |X |        |   |
%     |  |        | X | Keel
%     ----        |   |
%                 |   |
%    Rudder      <__X__>
%                  Bomb
% 
% indicees used: R, K, H, S, G, B, D, W
% for rudder, keel, hull, sail, center of Gravity,
% bomb, deck, wind sensor
% x : CAD reference point used in Endbericht, Anhang A.13

% For the boat we use a forward-starboard-down right-hand coordinate system xyz.

% CAD Reference point: middle of upper deck level at rear.
% The CAD reference system has a z axis that points UP. Thus all z coordinates have to be
% reversed finally.
CAD_to_xyz = [1; 1; -1];
cog = [1.747; 0; -0.756];  % Center of gravity
cog_x = cog(1);
cog_y = cog(2);
cog_z = cog(3);

% The coordinates for the centers of forces (like B.S, B.K etc.)
% all refer to the center of gravity.
% The center of gravity is the reference point of the boat coordinate system.
B.G =  [0; 0; 0];
mass_total = 538.5;   	% kg (450 was planned but 538.5 is actual)

% Keel
B.area_K = 0.528;       % m2, endbericht p. 85
B.length_K = 0.352;     % m, average length of profile chord
B.depth_K = 1.5;        % m, height from keel root to bomb
% Assume an axis at 0.25 length.
B.front_K = 0.25 * B.length_K;
B.rear_K  = 0.75 * B.length_K;
coa_K = coa_trapez_of("keel");
% point at keel axis, approx. above the middle height of the keel
B.K = [1.8+coa_K(1); 0; -0.7-coa_K(3)] - cog;

% Rudder
B.area_R = 0.085;   % m2
B.length_R = 0.165; % m
B.number_R = 2;     % we have 2 rudders, which are modelled midships.
B.front_R = 0.25 * B.length_R;
B.rear_R  = 0.75 * B.length_R;
coa_R = coa_trapez_of("rudder");
% point in the middle between both rudders.
% We model them at this place.
B.R = [0.36; 0; -0.45-coa_R(3)] - cog;
% distance between left and right rudder axis.
B.distance_lr_R = 0.7;  

% Sail
B.area_S = 8.4;    % m2 (f0_45_min_max.m)
B.length_S = 1.95; % m, average length of profile chord
B.height_S = 5;    % m
% ref. point at rear to lower leech of sail, boom thickness 0.15m
% so one could also add the boom to the sail area, and correct the height.
deck_to_leech = 0.355; % m, in z direction, 
% axis to mast nose
axis_to_mast_nose = 0.71;  
mast_diameter = 0.12;
B.front_S = axis_to_mast_nose;
B.rear_S = B.length_S - axis_to_mast_nose;
coa_S = coa_trapez_of("sail");
% COF of sail forces, COG to middle of sail)
% point *over* (thatswhy +) mast axis, in middle height of sail
B.S = [2.25; 0; deck_to_leech+coa_S(3)] - cog;

% Keel Bomb
B.length_B = 1.0;    % m
B.diameter_B = 0.25; % m 
B.c_drag_B = 0.019;  % undocumented, extrapolated from endbericht p. 86
                     % for diameter to length = 25%
B.c_drag_z_B = 0.04; %  a guess
% keel root, keel length, half bomb diameter
B.B =  [1.8+0.307/2; 0; -0.7-1.5-0.25/2] - cog;

% Drive Features
% 
% interface: angle reference value in degree
% maximum turning speed omega_max
% index R for rudder drive
% index S for sail drive
B.omega_max_S = deg2rad(180 / 13);  % rad/s, 13.8 degree per second, from Luuk; simulation_shell.m says 15 degree/s
B.omega_max_R = deg2rad(30);  % rad/s, from simulation_shell.m
B.gain_prop_S = 10;           % rad/s / rad, i.e. a control error of 1 radians
                              % causes a speed of 10 rad/s
B.gain_prop_R = 10;

% Hull, under water
B.WSA = 3.97;       % m^2, Area of the section of the hull by the waterline, s. endbericht, p. 65
B.length_H = 3.95;  % m, length of waterline 
B.width_H = 1.2;    % m, width of waterline, reference?
% Draught was 0.25m for a weight of 450kg after optimization (endbericht, p. 65)
% with the additional weight we get more draught
C = physical_constants();
% draught is the maximum depth of the hull (bottom of the hull)
draught = 0.25 + (mass_total - 450) / C.rho_water / B.WSA;
% Effect of the increased mass: 21mm, delta_z = (mass_total - 450) / C.rho_water / B.WSA
B.draught = draught;
B.H = [1.784; 0; -0.4-draught/2] - cog;  % COF endbericht p. 65    
% Hull hydrodynamics
% F_drag_x: 48.585N at 4kts, [endreport, p. 65]                                                        
% F = c_d/2 * rho_water * A * v^2
% values from report 123, p. 28 listed as comments
% A_eff=c_d*A             c_d,  A / m^2
A_eff_x=0.0222;  % m^2, 0.4  0.3  comment: This would results in a resistance force of 261N
                 % which contradicts the endbericht significantly.
A_eff_y=0.15;    % m^2, 1.2  0.8  comment: a c_d of 1.2 seems big, A is 4m * 0.25m draft
A_eff_z=B.WSA;   % m^2, 1.2  2.0  Comment: unlikely, the WSA is about 4 m^2  
% M_damp = -C * omega^2;
B.damping_rot_x = 0;  % irrelevant, keel and rudders have much more resistance
B.damping_rot_y = c_damp_rot_water(A_eff_x + A_eff_z, B.length_H);  % modeled as a plate of a certain
                                                                    % area and length in water  
B.damping_rot_z = c_damp_rot_water(A_eff_x + A_eff_y, B.length_H);
B.damping_rot_H = [B.damping_rot_x; B.damping_rot_y; B.damping_rot_z];

B.damping_trans_x = A_eff_x * C.rho_water / 2;
B.damping_trans_y = A_eff_y * C.rho_water / 2;
B.damping_trans_z = A_eff_z * C.rho_water / 2;
B.damping_trans_H = [B.damping_trans_x; B.damping_trans_y; B.damping_trans_z];

% Deck, over water
B.D = [1.784; 0; -0.2] - cog;
% TODO(grundmann): evaluate 
A_eff_x_D = B.width_H * 0.52 * 0.1;       % m^2, 0.52 height of deck with solar panels, 1.4 boom (width of boat) 
A_eff_y_D = B.length_H * 0.52 * 0.2;      % m^2
A_eff_z_D = B.length_H * B.width_H * 0.4; % m^2, if the wind blows along the z axis, then we see half of the boat only  
B.damping_trans_D = C.rho_air / 2 * [A_eff_x_D; A_eff_y_D; A_eff_z_D];

% Mass and rotational inertias
% Inertia tensor
% report 123, p. 26
inertia_tensor = [ ...
635  0   -20;
0   2427  0;
-20  0   150];
B.inertia_z = inertia_tensor(3,3);   % important for heading controller
m_m = mass_total * eye(3);
B.mass_matrix = [m_m         zeros(3, 3);
                 zeros(3, 3) inertia_tensor];
B.mass_matrix_inverted = inv(B.mass_matrix);
B.mass = mass_total;

% Wind sensor
% point at mast top for sail angle 0 
B.W = [...
  2.25+axis_to_mast_nose+mast_diameter/2;
  0; 
  deck_to_leech+B.height_S] - cog;

B.offset_rad_W = 0;   

% Derived lambda factor constants
lambda = 2 * B.area_K / (B.length_K + B.diameter_B)^2; 
B.lambda_factor_K = lambda / (lambda + 2);
lambda = 2 * B.area_R / B.length_R^2;
B.lambda_factor_R = lambda / (lambda + 2);
lambda = 2 * B.area_S / B.length_S^2;
B.lambda_factor_S = lambda / (lambda + 2);

% Finally, invert the z axis for all components
%  R, K, H, S, G, B, D, W 
B.R = B.R .* CAD_to_xyz;
B.K = B.K .* CAD_to_xyz;
B.H = B.H .* CAD_to_xyz;
B.S = B.S .* CAD_to_xyz;
B.G = B.G .* CAD_to_xyz;
B.B = B.B .* CAD_to_xyz;
B.D = B.D .* CAD_to_xyz;
B.W = B.W .* CAD_to_xyz;

% Fill the cache once
cached_boat_structure = B;
