% A log file from an Avalon sailing test on the 
% Urnersee in May 2009. Wind was between 6 and 10 knots (measured by the boat)
% in that time frame. Data format is timestamp in first column and then in the
% order of variables in the imu struct as in imu.h (attached).


function read_logs()

close all

file_name = "./imu_part.txt";

column_file_name = "./imu.h";


% IMU data, scenario is unknown, "Marine" is likely.
raw = load(file_name);
disp('samples:')
last_row_index = rows(raw)

% raw(1:10, :)
% pause

%			double speed; //in KNOTS!
%			//GPS-Data
%			struct {double longitude; double latitude; double altitude;} position;
%			//IMU-Data
%			struct {double roll; double pitch; double yaw;} attitude; // roll & pitch in rad; yaw in deg BUG in header comments!
%			struct {double x; double y; double z;} velocity; // velocity in x, y, z in knots
%			struct {double x; double y; double z;} acceleration; // acceleration in x, y, z in m/s^2
%			struct {double x; double y; double z;} gyro; // gyroscope data in x, y, z in rad/s; Also wrong sign !!; Bug in header comments !!
%			double temperature; // in deg C				

t_0 = raw(1, 1);
t = raw(:, 1) - t_0;

kts2ms = 1852 / 3600;
% want metric for positions and speed
speed = kts2ms * raw(:, 2); % to m/s

deg2rad = pi/180;
rad2deg = 180/pi;

lat_0 = raw(1, 4)
lon_0 = raw(1, 3)
lat_end = raw(last_row_index, 4)
lon_end = raw(last_row_index, 3)


["North: " deg_min_sec(lat_0)]
["East: " deg_min_sec(lon_0)]
["North_end: " deg_min_sec(lat_end)]
["East_end: " deg_min_sec(lon_end)]



lon_to_meters = 40000000 / 360 * cos(lat_0 * deg2rad);
lat_to_meters = 40000000 / 360;

% Position relative to starting point in m. 
north  = lat_to_meters * (raw(:, 4) - lat_0);
east   = lon_to_meters * (raw(:, 3) - lon_0);
z      = raw(:, 5);

north_change = lat_to_meters * (lat_end - lat_0);
east_change  = lon_to_meters * (lon_end - lon_0);

'Moved '
north_change
'm to the Nort and '
east_change
'm to the East.'

phi_x = raw(:, 6); % Really in deg
phi_y = raw(:, 7);
phi_z = raw(:, 8);

v_x = kts2ms * raw(:, 9);
v_y = kts2ms * raw(:, 10);
v_z = kts2ms * raw(:, 11);

acc_x = -raw(:, 12);
acc_y = -raw(:, 13);
acc_z = -raw(:, 14);

om_x  = -rad2deg*raw(:, 15);
om_y  = -rad2deg*raw(:, 16);
om_z  = -rad2deg*raw(:, 17);

temperature  = raw(:, 18);

disp("Time")
disp(["Test started at: ", ctime(t_0)]);
disp([num2str(size(t)), " samples"])
min_t  = min(t)
max_t  = max(t)
disp(["Measuring duration: ", num2str((max_t - min_t) / 60), " minutes"]);
disp("Sampling period")
dt = diff(t);
%plot(dt)
%title("T");
%pause
min_dt  = min(dt)
median_dt = median(dt)
mean_dt = mean(dt)
% Big spikes (3 times more than 1 s within 20000 samples, 3 times double T)
max_dt  = max(dt)

disp("Sampling frequency/Hz:")
1/median_dt

disp("Positions")
min_north  = min(north)
mean_north = mean(north)
max_north  = max(north)

min_east   = min(east)
mean_east  = mean(east)
max_east   = max(east)

min_z  = min(z)
mean_z = mean(z)
max_z  = max(z)

%plot(t, z)
%lbl("Height", "t / s", "z / m");
stddev_z = std(z);


figure()
plot(east, north)
lbl("Track", "east / m", "north / m");
axis("equal");


figure()
plot(t, speed)
lbl("Speed", "t / s", "speed / m/s");



figure()
plot(phi_z, speed)
lbl("Polar", "phi_z / deg", "speed / m/s");


figure()
plot(t, phi_x)
lbl("phi_x", "t / s", "phi_x / deg");
figure()
plot(t, phi_y)
lbl("phi_y", "t / s", "phi_y / deg");
figure()
plot(t, phi_z)
lbl("phi_z", "t / s", "phi_z / deg");


figure()
plot(t, om_x)
lbl("om_x", "t / s", "om_x / deg/s");
figure()
plot(t, om_y)
lbl("om_y", "t / s", "om_y / deg/s");
figure()
plot(t, om_z)
lbl("om_z", "t / s", "om_z / deg/s");

figure()
plot(t, acc_x)
lbl("acc_x", "t / s", "acc_x / m/s^2");
figure()
plot(t, acc_y)
lbl("acc_y", "t / s", "acc_y / m/s^2");
figure()
plot(t, acc_z)
lbl("acc_z", "t / s", "acc_z / m/s^2");



% N until 120s
% W until 650s
% N (E) until 1000s
% omega turn to west
% N until 1200s

figure()
plot(t(1:2000), phi_z(1:2000))
lbl("phi_z", "t / s", "phi_z / deg");

% N until 120s
% W until 650s
% N (E) until 1000s
% omega turn to west
% N until 1200s

turns = [0 6 112 130 640 660 970 1040 1170 max(t)-0.001];
phase = ["NE->N turn 6s";
         "About N 110s";
         "N->W turn 18s";
         "WNW 500s";
         "NW->NNE turn 20s";
         "N 200s";
         "N->W->N turn 70s";
         "NNE 40s";
         "NE->NW turn 40s"];
