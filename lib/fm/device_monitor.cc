#include "lib/fm/device_monitor.h"

#include <string.h>

#include "lib/fm/fm_messages.h"
#include "lib/fm/fm.h"

DeviceMonitor::DeviceMonitor(const char *name, long timeout_ms) :
    timeout_ms_(timeout_ms),
    valid_count_(0), comm_err_count_(0), dev_err_count_(0) {
  strncpy(device_name_, name, sizeof(device_name_));
}

void DeviceMonitor::Ok() {
  valid_count_++;
  Keepalive();
}

void DeviceMonitor::CommError() {
  comm_err_count_++;
  Keepalive();
}

void DeviceMonitor::DevError() {
  dev_err_count_++;
  Keepalive();
}

// Explicitly report task status to system monitor (optional)
void DeviceMonitor::SetStatus(FM_STATUS status, const char *msg){
  static const char *status_names[] = FM_STATUS_NAMES_;
  FM::Monitor().SendMonMsg(FM_MSG_STATUS,
                           device_name_,
                           status_names[status],
                           msg);
  Keepalive(true); // flush out keepalive stats as well
}

void  DeviceMonitor::Keepalive(bool force) {
  if (force || timer_.Elapsed() > (timeout_ms_ / 3)) {
    FM::Monitor().SendMonMsg(FM_MSG_DEVICE_KEEPALIVE,
                             device_name_,
                             timer_.Elapsed(),
                             valid_count_,
                             comm_err_count_,
                             dev_err_count_);
    timer_.Set();
    valid_count_ = comm_err_count_ = dev_err_count_ = 0;
  }
}
