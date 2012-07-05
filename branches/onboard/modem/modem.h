// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef MODEM_MODEM_H__
#define MODEM_MODEM_H__
 
#include "lib/util/serial.h"

#include <list>
#include <stdio.h>
#include <string>
#include <time.h>

using namespace std;

// Interface to control AT based modem using RS232 serial port
class Modem {
 public:
  enum ResultCode {
    OK = 0,
    CONNECT = 1,
    RING = 2,
    NO_CARRIER = 3,
    ERROR = 4,
    NO_DIALTONE = 6,
    BUSY = 7,
    NO_ANSWER = 7,
    HARDWARE_FAILURE = 127,
    COMMAND_NOT_SUPPORTED,
    MAX_RESULT_CODE  // Keep this last entry.
  };

  struct SmsMessage {
    int id;
    time_t time;
    string phone_number;
    string text;
  };
    
  Modem();
  ~Modem();

  // Connect to device over serial port
  bool Init(const char *devname);

  // Commands.
  ResultCode GetModemType(string* modem_type);
  ResultCode GetModemSerialNumber(string* modem_sn);
  ResultCode GetNetworkRegistration(bool* registered);
  ResultCode GetSignalQuality(int* signal_strength);
  // Retreive coarse geolocation, if available. Lat/lng are in decimal format.
  ResultCode GetGeolocation(double* lat, double* lng, time_t* timestamp);

  ResultCode ReceiveSMSMessages(list<SmsMessage>* messages);
  ResultCode DeleteSMSMessage(const SmsMessage& message);
  ResultCode SendSMSMessage(const string phone_number, const string message);

  // Idle loop to process asyncronous status messages from the modem.
  void IdleLoop();

 private:
  Serial serial_;
  list<string> info_;
  list<string> async_status_;

  // Send command.
  ResultCode SendCommand(const char* command);

  // Wait for command result code.
  ResultCode GetStatus(const int timeout_ms);
};

#endif  // MODEM_MODEM_H__
