// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.

#include <stdint.h>
#include <cstdio>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <set>
#include <vector>

#include "io/ipc/key_value_pairs.h"
#include "io/ipc/producer_consumer.h"

#include "lib/fm/fm.h"
#include "lib/fm/log.h"
#include "lib/util/reader.h"
#include "lib/util/serial.h"

using namespace std;

// TODO(rekwall): get debug mode from FM lib.
const bool debug_mode = false;

// Supported IMU messages. Enum values come from IMU manual.
enum ImuMessageType {
  IMU_GOTOMEASUREMENT      = 0x10,
  IMU_GOTOMEASUREMENTACK   = 0x11,
  IMU_GOTOCONFIG           = 0x30,
  IMU_GOTOCONFIGACK        = 0x31,
  IMU_WAKEUP               = 0x3E,
  IMU_WAKEUPACK            = 0x3F,
  IMU_OUTPUTSETTINGS       = 0xD2,
  IMU_OUTPUTSETTINGSACK    = 0xD3,
  IMU_OUTPUTMODE           = 0xD0,
  IMU_OUTPUTMODEACK        = 0xD1,
  IMU_MTDATA               = 0x32,
  IMU_SKIPFACTOR           = 0xD4,
  IMU_SKIPFACTORACK        = 0xD5,
  IMU_RESTOREFACTORYDEF    = 0x0E,
  IMU_RESTOREFACTORYDEFACK = 0x0F,
  IMU_REQPERIOD            = 0x04,
  IMU_REQPERIODACK         = 0x05,
  IMU_REQDATALENGTH        = 0x0A,
  IMU_DATALENGTH           = 0x0B,
  IMU_REQBAUDRATE          = 0x18,
  IMU_REQBAUDRATEACK       = 0x19,
  IMU_REQDATA              = 0x34,
  IMU_CONFIGURATION        = 0x0D,
};

// ImuMessage: ID, data and length of data. Data is stored with the
// least-significant byte in data[0] and the most-significant byte
// in data[length-1]. On the wire, data[length-1] is the first byte
// in the data part of the packet, whereas data[0] is the last one
// (closest to the checksum byte at the end of the packet).
struct ImuMessage {
  ImuMessage(): mid(0x0), length(0), data(NULL) {}

  // Does not take ownership of data_.
  // If length is zero, data can be NULL. If length is greater than 0,
  // data[0] must be the lowest-order, least-significant byte, and
  // data[length-1] the highest-order, most-significant one.
  ImuMessage(uint8_t mid_, uint8_t* data_, uint8_t length_)
      : mid(mid_),
        length(length_),
        data(data_) {}

  // Common case of ImuMessage without data (e.g. to get settings).
  explicit ImuMessage(uint8_t mid_)
      : mid(mid_),
        length(0),
        data(NULL) {}


  // Identifier. Must be one of the ImuMessageType values.
  uint8_t mid;
  uint8_t length;
  uint8_t* data;

  uint8_t Checksum() const {
    uint8_t sum = 0xFF;  // Second byte of the preamble.
    sum += mid;
    sum += length;
    for (int i = 0; i < length; ++i) {
      sum += data[i];
    }
    return 0x100 - sum;
  }

  // Returns this message as a sequence of bytes that can
  // be written to the IMU module.
  // Caller owns the data pointer. First is data, second is
  // number bytes in data.
  pair<const uint8_t*, int> GetWireformat() const {
    const int message_size =
        3 /* preamble + mid */ +
        1 /* length */ +
        length /* data bytes */ +
        1 /* checksum */;
    uint8_t* message = new uint8_t[message_size];
    // Preamble
    message[0] = 0xFA;
    message[1] = 0xFF;
    // Message id.
    message[2] = mid;
    // Length
    message[3] = length;
    // Data
    if (length > 0) {
      // On the wire, the most-significant byte is sent first. In data, the
      // most-significant byte is the one at data[length - 1].
      for (int i = 0; i < length; ++i) {
        message[4 + i] = data[length - 1 - i];
      }
    }
    // Checksum.
    message[length + 4] = Checksum();
    return make_pair(message, message_size);
  }
};

