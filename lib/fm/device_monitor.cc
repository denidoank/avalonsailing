#include "lib/fm/device_monitor.h"

#include <string.h>

#include "lib/fm/fm.h"

DeviceMonitor::DeviceMonitor(const char *name, long timeout_ms) :
    timeout_ms_(timeout_ms), valid_count_(0), error_count_(0) {
  strncpy(device_name_, name, sizeof(device_name_));
}

void DeviceMonitor::DataValid() {
  valid_count_++;
  Keepalive();
}

void DeviceMonitor::DataError() {
  error_count_++;
  Keepalive();
}

// Explicitly report task status to system monitor (optional)
void DeviceMonitor::SetStatus(FM_STATUS status, const char *msg){
  static const char *status_names[] = FM_STATUS_NAMES_;
  FM::Monitor().SendMonMsg("dev=%s;status=%s;msg=%s",
                           device_name_,
                           status_names[status],
                           msg);
  Keepalive(true); // flush out keepalive stats as well
}

void  DeviceMonitor::Keepalive(bool force) {
  if (force || timer_.Elapsed() > (timeout_ms_ / 3)) {
    FM::Monitor().SendMonMsg("dev=%s;time=%ld;data=%ld;error=%ld",
                             device_name_,
                             timer_.Elapsed(),
                             valid_count_,
                             error_count_);
    timer_.Set();
    valid_count_ = error_count_ = 0;
  }
}
