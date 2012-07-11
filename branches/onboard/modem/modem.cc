// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "reader.h"
#include "modem.h"
#include "sms.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

// Modem timeout set to 10 seconds.
#define MODEM_TIMEOUT_MILLISECS 10000

// First command that should be sent out to modem:
// Disable echo (E0), disable loudspeaker (L0), disable auto-answer (S0=0),
// enable command response (Q0), numeric responses (V0),
// extended result code (X4), DCD indicates the connection status (&C1).
#define MODEM_INIT_COMMAND "AT E0 L0 S0=0 Q0 V0 X4 &C1 +CREG=2\r\n"

// Maximum size of the encoded SMS PDU message.
#define MAX_SMS_PDU_LENGTH 256

// Iridium time shift compared with UNIX time.

// Iridium system time epoch: March 8, 2007, 03:50:21.00 GMT. 
// (Note: the original Iridium system time epoch was June 1, 1996, 00:00:11 GMT, and was reset to the new epoch in January, 2008).
// #define IRIDIUM_SYSTEM_TIME_SHIFT_S 833587211  //  June 1, 1996, 00:00:11 GMT on 
#define IRIDIUM_SYSTEM_TIME_SHIFT_S 1173325821  //  Thu Mar  8 03:50:21 UTC 2007

// Extract a comma separated list of integers from a string.
// The list can be comma separated and may have quotes. Example: 2, "6",005.
// The base vector is specifying how many integers are expected to be parsed and
// corresponding base (0 means autodetect base).
vector<int> get_int_list(const string& str, const vector<int>& base) {
  vector<int> ints;
  const char* ptr = str.c_str();
  while (*ptr == ' ' || *ptr == '"') ptr++;  // Skip spaces.
  for (unsigned int i = 0; i < base.size() && ptr != '\0'; ++i) {
    char* endptr;
    int n = strtol(ptr, &endptr, base[i]);
    if (endptr == ptr) break;  // Failed to get extract a number.
    ints.push_back(n);
    while (*endptr == ' ' || *endptr ==',' || *endptr == '"') endptr++;
    ptr = endptr;  // Move to next integer.
  }
  return ints;
}


bool Modem::Init(const char *devname) {
  if (!serial_.Init(devname, B9600)) {
    syslog(LOG_ERR, "Intializing serial port %s failed.", devname);
    return false;
  }
  // Initialize modem.
  return SendCommand(MODEM_INIT_COMMAND) == OK;
}

Modem::ResultCode Modem::SendCommand(const char* command) {
  serial_.Printf("%s\r", command);
  return GetStatus(MODEM_TIMEOUT_MILLISECS);
}

Modem::ResultCode Modem::GetStatus(const int timeout_ms) {
  char buffer[1024];
  info_.clear();
  while (true) {
    // We expect the command to finalize with a status number, error code or
    // descriptor or OK string. Before completion, one or more asyncronous or
    // information messages might be received.
    buffer[0] = '\0';
    int err = serial_.In().ReadLine(buffer, sizeof(buffer), timeout_ms, '\r');
    if (err == Reader::READ_OK) {
      char* response = buffer;
      if (buffer[0] == '\n') {  // Ignore new line, if present.
        response++;
      }
      int status = -1;
      if (!strncmp(response, "OK", 2))
        return OK;
      if (!strncmp(response , "COMMAND NOT SUPPORT", 19))
        return COMMAND_NOT_SUPPORTED;
      if (isdigit(response[0])) {  // Check if this is a numeric response code.
        char* endptr = NULL;
        status = strtol(response, &endptr, 10);
        if (*endptr == '\0' && status >= OK && status < MAX_RESULT_CODE &&
            status != RING) {
          return static_cast<ResultCode>(status);
        }
      }
      if (status == RING || response[0] == '-' || response[0] == '+' ||
          response[0] == '^') {
        // Got verbose response code.
        if (!strncmp(response, "+CME ERROR", 10) ||
            !strncmp(response, "+CMS ERROR", 10))
          return ERROR;
        async_status_.push_back(response);
      } else {  // Got more information
        info_.push_back(response);
      }
    } else {
      break;
    }
  }
  return ERROR;
}