const int kReadTimeout = 5000;

uint8_t ReadByte(Reader* reader) {
  char c;
  if (reader->ReadChar(&c, kReadTimeout) != Reader::READ_OK) {
    FM_LOG_FATAL("Reader errors on IMU tty");
  }
  return static_cast<uint8_t>(c);
}

// Finds a sequence of bytes in the input stream. Used to find the
// start of a message.
bool FindPreamble(Reader* reader) {
  vector<uint8_t> preamble;
  preamble.push_back(0xFA);
  preamble.push_back(0xFF);

  // Fill the window with preamble.size() chars to start with.
  vector<uint8_t> window(preamble.size(), 0);
  for (int i = 0; i < window.size(); ++i) {
    window[i] = ReadByte(reader);
  }
  // Now continue reading chars as long as sequence isn't matched.
  while (preamble != window) {
    window.erase(window.begin());
    window.push_back(ReadByte(reader));
  }
  return true;
}

// Logs the contents of message, prefixed by prefix.
void DebugLogData(const uint8_t* message, int length, const char* prefix) {
  if (!debug_mode) return;

  char* as_hex = new char[length * 5];
  for (int i = 0; i < length; ++i) {
    if (i > 0) {
      // snprintf adds a '\0' at the end of the string.
      as_hex[i * 5 - 1] = ' ';
    }
    snprintf(as_hex + i * 5, 5, "0x%02X ", message[i]);
  }
  FM_LOG_INFO("%s: %s", prefix, as_hex);
  delete as_hex;
}

// Convenience wrapper around the raw DebugLogData call, for ImuMessage objects.
void DebugLogData(const ImuMessage& message, const char* prefix) {
  // Factor out the data copying.
  pair<const uint8_t*, int> data = message.GetWireformat();
  DebugLogData(data.first, data.second, prefix);
  delete data.first;
}

// Reads one ImuMessage. Returns true if the message was read successfully,
// false otherwise (e.g. mismatching checksums or NULL reader or message
// parameters).
bool ReadImuMessage(Reader* reader, ImuMessage* message) {
  if (reader == NULL || message == NULL) return false;

  FindPreamble(reader);
  message->mid = ReadByte(reader);
  message->length = ReadByte(reader);
  message->data = new uint8_t[message->length];
  // TODO(rekwall): bulk read?
  // Bytes are read from most-significant to least-significant. message->data[0]
  // is the least significant byte.
  for (int i = message->length - 1; i >= 0; --i) {
    message->data[i] = ReadByte(reader);
  }
  uint8_t checksum = ReadByte(reader);
  if (checksum != message->Checksum()) {
    FM_LOG_WARN("Corrupt message. Receive checksum 0x%02X. Should be 0x%02X.",
                checksum, message->Checksum());
    // TODO(rekwall): log this with the system monitor.
    return false;
  }

  DebugLogData(*message, "Read message");
  return true;
}

// Writes an ImuMessage to fd.
bool SendToImu(int fd, const ImuMessage& toSend) {
  // uint8_t mid, uint8_t* data, uint8_t length) {
  // ImuMessage toSend(mid, data, length);
  pair<const uint8_t*, int> wireformat = toSend.GetWireformat();
  const uint8_t* data = wireformat.first;
  const int length = wireformat.second;

  DebugLogData(data, length, "Sending message");
  int done = 0;
  while (done < length) {
    int written = write(fd, data + done, length - done);
    if (written <= 0) {
      FM_LOG_FATAL("Write failed");
    }
    done += written;
  }
  delete data;
  return true;
}

