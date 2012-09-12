// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

// Pretend to be the IMU daemon, the rudder demon and the wind sensor demon.
// Usage: plug /var/run/linebus fakeboat

#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <list>


#include "common/check.h"
#include "common/convert.h"
#include "common/normalize.h"
#include "common/now.h"
#include "proto/meteo.h"
#include "../proto/rudder.h"
#include "../proto/imu.h"
#include "../proto/wind.h"

#include "helmsman/controller_io.h"
#include "helmsman/boat_model.h"
#include "helmsman/sampling_period.h"
#include "vskipper/util.h"

using std::string;

void SimulateShips(double latitude_deg, double longitude_deg);


static const char* argv0;
static int verbose = 0;
static int debug = 0;

static void
crash(const char* fmt, ...) {
  va_list ap;
  char buf[1000];
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  if (debug)
    fprintf(stderr, "%s:%s%s%s\n", argv0, buf,
      (errno) ? ": " : "",
      (errno) ? strerror(errno):"" );
  else
    syslog(LOG_CRIT, "%s%s%s\n", buf,
           (errno) ? ": " : "",
           (errno) ? strerror(errno):"" );
  exit(1);
  va_end(ap);
  return;
}

static void fault(int dum) { crash("fault"); }

static void
usage(void) {
  fprintf(stderr, "usage: [plug /path/to/linebus] %s\n", argv0);
  exit(1);
}