straight_phases = [2 4 6 8];
turn_phases = [1 3 5 7 9];
         
         

sections = [];
for i = 1: length(turns)-1
  start = min(find(t >= turns(i)));
  ende  = min(find(t > turns(i+1))) - 1;
  sections = [sections; [start ende]];
endfor

% This is used to define the turns vector (and the phases)
%for r = 1:rows(sections)
%  figure()
%  plot(t(sections(r, 1):sections(r, 2)), phi_z(sections(r, 1):sections(r, 2)))
%  lbl("phi_z", "t / s", "phi_z / deg");
%  pause
%endfor

% so to show turns
% for r = turn_phases
%   figure()
%   plot(t(sections(r, 1):sections(r, 2)), phi_z(sections(r, 1):sections(r, 2)))
%   lbl("phi_z", "t / s", "phi_z / deg");
%   pause
% endfor

if 1  % switch on for check of phi_z     versus om_z
for r = turn_phases
  r 
  disp("start at ");
  pause
  phi_z(sections(r, 1))
  figure()
  plot(t(sections(r, 1):sections(r, 2)), phi_z(sections(r, 1):sections(r, 2)), "g");
  lbl("phi_z", "t / s", "phi_z / deg");
  
  pause
  figure()
  plot(t(sections(r, 1):sections(r, 2)), om_z(sections(r, 1):sections(r, 2)),  "k");
  lbl("om_z", "t / s", "om_z / deg/s");
  
  int_om_z = integ2(dt(sections(r, 1):(sections(r, 2)-1)), ...
                    om_z(sections(r, 1):(sections(r, 2)-1)), ...
                    phi_z(sections(r, 1)));
  pause
  figure()
  plot(t(sections(r, 1):sections(r, 2)), int_om_z,                             "r");
  lbl("int_{omz}", "t / s", "int_{omz} / deg");
  pause
 
 
  figure()
  plot(t(sections(r, 1):sections(r, 2)), phi_z(sections(r, 1):sections(r, 2)), "g",
       t(sections(r, 1):sections(r, 2)), om_z(sections(r, 1):sections(r, 2)),  "k",
       t(sections(r, 1):sections(r, 2)), int_om_z, "r");
  lbl("phi_z", "t / s", "phi_z / deg, om_z / deg/s");
  pause
endfor
endif


all_phi_z = [];
all_speeds = [];

if 1  % switch on for speed versus direction checks
for r = straight_phases
  average_over = 100;
  phi_z_chunks = chunk(phi_z(sections(r, 1):sections(r, 2)), average_over);
  speed_chunks = chunk(speed(sections(r, 1):sections(r, 2)), average_over);
  figure()
  plot(phi_z_chunks, speed_chunks, "*b");
  lbl("speed = f(phi__z) averaged 100 samples", "phi_z / deg", "speed / m/s");
  all_phi_z = [all_phi_z; phi_z_chunks];
  all_speeds = [all_speeds; speed_chunks];  
endfor
figure()
plot(all_phi_z, all_speeds, "*b");
lbl("speed = f(phi__z) averaged 100 samples", "phi_z / deg", "speed / m/s");
pause

endif


if 1  % switch on for drift checks
for r = straight_phases
  average_over = 100;
  phi_z_chunks = chunk(phi_z(sections(r, 1):sections(r, 2)), average_over);
  speed_chunks = chunk(speed(sections(r, 1):sections(r, 2)), average_over);
  north_chunks = chunk(north(sections(r, 1):sections(r, 2)), average_over);
  east_chunks  = chunk(east(sections(r, 1):sections(r, 2)), average_over);
  
  north_chunks -= north_chunks(1);
  east_chunks  -= east_chunks(1);
  
  dn = cos(deg2rad * phi_z_chunks) .* speed_chunks * average_over * median_dt;
  de = sin(deg2rad * phi_z_chunks) .* speed_chunks * average_over * median_dt;
  rec_north = cumsum(dn);
  rec_east = cumsum(de);
  
  figure()
  plot(rec_east, rec_north, "*b", east_chunks, north_chunks, "+g");
  lbl("Mini-tracks (100 samples per step) GPS:green, phi_z: blue", "East / m", "North / m");
  axis("equal");
  
  x_app = rec_north(length(rec_north)) - rec_north(1);
  y_app = rec_east(length(rec_east)) - rec_east(1);
  phi_app = rad2deg * atan2(y_app, x_app)

  x_true = north_chunks(length(north_chunks)) - north_chunks(1);
  y_true = east_chunks(length(east_chunks)) - east_chunks(1);
  phi_true = rad2deg * atan2(y_true, x_true)
  drift_deg = abs(phi_app - phi_true)
  pause
endfor

endif








if 0  % switch on to check heeling angle and acceleration data
pause
phi_x_from_acc = -rad2deg * atan2(-acc_y, acc_z);

figure()
plot(t, phi_x, "g", t, phi_x_from_acc, "r")
lbl("phi_x vs. acc-derived phy_y", "t / s", "phi_y / deg");
pause
endif

% speeds
% speed = sqrt(v_x² + v_y²) in SW

figure()
plot(t, speed)
lbl("speed", "t / s", "speed / m/s");

figure()
plot(t, v_x)
lbl("speed x", "t / s", "speed / m/s");

figure()
plot(t, phi_x, "g", t, 10*v_y, "r")
lbl("phi_x (green) vs. 10*v_y (red)", "t / s", "phi_y / deg, v_y / 0.1*m/s");
pause



endfunction







