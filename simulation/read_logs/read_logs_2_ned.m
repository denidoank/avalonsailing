% A log file from an IMU test.

% Data format is the IMU proto, i.e.
% imu: timestamp_ms:1325319650067 temp_c:26.562 acc_x_m_s2:-2.318 acc_y_m_s2:-0.176 acc_z_m_s2:-9.546 gyr_x_rad_s:0.012 gyr_y_rad_s:-0.015 gyr_z_rad_s:0.009 mag_x_au:0.721 mag_y_au:0.844 mag_z_au:0.889 roll_deg:2.341 pitch_deg:-14.116 yaw_deg:-65.028 lat_deg:47.4172363 lng_deg:8.5314150 alt_m:490.813 vel_x_m_s:3.281 vel_y_m_s:-0.434 vel_z_m_s:0.442
% Measurement walk around the runtrack of the school on the other side of the Sihl.
% Each measurement was takes from afull, counterclockwise walk around the outer fence of the runway.

% data preparation with
%  wc -l imulog_garden.txt
%  cat runtrack-nwu.log |  tr ':' ' ' | awk '{ print $3,$5,$7,$9,$11,$13,$15,$17,$19,$21,$23,$25,$27,$29,$31,$33,$35,$37,$39,$41}' > runtrack_nwu_clean.txt
%  Discarded first 130 lines without timestamp
%  tail -50819 runtrack-ned.log > runtrack-ned-time.log
%  cat runtrack-ned-time.log |  tr ':' ' ' | awk '{ print $3,$5,$7,$9,$11,$13,$15,$17,$19,$21,$23,$25,$27,$29,$31,$33,$35,$37,$39,$41}' > runtrack_ned_clean.txt


function read_logs_2_ned()

close all

%file_name = "runtrack_nwu_clean.txt"; Wrong orientation!
file_name = "runtrack_ned_clean.txt";

% IMU data, scenario is "Marine". Coordinate setup is NED North-East-Down, angles are Euler angles.
raw = load(file_name);
disp('samples:')
last_row_index = rows(raw)

% IMU proto format NED (North East Down coordinate system)
%  1 timestamp_ms:1325254779647
%  2 temp_c:14.250
%  3 acc_x_m_s2:0.283
%  4 acc_y_m_s2:-1.377
%  5 acc_z_m_s2:-9.754
%  6 gyr_x_rad_s:-0.003
%  7 gyr_y_rad_s:-0.022
%  8 gyr_z_rad_s:0.013
%  9 mag_x_au:0.160
% 10 mag_y_au:-0.185
% 11 mag_z_au:0.941
% 12 roll_deg:8.993
% 13 pitch_deg:2.330
% 14 yaw_deg:60.499
% 15 lat_deg:47.365337
% 16 lng_deg:8.524779
% 17 alt_m:525.667
% 18 vel_x_m_s:0.059
% 19 vel_y_m_s:0.070
% 20 vel_z_m_s:0.468

% imu: timestamp_ms:1325605058787
% temp_c:29.125
%  acc_x_m_s2:-9.756 acc_y_m_s2:1.197 acc_z_m_s2:-0.747
%  gyr_x_rad_s:0.009 gyr_y_rad_s:-0.005 gyr_z_rad_s:0.008
%  mag_x_au:0.450 mag_y_au:-0.331 mag_z_au:0.142
%  roll_deg:-67.825 pitch_deg:-81.790 yaw_deg:-176.559
%  lat_deg:47.3667068 lng_deg:8.5235996 alt_m:454.550
%  vel_x_m_s:0.197 vel_y_m_s:0.568 vel_z_m_s:3.146

% t_0 is the absolute time in s, t is in s, relative to the first sample.
t_0 = raw(1, 1) / 1000.0;
t = (raw(:, 1) - raw(1, 1)) / 1000.0;

deg2rad = pi/180;
rad2deg = 180/pi;

lat_0 = raw(1, 15)
lon_0 = raw(1, 16)
lat_end = raw(last_row_index, 15)
lon_end = raw(last_row_index, 16)

["North: " deg_min_sec(lat_0)]
["East: " deg_min_sec(lon_0)]
["North_end: " deg_min_sec(lat_end)]
["East_end: " deg_min_sec(lon_end)]

lon_to_meters = 40000000 / 360 * cos(lat_0 * deg2rad);
lat_to_meters = 40000000 / 360;

