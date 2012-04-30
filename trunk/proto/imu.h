#ifndef PROTO_IMU_H
#define PROTO_IMU_H

#include <math.h>
#include <stdint.h>

struct IMUProto {
  int64_t timestamp_ms;
  double temp_c;
  double acc_x_m_s2;
  double acc_y_m_s2;
  double acc_z_m_s2;
  double gyr_x_rad_s;
  double gyr_y_rad_s;
  double gyr_z_rad_s;
  double mag_x_au;
  double mag_y_au;
  double mag_z_au;
  double roll_deg;
  double pitch_deg;
  double yaw_deg;
  double lat_deg;
  double lng_deg;
  double alt_m;
  double vel_x_m_s;  // in boat coordinates
  double vel_y_m_s;
  double vel_z_m_s;
};

#define INIT_IMUPROTO \
  {0,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN,NAN}

// For use in printf and friends.
#define OFMT_IMUPROTO(x)					 \
  "imu: timestamp_ms:%lld temp_c:%.3lf "			 \
  "acc_x_m_s2:%.3lf acc_y_m_s2:%.3lf acc_z_m_s2:%.3lf "	 \
  "gyr_x_rad_s:%.3lf gyr_y_rad_s:%.3lf gyr_z_rad_s:%.3lf " \
  "mag_x_au:%.3lf mag_y_au:%.3lf mag_z_au:%.3lf " \
  "roll_deg:%.3lf pitch_deg:%.3lf yaw_deg:%.3lf "		 \
  "lat_deg:%.6lf lng_deg:%.6lf alt_m:%.3lf "		 \
  "vel_x_m_s:%.3lf vel_y_m_s:%.3lf vel_z_m_s:%.3lf"	 \
  "\n"							 \
  , (x).timestamp_ms, (x).temp_c			 \
  , (x).acc_x_m_s2, (x).acc_y_m_s2, (x).acc_z_m_s2	\
  , (x).gyr_x_rad_s, (x).gyr_y_rad_s, (x).gyr_z_rad_s	\
  , (x).mag_x_au, (x).mag_y_au, (x).mag_z_au	\
  , (x).roll_deg, (x).pitch_deg, (x).yaw_deg		\
  , (x).lat_deg, (x).lng_deg, (x).alt_m		\
  , (x).vel_x_m_s, (x).vel_y_m_s, (x).vel_z_m_s

#define IFMT_IMUPROTO(x, n)					 \
  "imu: timestamp_ms:%lld temp_c:%lf "			 \
  "acc_x_m_s2:%lf acc_y_m_s2:%lf acc_z_m_s2:%lf "	 \
  "gyr_x_rad_s:%lf gyr_y_rad_s:%lf gyr_z_rad_s:%lf " \
  "mag_x_au:%lf mag_y_au:%lf mag_z_au:%lf "	 \
  "roll_deg:%lf pitch_deg:%lf yaw_deg:%lf "		 \
  "lat_deg:%lf lng_deg:%lf alt_m:%lf "		 \
  "vel_x_m_s:%lf vel_y_m_s:%lf vel_z_m_s:%lf\n"	 \
  "%n"							 \
  , &(x)->timestamp_ms, &(x)->temp_c			 \
  , &(x)->acc_x_m_s2, &(x)->acc_y_m_s2, &(x)->acc_z_m_s2	\
  , &(x)->gyr_x_rad_s, &(x)->gyr_y_rad_s, &(x)->gyr_z_rad_s	\
  , &(x)->mag_x_au, &(x)->mag_y_au, &(x)->mag_z_au	\
  , &(x)->roll_deg, &(x)->pitch_deg, &(x)->yaw_deg		\
  , &(x)->lat_deg, &(x)->lng_deg, &(x)->alt_m		\
  , &(x)->vel_x_m_s, &(x)->vel_y_m_s, &(x)->vel_z_m_s	\
  , (n)

#endif  // PROTO_IMU_H
