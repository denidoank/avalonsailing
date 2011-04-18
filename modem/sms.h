// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
#ifndef MODEM_SMS_H_
#define MODEM_SMS_H_

#include <time.h>

/* 
 * Encode an SMS message. Output the encoded message into output buffer.
 * Returns the length of the SMS encoded message in the output buffer or
 * a negative number in case encoding failed (for example provided output buffer
 * does not have enough space).
 */
int EncodeSMS(const char* service_center_number, const char* phone_number,
              const char* sms_text, const int sms_text_length,
              unsigned char* output_buffer, const int buffer_size);

/* 
 * Decode an SMS message. Output the decoded message into the sms text buffer.
 * Returns the length of the SMS dencoded message or a negative number in
 * case encoding failed (for example provided output buffer has not enough
 * space).
 */
int DecodeSMS(const unsigned char* buffer, const int buffer_size,
              time_t* output_sms_time, char* output_sender_phone_number,
              const int sender_phone_number_size, char* output_sms_text,
              const int sms_text_length);

#endif   // MODEM_SMS_H_
