// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Main loop for helmsman: open client sockets to wind and imu, server
// socket for helsman clients (e.g. the skipper and sysmon) and shovel
// data between all open file descriptors and the main controller.
//

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "common/convert.h"
#include "ship_control.h"

// -----------------------------------------------------------------------------
namespace {

// -----------------------------------------------------------------------------
//   Together with getopt in main, this is our minimalistic UI
// -----------------------------------------------------------------------------

const char* version = "$Id: $";
const char* argv0;
int verbose = 0;
int debug = 0;

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
	  "usage: %s [options]\n"
	  "options:\n"
	  "\t-C /working/dir     (default /var/run)\n"
	  "\t-S /path/to/socket  (default /working/dir/%s\n"
	  "\t-W /path/to/wind    (default /working/dir/wind\n"
	  "\t-I /path/to/imud    (default /working/dir/imud\n"
	  "\t-R /path/to/rudderd (default /working/dir/rudderd\n"		
	  "\t-f forward wind/imu/rudderd messages to clients\n"
	  "\t-d debug (don't go daemon, don't syslog)\n"
	  , argv0, argv0);
  exit(2);
}

// -----------------------------------------------------------------------------
//  Linked list of clients.
// -----------------------------------------------------------------------------
class Client {
public:

  // Accept a new connection from sck, and tie to a client.  Returns NULL
  // if the accept fails.  The new client will be put in the internal list.
  static Client* New(int sck);

  // Delete all clients who's files have been closed.
  static void Reap();

  // Tell select about all client's fd's
  static void SetFds(fd_set* rfds, fd_set* wfds, int* max_fd);

  // Read from and write to all clients whos fd was set.
  // Returns pointer to the last line of any client that got a complete input.
  static const char* Handle(fd_set* rfds, fd_set* wfds);

  // Send line to all clients
  static void Puts(char* line);

private:
  Client* next_;
  FILE* in_;	// NULL if accept failed or closed
  FILE* out_;
  struct sockaddr_un addr_;
  socklen_t addrlen_;
  bool pending_;  // possibly unflushed data in out buffer
  char line_[1024];  // last read line from in

  // List of all clients, linked on next_
  static Client* clients;

  Client(Client*, int);
  void puts(char* line);
  void flush();
};

Client* Client::clients = NULL;

Client::Client(Client* next, int sck) :
  next_(next), in_(NULL), out_(NULL), addrlen_(sizeof addr_), pending_(false)
{
  memset(line_, 0, sizeof line_);
  int fd = accept(sck, (struct sockaddr*)&addr_, &addrlen_);
  if (fd < 0) {
    fprintf(stderr, "accept:%s\n", strerror(errno));  // not fatal
    return;
  }
  in_  = fdopen(fd, "r");
  out_ = fdopen(dup(fd), "w");
  if (fcntl(fileno(in_),  F_SETFL, O_NONBLOCK) < 0) crash("fcntl(in)");
  if (fcntl(fileno(out_), F_SETFL, O_NONBLOCK) < 0) crash("fcntl(out)");
  setbuffer(out_, NULL, 64<<10); // 64k out buffer
}

Client* Client::New(int sck) {
  Client* cl = new Client(clients, sck);
  if (!cl->in_) {
    delete cl;
    return NULL;
  }
  clients = cl;
  return cl;
}

void Client::Reap() {
  Client** prevp = &clients;
  while (*prevp) {
    Client* curr = *prevp;
    if(!curr->in_) {
      *prevp = curr->next_;
      delete curr;
    } else {
      prevp = &curr->next_;
    }
  }
}

void Client::Puts(char* line) {
  for (Client* cl = clients; cl; cl = cl->next_) 
    cl->puts(line);
}

void Client::puts(char* line) {
        if (!out_) return;
        pending_ = true;
        if (fputs(line, out_) >= 0) return;
        if (feof(out_)) {
                pending_ = false;
                fclose(out_);
                out_ = NULL;
        }
}

void Client::flush() {
        if (!out_) return;
        if (fflush(out_) == 0) pending_ = false;
        if (feof(out_)) {
                pending_ = false;
                fclose(out_);
                out_ = NULL;
        }
}

void set_fd(fd_set* s, int* maxfd, FILE* f) {
        int fd = fileno(f);
        FD_SET(fd, s);
        if (*maxfd < fd) *maxfd = fd;
}

