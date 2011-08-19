// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Single shot version of skipper.
//
// sample invocation
//  skipper2 ...  <<EOF
// timestamp_ms:1311258570959 mmsi:265330000 msgtype:1 status:8 speed_m_s:1.3 lat_deg:55.558400 lng_deg:14.356425 cog_deg:209.6 
// timestamp_ms:1311258570959 mmsi:266137000 msgtype:18 speed_m_s:4.5 lat_deg:55.537585 lng_deg:14.374660 cog_deg:176.3 
// timestamp_ms:1311258570956 mmsi:258762000 msgtype:5 size_m:106 shipname:'ANICIA'
// timestamp_ms:1311258570956 mmsi:258762000 msgtype:3 status:1 rot_deg_min:0 speed_m_s:0.1 lat_deg:55.550258 lng_deg:14.380817 cog_deg:49.3 heading_deg:297 
// EOF
#include <vector>

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "vskipper.h"
#include "proto/helmsman.h"

const char* version = "$Id: $";
const char* argv0;
int verbose = 0;
int debug = 0;

namespace skipper {
namespace {

void crash(const char* fmt, ...) {
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

void usage(void) {
  fprintf(stderr,
          "usage: %s [options] < aistable.txt\n"
          "options:\n"
          "\t-T now_ms\n"
          "\t-A latitude_deg\n"
          "\t-O longitude_deg\n"
          "\t-W wind_angle_deg\n"
          "\t-S wind_speed_m_s\n"
          "\t-P global_plan_deg\n"
          "\t-d debug (don't go daemon, don't syslog)\n"
          , argv0);
  exit(2);
}

int sscan_ais(const char *line, uint64_t now_ms, AisInfo* s) {
  s->timestamp_ms = now_ms;
  while (*line) {
    char key[16];
    double value = NAN;

    int skip = 0;
    int n = sscanf(line, "%16[a-z_]:%lf %n", key, &value, &skip);
    if (n < 2) return 0;
    if (skip == 0) return 0;
    line += skip;

    if (!strcmp(key, "timestamp_ms")  && !isnan(value)) {
      s->timestamp_ms = uint64_t(value);
      continue;
    }
    if (!strcmp(key, "lat_deg")       && !isnan(value)) {
      s->position = LatLon::Degrees(value, s->position.lon_deg());
      continue;
    }
    if (!strcmp(key, "lng_deg")       && !isnan(value)) {
      s->position = LatLon::Degrees(s->position.lat_deg(), value);
      continue;
    }
    if (!strcmp(key, "speed_m_s")     && !isnan(value)) {
      s->speed_m_s = value;
      continue;
    }
    if (!strcmp(key, "cog_deg")       && !isnan(value)) {
      s->bearing = Bearing::Degrees(value);
      continue;
    }
    // ignore anything else
    // return 0;
  }
  return 1;
}

uint64_t now_ms() {
  timeval tv;
  if (gettimeofday(&tv, NULL) < 0) crash("gettimeofday");
  uint64_t ms1 = tv.tv_sec;  ms1 *= 1000;
  uint64_t ms2 = tv.tv_usec; ms2 /= 1000;
  return ms1 + ms2;
}

} // namespace
} // skipper


// -----------------------------------------------------------------------------
//   Main.
// -----------------------------------------------------------------------------
using namespace skipper;
int main(int argc, char* argv[]) {

  argv0 = strrchr(argv[0], '/');
  if (argv0) ++argv0; else argv0 = argv[0];

  int ch;

  AvalonState us;
  uint64_t now = now_ms();
  us.timestamp_ms = now;

  while ((ch = getopt(argc, argv, "dhv:A:O:T:W:S:P:")) != -1){
      switch (ch) {
      case 'd': ++debug; break;
      case 'v': ++verbose; break;
      case 'T': us.timestamp_ms = atoll(optarg); break;
      case 'A': us.position = LatLon::Degrees(atof(optarg), us.position.lon_deg()); break;
      case 'O': us.position = LatLon::Degrees(us.position.lat_deg(), atof(optarg)); break;
      case 'W': us.wind_from = Bearing::Degrees(atof(optarg)); break;
      case 'S': us.wind_speed_m_s = atof(optarg); break;
      case 'P': us.target = Bearing::Degrees(atof(optarg)); break;
      case 'h':
      default:
         usage();
      }
  }

  // TODO(zis): Set state.target according to global plan given lat/lon

  if (argc != optind) usage();

  if (debug) {
    fprintf(stderr, "Skipper version %s\n", version);
  } else {
    openlog(argv0, LOG_PERROR, LOG_DAEMON);
    syslog(LOG_INFO, "Skipper version %s", version);
  }

  std::vector<AisInfo> ais;
  while (!feof(stdin)) {
    char line[1024];
    if (!fgets(line, sizeof(line), stdin)) break;
    AisInfo ai;
    if (!sscan_ais(line, now, &ai)) continue;
    ais.push_back(ai);

    if (debug) fprintf(stderr, "ais: %lld %f %f %f %f\n",
                       ai.timestamp_ms, ai.position.lat_deg(), ai.position.lon_deg(),
                       ai.speed_m_s, ai.bearing.deg());
  }

  HelmsmanCtlProto ctl = INIT_HELMSMANCTLPROTO;
  ctl.timestamp_ms = us.timestamp_ms;
  ctl.alpha_star_deg = RunVSkipper(us, ais, debug).deg();
  int dummy;
  printf(OFMT_HELMSMANCTLPROTO(ctl, &dummy));

  return 0;
}