% Proper names
% Position relative to starting point in m.
north  = lat_to_meters * (raw(:, 15) - lat_0);
east   = lon_to_meters * (raw(:, 16) - lon_0);
z      = raw(:, 17);

north_change = max(north) - min(north);
east_change  = max(east)  - min(east);

['Moved in ranges of ', num2str(north_change), 'm South-North and ', ...
 num2str(east_change), 'm West-East.']

phi_x = raw(:, 12); % in deg
phi_y = raw(:, 13);
phi_z = raw(:, 14);

v_x = raw(:, 18);
v_y = raw(:, 19);
v_z = raw(:, 20);

acc_x = raw(:, 3);
acc_y = raw(:, 4);
acc_z = raw(:, 5);

om_x  = rad2deg*raw(:, 6);
om_y  = rad2deg*raw(:, 7);
om_z  = rad2deg*raw(:, 8);

mag_x = raw(:, 9);
mag_y = raw(:, 10);
mag_z = raw(:, 11);

temperature  = raw(:, 2);


disp("Time")
disp(["Test started at: ", ctime(t_0)]);
disp([num2str(size(t)), " samples"])
min_t  = min(t)
max_t  = max(t)
disp(["Measuring duration: ", num2str((max_t - min_t) / 60), " minutes"]);
disp("Sampling period")
dt = diff(t);
min_dt  = min(dt)
median_dt = median(dt)
mean_dt = mean(dt)
max_dt  = max(dt)
if max_dt > min_dt
  plot(dt)
  title("T");
  pause
endif

if median_dt > 0
  disp("Sampling frequency/Hz:")
  1/median_dt
  T = median_dt;
else
  T=0.001
  disp "Assume T=10ms"
  t = T * 0:(length(t) - 1);
endif

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



figure()
plot(east, north)
lbl("Track", "east / m", "north / m");
axis("equal");

%figure()
%plot(t, speed)
%lbl("Speed magnitude", "t / s", "speed / m/s");
%pause

if 0
  figure()
  plot(t, phi_x)
  lbl("phi_x", "t / s", "phi_x / deg");
  figure()
  plot(t, phi_y)
  lbl("phi_y", "t / s", "phi_y / deg");
  figure()
  plot(t, phi_z)
  lbl("phi_z", "t / s", "phi_z / deg");
  pause
  % sliding mean over
  k_samples = 4.0 / T;
  phi_x_f = filter(1/k_samples*ones(1, k_samples), 1, phi_x);
  phi_y_f = filter(1/k_samples*ones(1, k_samples), 1, phi_y);
  phi_z_f = filter(1/k_samples*ones(1, k_samples), 1, phi_z);
  figure()
  plot(t, phi_x_f)
  lbl("phi_x filt.", "t / s", "phi_x_f / deg");
  figure()
  plot(t, phi_y_f)
  lbl("phi_y filt.", "t / s", "phi_y_f / deg");
  figure()
  plot(t, phi_z_f)
  lbl("phi_z filt.", "t / s", "phi_z_f / deg");
endif

if 1
  figure()
  plot(t, om_x)
  lbl("om_x", "t / s", "om_x / deg/s");
  figure()
  plot(t, om_y)
  lbl("om_y", "t / s", "om_y / deg/s");
  figure()
  plot(t, om_z)
  lbl("om_z", "t / s", "om_z / deg/s");
  % sliding mean over
  k_samples = 8.0 / T;
  om_x_f = filter(1/k_samples*ones(1, k_samples), 1, om_x);
  om_y_f = filter(1/k_samples*ones(1, k_samples), 1, om_y);
  om_z_f = filter(1/k_samples*ones(1, k_samples), 1, om_z);
  figure()
  plot(t, om_x_f)
  lbl("om_x filt.", "t / s", "om_x_f / deg/s");
  figure()
  plot(t, om_y_f)
  lbl("om_y filt.", "t / s", "om_y_f / deg/s");
  figure()
  plot(t, om_z_f)
  lbl("om_z filt.", "t / s", "om_z_f / deg/s");
endif

