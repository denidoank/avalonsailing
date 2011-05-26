// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#include "lib/fm/log.h"
#include "lib/util/reader.h"
#include "modem/modem.h"
#include "modem/sms.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// Modem timeout set to 10 seconds.
#define MODEM_TIMEOUT_MILLISECS 10000

// First command that should be sent out to modem:
// Disable echo (E0), disable loudspeaker (L0), disable auto-answer (S0=0),
// enable command response (Q0), numeric responses (V0),
// extended result code (X4), DCD indicates the connection status (&C1).
#define MODEM_INIT_COMMAND "AT E0 L0 S0=0 Q0 V0 X4 &C1 +CREG=2\r\n"

// Maximum size of the encoded SMS PDU message.
#define MAX_SMS_PDU_LENGTH 256

Modem::Modem() {
}

Modem::~Modem() {
}

bool Modem::Init(const char *devname) {
  if (!serial_.Init(devname, B9600)) {
    FM_LOG_ERROR("Intializing serial port %s failed.", devname);
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
      if (isdigit(response[0])) {  // Check if this is a numeric response code.
        char* endptr = NULL;
        status = strtol(response, &endptr, 10);
        if (*endptr == '\0' && status >= OK && status < MAX_RESULT_CODE &&
            status != RING) {
          return static_cast<ResultCode>(status);
        }
      }
      if (status == RING || response[0] == '+' || response[0] == '^') {
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

Modem::ResultCode Modem::SendSMSMessage(const string phone_number,
                                        const string message) {
  unsigned char pdu[MAX_SMS_PDU_LENGTH];
  int pdu_length = EncodeSMS("", phone_number.c_str(), message.c_str(),
                             message.length(), pdu, sizeof(pdu));
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
  ResultCode result = SendCommand("AT+CMGL=1");
  while (!async_status_.empty()) {
    // Parse "+CMGL: " not required. We parse info to retrieve valid SMS-es.
    async_status_.pop_front();
  };
  while (!info_.empty()) {
    time_t sms_time;
    char sender_phone_number[32];
    unsigned char pdu[MAX_SMS_PDU_LENGTH];
    char sms_text[161];
    for (int i = 0; i < info_.front().length(); i += 2) {
      pdu[i / 2] = strtol(info_.front().substr(i, 2).c_str(), NULL, 16);
    }
    int sms_length = DecodeSMS(pdu, info_.front().length() / 2, &sms_time,
                               sender_phone_number, sizeof(sender_phone_number),
                               sms_text, sizeof(sms_text));
    if (sms_length > 0) {  // Valid SMS received.
      SmsMessage message;
      message.time = sms_time;
      message.phone_number = sender_phone_number;
      message.text = sms_text;
      messages->push_back(message);
    }

    info_.pop_front();
  };
}

void Modem::IdleLoop() {
  GetStatus(0);  // Use no timeout. Should always return ERROR.
  while (!async_status_.empty()) {
    // TODO: process asyncronous messages such as:
    // - network registration status change.
    // - network location changes.
    FM_LOG_DEBUG("asynchronous status: %s", async_status_.front().c_str());
    async_status_.pop_front();
  }
  while (!info_.empty()) {
    // TODO: process asyncronous messages such as:
    // - network registration status change.
    // - network location changes.
    FM_LOG_DEBUG("info: %s", info_.front().c_str());
    info_.pop_front();
  }
}
