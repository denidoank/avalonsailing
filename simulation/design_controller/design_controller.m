% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function k = design_controller()


boat_parameters = boat();

% The inertia is documented to be 100 or 150 kg*m^2, depending on the source.
% In the signal path there is the speed through the water which has
% a quadratic influence on the torque. So one can assume additional
% failures of 25%.

J_z = boat_parameters.inertia_z;
J_z_min = 80;
J_z_max = 200;


% state space model
% input torque
% state: omega_z, phi_z, integral over phi_z 

B = [1/J_z; 0; 0];
B_min = [1/J_z_min; 0; 0];
B_max = [1/J_z_max; 0; 0];
B_min_x = [1/(0.5*J_z_min); 0; 0];
B_max_x = [1/(2*J_z_max); 0; 0];

A = [ 0  0  0;
      1  0  0;
      0  1  0];
C = [1 0 0;
     0 1 0];
D = [0;
     0];

% initial guess
% omega_closed_loop
om = 2 * pi * 0.2;  % 0.2 works well
p = [  -om;
       -0.7 * om + 0.7 * om * i;
       -0.7 * om - 0.7 * om * i ];

k = acker(A, B, p);

initial_eig = eig(A);

eig_nominal = eig(A - B * k)
eig_min = eig(A - B_min * k)
eig_max = eig(A - B_max * k)


plant = ss(A, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, 20.0)

plant = ss(A-B*k, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, 20.0)

plant = ss(A-B_min*k, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, 20.0)

plant = ss(A-B_max*k, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, 20.0)

plant = ss(A-B_min_x*k, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, 20.0)

plant = ss(A-B_max_x*k, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, 20.0)

k

endfunction