if 1   % Acceleration filtering
  figure()
  plot(t, acc_x)
  lbl("acc_x", "t / s", "acc_x / m/s^2");
  figure()
  plot(t, acc_y)
  lbl("acc_y", "t / s", "acc_y / m/s^2");
  figure()
  plot(t, acc_z)
  lbl("acc_z", "t / s", "acc_z / m/s^2");
  % sliding mean over 1s
  k_samples = 5.0 / T;
  acc_x_f = filter(1/k_samples*ones(1, k_samples), 1, acc_x);
  acc_y_f = filter(1/k_samples*ones(1, k_samples), 1, acc_y);
  acc_z_f = filter(1/k_samples*ones(1, k_samples), 1, acc_z);
  figure()
  plot(t, acc_x_f)
  lbl("acc_x filt.", "t / s", "acc_x_f / m/s^2");
  figure()
  plot(t, acc_y_f)
  lbl("acc_y filt.", "t / s", "acc_y_f / m/s^2");
  figure()
  plot(t, acc_z_f)
  lbl("acc_z filt.", "t / s", "acc_z_f / m/s^2");
endif


% N until 120s
% W until 650s
% N (E) until 1000s
% omega turn to west
% N until 1200s

figure()
plot(t(1:length(t)), phi_z(1:length(t)))
lbl("phi_z", "t / s", "phi_z / deg");

% no phases for imu logs so far

%turns = [0 6 112 130 640 660 970 1040 1170 max(t)-0.001];
%phase = ["NE->N turn 6s";
%         "About N 110s";
%         "N->W turn 18s";
%         "WNW 500s";
%         "NW->NNE turn 20s";
%         "N 200s";
%         "N->W->N turn 70s";
%         "NNE 40s";
%         "NE->NW turn 40s"];
%straight_phases = [2 4 6 8];
%turn_phases = [1 3 5 7 9];
         
turns = [0  max(t)-0.001];
phase = ["Rest"];
straight_phases = [1];
turn_phases = [1];


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

if 1  % switch on for check of phi_z versus om_z
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



if 0  % switch on for speed versus direction checks, Polar diagram measurements
  all_phi_z = [];
  all_speeds = [];
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

if 1
  % speed, x-component of the boat speed over ground, projected
  % on to the local tangent plane.
  v_b_x =  v_x .* cos(phi_z * deg2rad) + v_y .* sin(phi_z * deg2rad);
  % y component
  v_b_y = -v_x .* sin(phi_z * deg2rad) + v_y .* cos(phi_z * deg2rad);

  % check correctness
  mv = v_x .^ 2 + v_y .^ 2;
  mv = sqrt(mv);
  mv_b = v_b_x .^ 2 + v_b_y .^ 2;
  mv_b = sqrt(mv_b);
  %figure()
  %plot(t, mv, "b", t, mv_b, "g", t, mv ./ mv_b, "r")
  %lbl("Is this equal blue in green out", "t / s", "v_b_x / m/s");
  %pause




  % sliding mean over 60s
  k_samples = 60 / T;

  % ATTENTION: Cheating factor of 0.8 !!!
  v_b_x_60s = 0.8 * filter(1/k_samples*ones(1, k_samples), 1, v_b_x);
  v_b_y_60s = 0.8 * filter(1/k_samples*ones(1, k_samples), 1, v_b_y);

  clip_at = 2.8;  % clip the magnitude over this
  [v_b_x_clipped, v_b_y_clipped] = clip_mag(v_b_x_60s, v_b_y_60s, clip_at);

  figure()
  plot(t, v_b_x)
  lbl("v_b_x", "t / s", "v_b_x / m/s");
  pause

  figure()
  plot(t, v_b_x_60s, "g", t, v_b_x_clipped, "r");
  lbl("v_b_x SM 60s filtered (green), clipped red", "t / s", "v_b_x / m/s");
  pause

  figure()
  plot(t, v_b_y)
  lbl("v_b_y", "t / s", "v_b_y / m/s");
  pause

  figure()
  plot(t, v_b_y_60s, "g", t, v_b_y_clipped, "r")
  lbl("v_b_y SM 60s filtered (green), clipped red", "t / s", "v_b_y / m/s");
  pause
  figure()
  speed_b = sqrt(v_b_x_60s .^ 2 + v_b_y_60s .^ 2);
  speed_b_clipped = sqrt(v_b_x_clipped .^ 2 + v_b_y_clipped .^ 2);
  plot(t, speed_b, "g", t, speed_b_clipped, "r");
  lbl("boat speed magnitude SM 60s filtered (green), clipped (red)", "t / s", "v_b / m/s");
  pause

endif