void Client::SetFds(fd_set* rfds, fd_set* wfds, int* max_fd) {
  for (Client* cl = clients; cl; cl = cl->next_) {
    if (cl->in_) set_fd(rfds, max_fd, cl->in_);
    if (cl->out_ && cl->pending_) set_fd(wfds, max_fd, cl->out_);
  }
}

// Returns the last line read from any client with input.
const char* Client::Handle(fd_set* rfds, fd_set* wfds) {
  const char* line = NULL;
  for (Client* cl = clients; cl; cl = cl->next_) {
    if (cl->out_ && FD_ISSET(fileno(cl->out_), wfds)) cl->flush();
     if (cl->in_ && FD_ISSET(fileno(cl->in_), rfds)) {
      while(fgets(cl->line_, sizeof(cl->line_), cl->in_)) {  // will read until EAGAIN
	line = cl->line_;
      }
    }
  }
  return line;
}
// -----------------------------------------------------------------------------

int svrsockopen(const char* path) {
  unlink(path);
  int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
  if (fd < 0) crash("socket");
  struct sockaddr_un addr = { AF_LOCAL, 0 };
  strncpy(addr.sun_path, path, sizeof addr.sun_path );
  if (bind(fd, (struct sockaddr*) &addr, sizeof addr) < 0) crash("bind(%s)", path);
  if (listen(fd, 8) < 0) crash("listen(%s)", path);
  return fd;
}

int clsockopen(const char* path) {
  int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
  if (fd < 0) crash("socket");
  struct sockaddr_un addr = { AF_LOCAL, 0 };
  strncpy(addr.sun_path, path, sizeof(addr.sun_path));
  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(fd);
    return 0;
  }
  if (fcntl(fd,  F_SETFL, O_NONBLOCK) < 0) crash("fcntl(%s)", path);
  return fd;
}

int clsockopen_wait(const char* path) {
  int s;
  for (int i = 0; ; i++) {
    s = clsockopen(path);
    if (s) break;
    fprintf(stderr, "Waiting for %s...%c\r", path, "-\\|/"[i++%4]);
    sleep(1);
  }
  fprintf(stderr, "Waiting for %s...bingo!\n", path);
  return s;
}

// -----------------------------------------------------------------------------
//   I/O parsing and printing
// All return false on garbage
// -----------------------------------------------------------------------------

int sscan_rudd(const char *line, DriveActualValuesRad* s) {
  s->Reset();
  while (*line) {
    char key[16];
    double value;

    int skip = 0;
    int n = sscanf(line, "%16[a-z_]:%lf %n", key, &value, &skip);
    if (n < 2) return 0;
    if (skip == 0) return 0;
    line += skip;

    if (!strcmp(key, "rudder_l_deg") && !isnan(value))  { s->gamma_rudder_left_rad  = Deg2Rad(value); s->homed_rudder_left  = true; continue; }
    if (!strcmp(key, "rudder_r_deg") && !isnan(value))  { s->gamma_rudder_right_rad = Deg2Rad(value); s->homed_rudder_right = true; continue; }
    if (!strcmp(key, "sail_deg")     && !isnan(value))  { s->gamma_sail_rad         = Deg2Rad(value); s->homed_sail         = true; continue; }
    if (!strcmp(key, "timestamp_ms"))  continue;

    return 0;
  }
  return 1;
}

int sscan_wind(const char *line, WindSensor* s) {
  s->Reset();
  while (*line) {
    char key[16];
    double value;

    int skip = 0;
    int n = sscanf(line, "%16[a-z_]:%lf %n", key, &value, &skip);
    if (n < 2) return 0;
    if (skip == 0) return 0;
    line += skip;

    if (!strcmp(key, "angle_deg") && !isnan(value))  { s->alpha_deg = Deg2Rad(value); continue; }
    if (!strcmp(key, "speed_m_s") && !isnan(value))  { s->mag_kn = MeterPerSecondToKnots(value); continue; }
    if (!strcmp(key, "timestamp_ms"))  continue;
    
    return 0;
  }
  return 1;
}