Modem::ResultCode Modem::GetModemType(string* modem_type) {
  ResultCode result = SendCommand("AT+CGMI");
  if (result == OK && !info_.empty()) {
    *modem_type = info_.back();
  }
  return result;
}

Modem::ResultCode Modem::GetModemSerialNumber(string* modem_sn) {
  ResultCode result = SendCommand("AT+CGSN");
  if (result == OK && !info_.empty()) {
    (*modem_sn) = info_.back();
  }
  return result;
}

Modem::ResultCode Modem::GetNetworkRegistration(bool* registered) {
  ResultCode result = SendCommand("AT+CREG?");
  if (result == OK) {
    *registered = false;
    while (!async_status_.empty()) {
      if (async_status_.front().substr(0, 6) == "+CREG:") {
        vector<int> ints = get_int_list(async_status_.front().substr(6), vector<int>(2, 10));
        if (ints.size() >= 2 && (ints[1] == 1 || ints[1] == 5)) {
          // Registered home network (1) or roaming (5).
          *registered = true;
        }
      }
      async_status_.pop_front();
    }
  }
  return result;  
}

Modem::ResultCode Modem::GetSignalQuality(int* signal_quality) {
  ResultCode result = SendCommand("AT+CSQ");
  if (result == OK) {
    *signal_quality = 0;
    while (!async_status_.empty()) {
      if (async_status_.front().substr(0, 5) == "+CSQ:") {
        vector<int> ints = get_int_list(async_status_.front().substr(5), vector<int>(1, 10));
        if (ints.size() >= 1)
	  *signal_quality = ints[0];
      }
      async_status_.pop_front();
    }
  }
  return result;  
}

Modem::ResultCode Modem::GetGeolocation(double* lat, double* lng, int64_t* timestamp_ms) {
  ResultCode result = SendCommand("AT-MSGEO");
  if (result == OK) {
    while (!async_status_.empty()) {
      *lat = *lng = 0.0;
      *timestamp_ms = 0;
      if (async_status_.front().substr(0, 7) == "-MSGEO:") {
        vector<int> base(3, 10);  // Expect 3 integers for x, y, z.
        base.push_back(16);  // and one hexa timestamp.
        vector<int> ints = get_int_list(async_status_.front().substr(7), base);
        if (ints.size() == 4) {
          *lat = atan2(ints[2], sqrt(ints[0] * ints[0] + ints[1] * ints[1])) * (180.0 / M_PI);
          *lng = atan2(ints[1], ints[0]) * (180.0 / M_PI);
          // Iridium clock runs on 90ms frequency.
          *timestamp_ms = ints[3] * 90LL + (IRIDIUM_SYSTEM_TIME_SHIFT_S * 1000LL);
        }
      }
      async_status_.pop_front();
    }
  }
  return result;  

}