// Waits for an ImuMessage and possibly skips a number of data messages if
// skip_data_messages is true. Returns the identifier of the message that
// was read. If received is not NULL, the message that was read is copied to it.
uint8_t WaitForMessage(bool skip_data_messages, Reader* reader,
                       ImuMessage* received) {
  ImuMessage ack;
  for (int reads = 0; reads < 100; ++reads) {
    // Reset ack.
    ack.mid = 0;
    ack.length = 0;
    if (ack.data != NULL) {
      delete ack.data;
      ack.data = NULL;
    }

    if (!ReadImuMessage(reader, &ack)) {
      FM_LOG_FATAL("Could not read IMU message");
    }
    if (skip_data_messages && ack.mid == IMU_MTDATA) {
      continue;
    }
    if (received != NULL) {
      *received = ack;
    }
    return ack.mid;
  }
  FM_LOG_FATAL("Skipped too many messages.");
}

// Shorthand for WaitForMessage without skipping MTData messages.
uint8_t WaitForMessage(Reader* reader, ImuMessage* received) {
  return WaitForMessage(false, // Don't skip data messages.
                        reader, received);
}

// Expected output mode.
uint8_t output_mode[] =
  {
    0x37,  // (1 << 0) for Temperature.
           // (1 << 1) for Calibrated data.
           // (1 << 2) for Orientation data.
           // (1 << 4) for Position data.
           // (1 << 5) for Velocity.
    0x0    // Defaults for everything else.
  };

// Expected output settings.
uint8_t output_settings[] =
  {
    0x04,  // (1 << 2) for Euler angles.
    0x0C,  // (1 << 10) for Disable analog #1 output.
           // (1 << 11) for Disable analog #2 output.
    0x0,
    0x0
  };

// Expected skip factor.
uint8_t skip_factor[] =
  {
    0x13, // Skip 19 out of 20 (to get data at about 5Hz).
    0x0
  };

// Expected period for measurements.
uint8_t period[] = { 0x80, 0x04 };  // Default 100 Hz.


// Returns true if the IMU is already configured the way we expect.
bool IsInExpectedConfig(ImuMessageType request,
                        ImuMessageType ack,
                        uint8_t* expected_config,
                        int expected_config_length,
                        Reader* reader) {
  SendToImu(reader->GetFd(), ImuMessage(request));
  ImuMessage received;
  if (ack != WaitForMessage(reader, &received)) {
    FM_LOG_WARN("Didn't get expected ack 0x%02X.", ack);
    return false;
  }
  if (received.length != expected_config_length) {
    FM_LOG_WARN("Wrong data length for config 0x%02X.", request);
    return false;
  }
  for (int i = 0; i < expected_config_length; ++i) {
    if (received.data[i] != expected_config[i]) {
      return false;
    }
  }
  return true;
}

bool GetOutputMode(Reader* reader) {
  return IsInExpectedConfig(IMU_OUTPUTMODE, IMU_OUTPUTMODEACK,
                            output_mode, 2, reader);
}

bool SetOutputMode(Reader* reader) {
  SendToImu(reader->GetFd(), ImuMessage(IMU_OUTPUTMODE, output_mode, 2));
  return IMU_OUTPUTMODEACK == WaitForMessage(reader, NULL);
}

bool GetOutputSettings(Reader* reader) {
  return IsInExpectedConfig(IMU_OUTPUTSETTINGS, IMU_OUTPUTSETTINGSACK,
                            output_settings, 4, reader);
}

void SetOutputSettings(Reader* reader) {
  SendToImu(reader->GetFd(),
                 ImuMessage(IMU_OUTPUTSETTINGS, output_settings, 4));
  WaitForMessage(IMU_OUTPUTSETTINGSACK, reader, NULL);
}

bool GetPeriod(Reader* reader) {
  return IsInExpectedConfig(IMU_REQPERIOD, IMU_REQPERIODACK,
                            period, 2, reader);
}

bool SetPeriod(Reader* reader) {
  SendToImu(reader->GetFd(), ImuMessage(IMU_REQPERIOD, period, 2));
  return IMU_REQPERIODACK == WaitForMessage(reader, NULL);
}