int main(int argc, char* argv[]) {
  int ch;
  argv0 = strrchr(argv[0], '/');
  if (argv0) ++argv0; else argv0 = argv[0];

  while ((ch = getopt(argc, argv, "dhv")) != -1){
    switch (ch) {
    case 'd': ++debug; break;
    case 'v': ++verbose; break;
    case 'h':
    default:
            usage();
    }
  }

  argv += optind;
  argc -= optind;
  if (argc != 0) usage();
  setlinebuf(stdout);
  if (!debug) openlog(argv0, LOG_PERROR, LOG_LOCAL0);
  if (signal(SIGBUS, fault) == SIG_ERR)  crash("signal(SIGBUS)");
  if (signal(SIGSEGV, fault) == SIG_ERR)  crash("signal(SIGSEGV)");

  int64_t last = now_ms();
  int rounds = 0;

  // Model
  struct MeteoProto  meteo_info = INIT_METEOPROTO;         // in
  struct RudderProto drives_reference = INIT_RUDDERPROTO;  // in
  meteo_info.true_wind_deg = 0.0;  // Wind from North.
  meteo_info.true_wind_speed_kt = MeterPerSecondToKnots(10.0);  // 10 m/s.
  meteo_info.turbulence = 0.0;  // Stable conditions.
  ControllerInput controller_input;
  controller_input.alpha_star_rad = Deg2Rad(90);  // go East!
  DriveReferenceValuesRad drives_ref;
  // Drive status transport delay line.
  struct RudderProto prev_status_drives  = INIT_RUDDERPROTO;      // out
  struct RudderProto prev_prev_status_drives  = INIT_RUDDERPROTO; // out
  // Wind measurement transport delay.
  struct WindProto prev_wind_sensor = INIT_WINDPROTO;      // out


  BoatModel model(kSamplingPeriod,
                  0,                // omega_ / rad, turning rate, + turns right
                  0,                // phi_z_ / rad, heading relative to North, + turns right
                  0,                // v_x_ / m/s,   speed
                  -M_PI / 2,        // gamma_sail_ / rad, e.g. -90 degrees here
                  0.03,             // initial rudder angles, left and right,
                  -0.03);           // can be used to cause initial rotation.
  // model.SetLatLon(48.23900, -4.7698);  // Brest, France
  model.SetLatLon(47.3482, 8.5434);  // Lake Zurich, middle of the lake between Chinagarden and Mythenquai.

  while (!feof(stdin)) {
    if (ferror(stdin)) clearerr(stdin);
    struct timespec timeout = { 0, 1E8 }; // wake up at least 10/second
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fileno(stdin), &rfds);

    sigset_t empty_mask;
    sigemptyset(&empty_mask);
    int r = pselect(fileno(stdin) + 1, &rfds, NULL, NULL, &timeout, &empty_mask);
    if (r == -1 && errno != EINTR) crash("pselect");

    if (debug > 2) fprintf(stderr, "Woke up %d\n", r);

    if (FD_ISSET(fileno(stdin), &rfds)) {
      char line[1024];
      if (!fgets(line, sizeof line, stdin))
        crash("Reading input");

      if (debug) fprintf(stderr, "Got line:%s\n", line);

      RudderProto drives_reference_input = INIT_RUDDERPROTO;
      MeteoProto meteo_input = INIT_METEOPROTO;
      int nn = 0;
      char* line_ptr = line;

      while (strlen(line_ptr) > 0) {
        if (sscanf(line_ptr,
                   IFMT_RUDDERPROTO_CTL(&drives_reference_input, &nn)) > 0) {
          drives_reference = drives_reference_input;
        } else if (sscanf(line_ptr, IFMT_METEOPROTO(&meteo_input, &nn)) > 0) {
          meteo_info = meteo_input;
        } else {
          // Ignore input for other recipients.
          // fprintf(stderr, "Unreadable input: \"%s\"\n", line_ptr);
          nn = strlen(line_ptr);
        }
        line_ptr += nn;
      }
    }

    int64_t now = now_ms();
    if (now < last + 100) continue;
    // The rest of the loop is executed once per 100 ms.
    last = now;

    // Meteo simulation
    double wind_deg = meteo_info.true_wind_deg;
    double wind_speed_kt = meteo_info.true_wind_speed_kt;
    if (meteo_info.turbulence > 0) {  // Simple turbulent model.
      double turbulence =
          meteo_info.turbulence * (rand() * 1.0 / RAND_MAX - 0.5);
      wind_deg += turbulence;
      wind_speed_kt += turbulence;
      if (wind_speed_kt < 0.0)
        wind_speed_kt = 0.0;
    }
    Polar true_wind = Polar(Deg2Rad(NormalizeDeg(wind_deg + 180.0)),
                            KnotsToMeterPerSecond(wind_speed_kt));

    drives_ref.FromProto(drives_reference);
    model.Simulate(drives_ref,
                   true_wind,
                   &controller_input);

    struct RudderProto status_drives  = INIT_RUDDERPROTO;    // out
    struct WindProto wind_sensor = INIT_WINDPROTO;           // out
    struct CompassProto compass = INIT_COMPASSPROTO;         // out
    struct GPSProto gps = INIT_GPSPROTO;                     // out

    struct IMUProto    imu = {                               // out
      last, 28.5,
      0, 0, 9.8, // acc
      0, 0, 0,   // gyr
      1, 0, 0,   // mag
      0, 0, 0,   // r/p/y
      48.2390, -4.7698, 0,   // l/l/a
      0, 0, 0   // speed
    };

    // Here come the lions, i.e. the stochastic delays by our
    // computer system, the Out-of-sync processing and communication delays by e.g.
    // wind, drive and IMU sensors and X-cats.
    controller_input.ToProto(&wind_sensor,
                             &status_drives,
                             &imu, &compass, &gps);
    // The wind sensor measures for a second, then sends that averaged value.
    if (rounds % 10 == 0) {
      prev_wind_sensor.timestamp_ms = now_ms();
      printf(OFMT_WINDPROTO(prev_wind_sensor));
    }
    // Take the middle measurement as a representative of the average over the last second.
    if (rounds % 10 == 5) {
      prev_wind_sensor = wind_sensor;
    }

    // Assume a delay of about 200ms for the drive status.
    // We get a fresh message every 5 ticks.
    if (rounds % 5 == 0) {
      // TODO Occasionally drop a status for realistic effect.
      printf(OFMT_STATUS_LEFT(prev_prev_status_drives));
      printf(OFMT_STATUS_RIGHT(prev_prev_status_drives));
      printf(OFMT_STATUS_SAIL(prev_prev_status_drives));
    }
    prev_prev_status_drives = prev_status_drives;
    prev_status_drives = status_drives;

    if (rounds % 4 == 0) {
      imu.timestamp_ms = now_ms();
      printf(OFMT_IMUPROTO(imu));
    }
    if (rounds % 4 == 0) {
      compass.timestamp_ms = now_ms();
      printf(OFMT_COMPASSPROTO(compass));
    }
    // The GPS sends with 1Hz.
    if (rounds % 10 == 1) {
      gps.timestamp_ms = now_ms();
      printf(OFMT_GPSPROTO(gps));
    }

    // Other ships
    if (rounds % 30 == 1) {  // 600 is the correct value here !!!
      double lat_deg = gps.lat_deg;
      double lon_deg = gps.lon_deg;
      // GPS faults are simulated, so sometimes we work with the IMU data.
      if (isnan(lat_deg)) {
        lat_deg = imu.lat_deg;
        lon_deg = imu.lng_deg;
      }
      fprintf(stderr, "SimulateShips, %lf %lf \n", lat_deg, lng_deg);
      SimulateShips(lat_deg, lng_deg);  // Writes the ais file.
    }

    ++rounds;
  }

  crash("Terminating.");
  return 0;
}


