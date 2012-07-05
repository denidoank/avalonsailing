% Copyright 2011 The Avalon Project Authors. All rights reserved.
% Use of this source code is governed by the Apache License 2.0
% that can be found in the LICENSE file.
% Steffen Grundmann, April 2011

function k = design_controller()


boat_parameters = boat();

% The inertia is documented to be 100 or 150 kg*m^2, depending on the source.
% In the signal path there is the speed through the water which has
% a quadratic influence on the torque. So one can assume significant additional
% model failures (perturbatons).
% But, for very low speeds we loose controllability anyway and on the
% other extreme the magnitude of the speed through the water is limited.

% The slow response has the advantage of suppressing wave effects (sub-Hertz).

J_z = boat_parameters.inertia_z;
J_z_min = 80;
J_z_max = 200;

% In the following the suffixes min and max always mean min and max
% of the inertia J and not of the derives values.

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
% Variant A: om = 0.2 * 2pi, one real pole at -om, imaginary poles -0.7+-0.7i om
% Variant B: om = 0.1 * 2pi, one real pole at -om, imaginary poles -0.7+-0.7i om
% Variant C: om = 0.1 * 2pi, one real pole at -om, imaginary poles -0.7+-0.2i om
% Variant D: om = 0.1 * 2pi, one real pole at -0.5om, imaginary poles -0.7+-0.2i om

% omega_closed_loop
om = 2 * pi * 0.2;  % 0.2 works well, 0.2, 
p = [  -0.7*om;
       -0.7 * om + 0.7 * om * i;
       -0.7 * om - 0.7 * om * i ];

k = acker(A, B, p);
k = real(k);

initial_eig = eig(A);

eig_nominal = eig(A - B * k)
eig_min = eig(A - B_min * k)
eig_max = eig(A - B_max * k)

t_end = 20;  % 20, 40, 80

plant = ss(A, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, t_end)
title("Plant, input torque, outputs omega_z and phi_z")


plant = ss(A-B*k, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, t_end)
title(["J at nominal ", num2str(J_z), "kgm^2"]);


plant = ss(A-B_min*k, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, t_end)
title(["J at ", num2str(J_z_min), "kgm^2"]);

plant = ss(A-B_max*k, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, t_end)
title(["J at ", num2str(J_z_max), "kgm^2"]);

plant = ss(A-B_min_x*k, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, t_end)
title(["J at ", num2str(J_z_min*0.5), "kgm^2"]);

plant = ss(A-B_max_x*k, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, t_end)
title(["J at ", num2str(J_z_max*2.0), "kgm^2"]);

k = real(k)


k_disturbed = k + [3 -2 -3 ]
plant = ss(A-B_min*k_disturbed, B, C, D, 0, 3, 0, ["omega_z"; "phi_z"; "int_phi_z"], ["torque_z"], ["omega_z"; "phi_z"], [])
figure
step(plant, 1, t_end)
title(["J at ", num2str(J_z_min), "kgm^2"]);


endfunction