int sscan_imud(const char *line, Imu* s) {
  s->Reset();
  while (*line) {
    char key[16];
    double value;

    int skip = 0;
    int n = sscanf(line, "%16[a-z_]:%lf %n", key, &value, &skip);
    if (n < 2) return 0;
    if (skip == 0) return 0;
    line += skip;

    if (!strcmp(key, "temp_c"))      { s->temperature_c       = value; continue; }

    if (!strcmp(key, "acc_x_m_s2"))  { s->acceleration.x_m_s2 = value; continue; }
    if (!strcmp(key, "acc_y_m_s2"))  { s->acceleration.y_m_s2 = value; continue; }
    if (!strcmp(key, "acc_z_m_s2"))  { s->acceleration.z_m_s2 = value; continue; }

    if (!strcmp(key, "gyr_x_rad_s"))  { s->gyro.omega_x_rad_s = value; continue; }
    if (!strcmp(key, "gyr_y_rad_s"))  { s->gyro.omega_y_rad_s = value; continue; }
    if (!strcmp(key, "gyr_z_rad_s"))  { s->gyro.omega_z_rad_s = value; continue; }

    if (!strcmp(key, "mag_x_au"))  { continue; }
    if (!strcmp(key, "mag_y_au"))  { continue; }
    if (!strcmp(key, "mag_z_au"))  { continue; }

    if (!strcmp(key, "roll_deg"))   { s->attitude.phi_x_rad = Deg2Rad(value); continue; }
    if (!strcmp(key, "pitch_deg"))  { s->attitude.phi_y_rad = Deg2Rad(value); continue; }
    if (!strcmp(key, "yaw_deg"))    { s->attitude.phi_z_rad = Deg2Rad(value); continue; }

    if (!strcmp(key, "lat_deg"))  { s->position.latitude_deg  = value; continue; }
    if (!strcmp(key, "lng_deg"))  { s->position.longitude_deg = value; continue; }
    if (!strcmp(key, "alt_m"))    { s->position.altitude_m    = value; continue; }

    if (!strcmp(key, "vel_x_m_s"))  { s->velocity.x_m_s = value; continue; }
    if (!strcmp(key, "vel_y_m_s"))  { s->velocity.y_m_s = value; continue; }
    if (!strcmp(key, "vel_z_m_s"))  { s->velocity.z_m_s = value; continue; }

    if (!strcmp(key, "timestamp_ms"))  continue;
    
    return 0;
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

int snprint_rudd(char *line, int size, const DriveReferenceValuesRad& s) {
  return snprintf(line, size,
		  "timestamp_ms:%lld rudder_l_deg:%.3f rudder_r_deg:%.3f sail_deg:%.3f\n",
		  now_ms(), s.gamma_rudder_star_left_rad, s.gamma_rudder_star_right_rad,  s.gamma_sail_star_rad);
}

int snprint_helmsmanout(char *line, int size, const SkipperInput& s) {
    return snprintf(line, size,
		    "timestamp_ms:%lld lat_deg:%f lng_deg:%f wind_angle_deg:%f wind_speed_m_s:%f\n" ,
		    now_ms(), s.latitude_deg, s.longitude_deg,  s.angle_true_deg, KnotsToMeterPerSecond(s.mag_true_kn));
}

} // namespace

// -----------------------------------------------------------------------------
//   Main.
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {

  const char* cwd = "/var/run";
  const char* path_to_socket = NULL;
  const char* path_to_imud = "imud";
  const char* path_to_wind = "wind";
  const char* path_to_rudderd = "rudderd";
  
  int ch;
  int forward = 0;

  argv0 = strrchr(argv[0], '/');
  if (argv0) ++argv0; else argv0 = argv[0];

  while ((ch = getopt(argc, argv, "C:I:R:S:W:dfhv")) != -1){
      switch (ch) {
      case 'C': cwd = optarg; break;
      case 'S': path_to_socket  = optarg; break;
      case 'I': path_to_imud    = optarg; break;
      case 'W': path_to_wind    = optarg; break;
      case 'R': path_to_rudderd = optarg; break;
      case 'd': ++debug; break;
      case 'f': ++forward; break;
      case 'v': ++verbose; break;
      case 'h':
      default:
	  usage();
      }
  }

  if (argc != optind) usage();

  if (!debug) openlog(argv0, LOG_PERROR, LOG_DAEMON);

  if (strcmp(cwd, ".") && chdir(cwd) < 0) crash("chdir(%s)", cwd);
  cwd = getcwd(NULL, 0);

  // Open client sockets.  Wait until they're all there so you can start up in any order.
  FILE* imud = fdopen(clsockopen_wait(path_to_imud), "r");
  FILE* wind = fdopen(clsockopen_wait(path_to_wind), "r");
  FILE* rudd = fdopen(clsockopen_wait(path_to_rudderd), "r+");
  setlinebuf(rudd);
  bool rudd_cmd_pending = false;

  // Set up server socket.
  if (!path_to_socket) path_to_socket = argv0;
  int sck = svrsockopen(path_to_socket);
  
  // clients that hang up should not make us crash.
  if (signal(SIGPIPE, SIG_IGN) == -1) crash("signal");

  if (debug)
    fprintf(stderr, "created socket:%s\n", path_to_socket);
  else
    syslog(LOG_INFO, "Helmsman version %s working dir %s listening on %s", version, cwd, path_to_socket);

  // Go daemon and write pidfile.
  if (!debug) {
    daemon(0,0);
    
    char* path_to_pidfile = NULL;
    asprintf(&path_to_pidfile, "%s.pid", path_to_socket);
    FILE* pidfile = fopen(path_to_pidfile, "w");
    if(!pidfile) crash("writing pidfile");
    fprintf(pidfile, "%d\n", getpid());
    fclose(pidfile);
    free(path_to_pidfile);
  }

  ControllerInput ctrl_in;

  // Main loop
  for (;;) {
    timespec timeout = { 0, 250*1000*1000 }; // wake up at least 4/second
    fd_set rfds;
    fd_set wfds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    FD_SET(sck, &rfds);
    int max_fd = sck;

    Client::SetFds(&rfds, &wfds, &max_fd);

    set_fd(&rfds, &max_fd, wind);
    set_fd(&rfds, &max_fd, imud);
    set_fd(&rfds, &max_fd, rudd);

    if (rudd_cmd_pending) set_fd(&wfds, &max_fd, rudd);

    sigset_t empty_mask;
    sigemptyset(&empty_mask);
    int r = pselect(max_fd + 1, &rfds, &wfds, NULL, &timeout, &empty_mask);
    if (r == -1 && errno != EINTR) crash("pselect");

    if (FD_ISSET(sck, &rfds)) Client::New(sck);

    if (FD_ISSET(fileno(rudd), &wfds)) fflush(rudd);

    if (FD_ISSET(fileno(rudd), &rfds)) {
      char line[1024];
      while (fgets(line, sizeof line, rudd)) {
	int n = sscan_rudd(line, &ctrl_in.drives);
	if (!n && debug) fprintf(stderr, "Ignoring garbage from rudderd: %s\n", line);
      }
      if (forward) Client::Puts(line);
    }

    if (FD_ISSET(fileno(wind), &rfds)) {
      char line[1024];
      while (fgets(line, sizeof line, wind)) {
	int n = sscan_wind(line, &ctrl_in.wind);
	if (!n && debug) fprintf(stderr, "Ignoring garbage from wind: %s\n", line);
      }
      if (forward) Client::Puts(line);
    }

    if (FD_ISSET(fileno(imud), &rfds)) {
      char line[1024];
      while (fgets(line, sizeof line, wind)) {
	int n = sscan_imud(line, &ctrl_in.imu);
	if (!n && debug) fprintf(stderr, "Ignoring garbage from imud: %s\n", line);
      }
      if (forward) Client::Puts(line);
    }

    { // see if anyone sent us a command
      const char* line = Client::Handle(&rfds, &wfds);
      if (line) {
	if (strstr(line, "brake")) ShipControl::Brake("");
	else if(strstr(line, "dock")) ShipControl::Docking("");
	else if(sscanf(line, "alpha_star_deg:%lf", &ctrl_in.alpha_star) == 1) {
	  ShipControl::Normal("");
	} else if(sscanf(line, "%lf", &ctrl_in.alpha_star) == 1) {  // nice for manual control
	  ShipControl::Normal("");
	} else {
	  ctrl_in.alpha_star = 0;
	}
      }
    }

    ControllerOutput ctrl_out;
    ShipControl::Run(ctrl_in, &ctrl_out);

    // send rudder command
    if (1) { // always?
      char line[1024];
      int n = snprint_rudd(line, sizeof line, ctrl_out.drives_reference);
      if (n <= 0 || n > sizeof line) crash("printing rudder command line");
      if (fputs(line, rudd) == EOF) crash("Could not send ruddercommand");  // TODO allow for a couple of EAGAINS
      rudd_cmd_pending = true;
    }

    // write the helmsman output
    if (1) {  // always?
      char line[1024];
      int n = snprint_helmsmanout(line, sizeof line, ctrl_out.skipper_input);
      if (n <= 0 || n > sizeof line) crash("printing state line");
      Client::Puts(line);
    }

  }  // for ever

  crash("Main loop exit");

  return 0;
}