bool GetSkipFactor(Reader* reader) {
  return IsInExpectedConfig(IMU_SKIPFACTOR, IMU_SKIPFACTORACK,
                            skip_factor, 2, reader);
}

bool SetSkipFactor(Reader* reader) {
  SendToImu(reader->GetFd(), ImuMessage(IMU_SKIPFACTOR, skip_factor, 2));
  // Undocumented: IMU sends a IMU_WAKEUP after the skip factor is reset.
  if (IMU_WAKEUP != WaitForMessage(reader, NULL)) {
    FM_LOG_WARN("Expected IMU_WAKEUP");
    return false;
  }
  SendToImu(reader->GetFd(), ImuMessage(IMU_WAKEUPACK));
  SendToImu(reader->GetFd(), ImuMessage(IMU_SKIPFACTOR));
  return IMU_SKIPFACTORACK == WaitForMessage(reader, NULL);
}

bool GetDataLength(Reader* reader) {
  SendToImu(reader->GetFd(), ImuMessage(IMU_REQDATALENGTH));
  return IMU_DATALENGTH == WaitForMessage(reader, NULL);
}

bool GetBaudrate(Reader* reader) {
  SendToImu(reader->GetFd(), ImuMessage(IMU_REQBAUDRATE));
  return IMU_REQBAUDRATEACK == WaitForMessage(reader, NULL);
}

// Resets the IMU to factory defaults.
// Warning: this is slow (on the order of one minute) and the IMU
// needs to be reconfigured after this.
void ResetToFactoryDefaults(Serial* serial) {
  int fd = serial->In().GetFd();
  SendToImu(fd, ImuMessage(IMU_RESTOREFACTORYDEF));
  uint8_t ack = WaitForMessage(true, // Skip MTData messages
                               &serial->In(), NULL);
  switch (ack) {
    case IMU_WAKEUP:
      // Quickly answer wakeup message.
      SendToImu(fd, ImuMessage(IMU_WAKEUPACK));
      break;
    case IMU_RESTOREFACTORYDEFACK:
      // Do nothing.
      break;
    default:
      FM_LOG_WARN("Unexpected ack to restore factory defaults: 0x%20X",
                  ack);
      break;
  }
}

// Configures the IMU to output what we expect. It also resets some of
// the IMU's state (e.g. altitude, velocity), which quickly drifts when
// no GPS fix is available. Configuring the IMU is fast (around 1-2 seconds).
// TODO(rekwall): check whether this invalidates the "history" of the IMU, which
// is used to get more precise measurements. If yes, don't reconfigure too
// often.
bool ConfigureImu(Serial* serial) {
  Reader* reader = &serial->In();
  const int fd = reader->GetFd();

  // Go to config mode.
  {
    SendToImu(fd, ImuMessage(IMU_GOTOCONFIG));
    uint8_t ack = WaitForMessage(true, // Skip MTData messages
                                 reader, NULL);
    switch (ack) {
      case IMU_WAKEUP:
        // A goto config message is sometimes replied to with an IMU_WAKEUP. If
        // it is, the IMU_WAKEUPACK has to be sent within 500ms or the IMU goes
        // back to measurement mode.
        SendToImu(fd, ImuMessage(IMU_WAKEUPACK));
        // Again ask for config.
        SendToImu(fd, ImuMessage(IMU_GOTOCONFIG));
        ack = WaitForMessage(reader, NULL);

        if (IMU_GOTOCONFIGACK != ack) {
          FM_LOG_WARN("Could not enter config mode. "
                      "Got unexpected ack: 0x%20X", ack);
          return false;
        }
        break;
      case IMU_GOTOCONFIGACK:
        // Do nothing: we're in config mode.
        break;
      default:
        FM_LOG_WARN("Unexpected ack to go to config: 0x%20X", ack);
        return false;
    }
  }

  if (!GetOutputSettings(reader)) {
    SetOutputSettings(reader);
  }
  if (!GetOutputMode(reader)) {
    SetOutputMode(reader);
  }
  if (!GetPeriod(reader)) {
    SetPeriod(reader);
  }

  // Here for debugging currently. We should check that the data length is what
  // we expect.
  GetDataLength(reader);
  GetBaudrate(reader);
  SetSkipFactor(reader);

  SendToImu(fd, ImuMessage(IMU_GOTOMEASUREMENT));
  uint8_t ack = WaitForMessage(reader, NULL);
  if (IMU_GOTOMEASUREMENTACK != ack) {
    FM_LOG_WARN("Unexpected ack to go to measurement: 0x%02X", ack);
    return false;
  }
  return true;
}