Modem::ResultCode Modem::SendSMSMessage(const string phone_number, const string message) {
  unsigned char pdu[MAX_SMS_PDU_LENGTH];
  int pdu_length = EncodeSMS("", phone_number.c_str(), message.c_str(), message.length(), pdu, sizeof(pdu));
  if (pdu_length <= 0)
    return ERROR;
  // User data length equals with PDU length, except the SMSC.
  const int pdu_length_except_smsc = pdu_length - 1 - pdu[0];
  serial_.Printf("AT+CMGS=%d\r\n", pdu_length_except_smsc);

  // Expect PDU mode initializer "> ".
  char buffer[MAX_SMS_PDU_LENGTH * 2 + 1];  // Each PDU byte has 2 digits.
  int err = 0;
  err = serial_.In().PeekChar(buffer, MODEM_TIMEOUT_MILLISECS);
  while (err == Reader::READ_OK && (buffer[0] == '\r' || buffer[0] == '\n')) {
    err = serial_.In().ReadChar(buffer, MODEM_TIMEOUT_MILLISECS);  // '\r', '\n'
    err = serial_.In().PeekChar(buffer, MODEM_TIMEOUT_MILLISECS);
  }
  if (err != Reader::READ_OK) {
    return ERROR;
  }
  if (buffer[0] != '>') {  // PDU mode error. Expected "> ".
    return GetStatus(MODEM_TIMEOUT_MILLISECS);  // Not in PDU mode.
  }
  err = serial_.In().ReadChar(buffer, MODEM_TIMEOUT_MILLISECS);  // '>'
  err = serial_.In().ReadChar(buffer, MODEM_TIMEOUT_MILLISECS);  // ' '
  if (buffer[0] != ' ') {  // PDU mode error. Expected "> ".
    return GetStatus(MODEM_TIMEOUT_MILLISECS);  // Not in PDU mode.
  }

  // In PDU mode. Build PDU message.
  for (int i = 0; i < pdu_length; ++i) {
    sprintf(buffer + i * 2, "%02X", pdu[i]);
  }
  serial_.Printf("%s\032\r\n", buffer);  // End PDU mode with Ctrl-Z.

  return GetStatus(MODEM_TIMEOUT_MILLISECS);
}

Modem::ResultCode Modem::ReceiveSMSMessages(list<SmsMessage>* messages) {
  // Command: 0=REC-UNREAD, 1=REC-READ, 2=STO-UNSENT, 3=STO-SENT, 4=ALL
  ResultCode result = SendCommand("AT+CMGL=4");
  list<int> ids;
  while (!async_status_.empty()) {
    string info = async_status_.front();
    if (info.substr(0, 6) == "+CMGL:") {
      // Parse "+CMGL: id,..." and extract message id.
      const string id_str = info.substr(6, info.find(',') - 6);
      const int id = strtol(id_str.c_str(), NULL, 10);
      ids.push_back(id);
    }
    async_status_.pop_front();
  };
  while (!info_.empty()) {
    time_t sms_time;
    char sender_phone_number[32];
    unsigned char pdu[MAX_SMS_PDU_LENGTH];
    char sms_text[161];
    for (unsigned int i = 0; i < info_.front().length(); i += 2) {
      pdu[i / 2] = strtol(info_.front().substr(i, 2).c_str(), NULL, 16);
    }
    int sms_length = DecodeSMS(pdu, info_.front().length() / 2, &sms_time,
                               sender_phone_number, sizeof(sender_phone_number),
                               sms_text, sizeof(sms_text));
    if (sms_length > 0) {  // Valid SMS received.
      SmsMessage message;
      if (ids.empty()) {
        syslog(LOG_ERR, "Invalid message id!");
        return ERROR;
      }
      message.id = ids.front();
      ids.pop_front();
      message.time = sms_time;
      message.phone_number = sender_phone_number;
      message.text.assign(sms_text, sms_length);
      messages->push_back(message);
    }

    info_.pop_front();
  }
  return result;
}

Modem::ResultCode Modem::DeleteSMSMessage(const SmsMessage& message) {
  char cmd[128];
  sprintf(cmd, "AT+CMGD=%d", message.id);
  syslog(LOG_DEBUG, "deleting SMS message id: %d", message.id);
  return SendCommand(cmd);
}

void Modem::IdleLoop() {
  GetStatus(0);  // Use no timeout. Should always return ERROR.
  while (!async_status_.empty()) {
    // TODO: process asyncronous messages such as:
    // - network registration status change.
    // - network location changes.
    syslog(LOG_DEBUG, "asynchronous status: %s", async_status_.front().c_str());
    async_status_.pop_front();
  }
  while (!info_.empty()) {
    // TODO: process asyncronous messages such as:
    // - network registration status change.
    // - network location changes.
    syslog(LOG_DEBUG, "info: %s", info_.front().c_str());
    info_.pop_front();
  }
}