if 1  % switch on for checks
for r = straight_phases
  average_over = 100;
  phi_z_chunks = chunk(phi_z(sections(r, 1):sections(r, 2)), average_over);
  speed_chunks = chunk(speed_b_clipped(sections(r, 1):sections(r, 2)), average_over);
  north_chunks = chunk(north(sections(r, 1):sections(r, 2)), average_over);
  east_chunks  = chunk(east(sections(r, 1):sections(r, 2)), average_over);
  
  north_chunks -= north_chunks(1);
  east_chunks  -= east_chunks(1);
  
  dn = cos(deg2rad * phi_z_chunks) .* speed_chunks * average_over * median_dt;
  de = sin(deg2rad * phi_z_chunks) .* speed_chunks * average_over * median_dt;
  rec_north = cumsum(dn);
  rec_east = cumsum(de);
  
  figure()
  plot(rec_east, rec_north, "b", east_chunks, north_chunks, "g");
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







% heeling
if 0  % switch on to check heeling angle and acceleration data
  pause
  phi_x_from_acc = -rad2deg * atan2(-acc_y, acc_z);

  figure()
  plot(t, phi_x, "g", t, phi_x_from_acc, "r")
  lbl("phi_x vs. acc-derived phy_y", "t / s", "phi_y / deg");
  pause
endif

% speeds
if 1
  figure()
  plot(t, v_x)
  lbl("speed x North", "t / s", "speed / m/s");
  pause

  figure()
  plot(t, v_y)
  lbl("speed y East", "t / s", "speed / m/s");
  pause

  figure()
  plot(t, v_z)
  lbl("speed z Down", "t / s", "speed / m/s");
  pause
  % sliding mean over
  k_samples = 60.0 / T;
  v_x_f = filter(1/k_samples*ones(1, k_samples), 1, v_x);
  v_y_f = filter(1/k_samples*ones(1, k_samples), 1, v_y);
  v_z_f = filter(1/k_samples*ones(1, k_samples), 1, v_z);
  figure()
  plot(t, v_x_f)
  lbl("v_x North filt.", "t / s", "v_x_f / m/s");
  figure()
  plot(t, v_y_f)
  lbl("v_y East filt.", "t / s", "v_y_f / m/s");
  figure()
  plot(t, v_z_f)
  lbl("v_z Down filt.", "t / s", "v_z_f / m/s");


  pos_x = T * cumsum(v_x_f);
  pos_y = T * cumsum(v_y_f);
  figure()
  plot(pos_y, pos_x)
  lbl("track_x_y filt.", "y / m", "x / m");
  axis("equal");
  pause
endif



% Height
if 0
  plot(t, z)
  lbl("Height", "t / s", "z / m");
  stddev_z = std(z);
  pause

  figure();
  plot(t(1:600), v_z(1:600))
  lbl("speed z", "t / s", "speed / m/s");
  pause

  figure();
  plot(t(1:600), z(1:600))
  lbl("Height", "t / s", "z / m");
  stddev_z = std(z)
  pause
endif


% Magnetics
if 1
  % sliding mean over
  k_samples = 1.0 / T;
  mag_x_f = filter(1/k_samples*ones(1, k_samples), 1, mag_x);
  mag_y_f = filter(1/k_samples*ones(1, k_samples), 1, mag_y);
  mag_z_f = filter(1/k_samples*ones(1, k_samples), 1, mag_z);
  figure()
  plot(t, mag_x_f)
  lbl("mag_x filt.", "t / s", "mag_x_f / au");
  figure()
  plot(t, mag_y_f)
  lbl("mag_y filt.", "t / s", "mag_y_f / au");
  figure()
  plot(t, mag_z_f)
  lbl("mag_z filt.", "t / s", "mag_z_f / au");
  pause
  figure();
  plot(mag_x_f, mag_y_f);
  lbl("maxY=f(mag_x)", "mag_x / au", "mag_y / au");
  axis("equal");
  figure();
  plot(mag_y_f, mag_z_f);
  lbl("mag_z=f(mag_y)", "mag_y / au", "mag_z / au");
  axis("equal");
  figure();
  plot(mag_z_f, mag_x_f);
  lbl("mag_x=f(mag_z)", "mag_z / au", "mag_x / au");
  axis("equal");

  figure();
  plot(t, rad2deg * -atan2(mag_y_f, mag_x_f), "b", t, phi_z, "g");
  lbl("maxY=f(mag_x)", "phi_{z mag} (blue), phi_z (green) / deg", "t / s");
  pause
endif


endfunction