// Reads one MTData packet and converts the data part to an array of floats.
// Conveniently, all of our data comes in 4-byte chunks that can be converted
// to floats. The order in numbers is the same as in the incoming IMU message,
// so that the order fits the IMU documentation.
bool ReadData(Serial* serial, float* numbers, int numbers_size) {
  ImuMessage data;
  // Undocumented: the IMU was previously in a state where it would always
  // broadcast IMU_MTDATA packets in measurement mode. Now it does not, so
  // we have to explicitly request the messages.
  // TODO(rekwall): investigate further. The documentation says that this
  // should only happen if the skip factor is 0xFFFF, but we set it to
  // 0x0013 (skip 19 out of 20 messages, to get data at about 5 Hz).
  SendToImu(serial->In().GetFd(), ImuMessage(IMU_REQDATA));
  uint8_t mid = WaitForMessage(&serial->In(), &data);
  if (IMU_MTDATA  != mid) {
    FM_LOG_WARN("Received unexpected message id 0x%02X when waiting for data.",
                mid);
    return false;
  }

  if (data.length != numbers_size * 4) {
    // TODO(rekwall): go to config mode again.
    FM_LOG_WARN("Wrong data length: %d. Expected %d.",
                data.length, numbers_size * 4);
    return false;
  }

  // numbers[0] should be the first chunk received in the IMU message, i.e.
  // the end of the data.data array.
  for (int i = 0; i < numbers_size; ++i) {
    numbers[i] = *(float*) (&data.data[(numbers_size - 1 - i) * 4]);
  }
  return true;
}

const char* labels[] = {
  "temp_c",
  "acc_x_m_s2",
  "acc_y_m_s2",
  "acc_z_m_s2",
  "gyr_x_rad_s",
  "gyr_y_rad_s",
  "gyr_z_rad_s",
  "mag_x_NA",
  "mag_y_NA",
  "mag_z_NA",
  "roll_deg",
  "pitch_deg",
  "yaw_deg",
  "lat_deg",
  "lng_deg",
  "alt_m",
  "vel_x_m_s",
  "vel_y_m_s",
  "vel_z_m_s",
  NULL
};

void DumpData(float* numbers, int numbers_size, const Producer& producer) {
  KeyValuePair result;
  result.Add("source", "imu");
  for (int i = 0; i < numbers_size; ++i) {
    if (labels[i]) {
      char number[20] = "";
      snprintf(number, 20, "%f", numbers[i]);
      result.Add(labels[i], number);
    }
  }
  producer.Produce(result.ToString());
}

int main(int argc, char** argv) {
  int arg_start = FM::Init(argc, argv);

  if (argc - arg_start != 2) {
    FM_LOG_FATAL("Usage: %s <tty> <outputfile>",
                 argv[0]);
  }

  Serial serial;
  if (!serial.Init(argv[arg_start], B115200)) {
    FM_LOG_FATAL("Could not open tty %s", argv[arg_start]);
  }

  Producer producer(argv[arg_start + 1]);

  const int numbers_size = 19;
  float numbers[numbers_size];
  while(1) {
    ConfigureImu(&serial);
    for (int i = 0; i < 3000; ++i) {
      if (ReadData(&serial, numbers, numbers_size)) {
        DumpData(numbers, numbers_size, producer);
        usleep(1e5);  // Output at approx 10Hz.
      }
    }
  }
  // not reached
}