const int kShipStepSeconds=60;
const double MilesToDegrees = 1.852*360/40000;
int ais_debug = 1;

class Ship {
 public:
  Ship(double lat, double lon, double bearing_deg, double speed_kts, const std::string& name)
      : lat_(lat), lon_(lon), name_(name), cog_(bearing_deg) {
    lat_increment_ = kShipStepSeconds / 3600 * MilesToDegrees *
                     speed_kts * cos(Deg2Rad(bearing_deg));
    lon_increment_ = kShipStepSeconds / 3600 * MilesToDegrees *
                     speed_kts * sin(Deg2Rad(bearing_deg)) / cos(Deg2Rad(lat));
    speed_ = KnotsToMeterPerSecond(speed_kts);
    if (ais_debug) fprintf(stderr, "Create ship %s at [% 10.7lf, % 10.7lf] bearing %lf deg with %lf knots\n",
                           name_.c_str(), lat_, lon_, cog_, speed_kts);
  }

  bool less(const Ship& other) {
    if (speed_ == other.speed_) {
      return cog_ < other.cog_;
    }
    return speed_ < other.speed_;
  }

  void Advance() {
    lat_ += lat_increment_;
    lon_ += lon_increment_;
  }

  std::string ToAis() {
    char buffer[1000];
    sprintf(buffer, "timestamp_ms:%lld lat_deg:%10.7lf lng_deg:%10.7lf speed_m_s:%lf cog_deg:%lf",
     now_ms(), lat_, lon_, speed_, cog_);
    std::string result(buffer);
    return result;
  }

  double DistanceMiles(double lat, double lon) {
    const double dx = (lon - lon_) * cos(Deg2Rad(lat));
    const double dy = lat - lat_;
    return 1.0 / MilesToDegrees * sqrt(dx*dx + dy*dy);
  }

 private:
  double lat_;
  double lon_;
  double lat_increment_;
  double lon_increment_;
  std::string name_;
  double speed_;
  double cog_;
};


std::list<Ship> ships;
// called once a minute
// void SimulateShips(gps.latitude_deg, gps.longitude_deg);
// Tales the Avalons position as argument such that we create
// ships in the vincinity and remove them if they are too far away.
void SimulateShips(double latitude_deg, double longitude_deg) {

  double bearing_deg = rand() % 360;
  double speed_kts = 5 + (rand() % 15);
  double relative_rad = Deg2Rad(rand() % 360);  // bearing seen from Avalon.
  double first_distance_miles = 3;
  double lat =
      latitude_deg + first_distance_miles * MilesToDegrees * cos(relative_rad);
  double lon =
      longitude_deg + first_distance_miles * MilesToDegrees * sin(relative_rad) / cos(Deg2Rad(latitude_deg));
  if (!isnan(lat) && !isnan(lon)) {
    Ship flying_dutchman(lat, lon, bearing_deg, speed_kts, "MS Martha");
    ships.push_back(flying_dutchman);
  }

  FILE* fp = fopen("/tmp/ais.txt", "w");
  double min_distance = 10000;
  for (std::list<Ship>::iterator it = ships.begin();
       it != ships.end();
       ++it) {
    it->Advance();
    string line = it->ToAis();
    fprintf(fp,"%s\n", line.c_str());
    double distance_miles = it->DistanceMiles(latitude_deg, longitude_deg);
    if (ais_debug)
      fprintf(stderr, " distance %lf miles\n", distance_miles);
    if (distance_miles > 20)
      it = ships.erase(it);
    if (distance_miles < min_distance)
      min_distance = distance_miles;
  }

  if (ais_debug)
    fprintf(stderr, "%ld other ships, minimum distance %lf\n", ships.size(), min_distance);

  if (min_distance < 0.1)
    fprintf(stderr, "\n\n\nCRASH with minimum distance %lf\n\n\n", min_distance);

  fclose(fp);
}
