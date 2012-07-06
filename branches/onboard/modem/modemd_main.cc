// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
//
// Communication daemon for sending/receiving status messages and commands by SMS
// over the Irridium modem.

#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <sys/time.h>

#include <string>

#include "message-queue.h"
#include "modem.h"
#include "proto/modem.h"

using namespace std;

namespace {

const char* argv0;
int verbose = 0;
int debug = 0;

void crash(const char* fmt, ...) {
	va_list ap;
	char buf[1000];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	syslog(LOG_CRIT, "%s%s%s\n", buf,
	       (errno) ? ": " : "",
	       (errno) ? strerror(errno):"" );
	exit(1);
	va_end(ap);
	return;
}

void fault(int) { crash("fault"); }

int64_t now_ms() {
        struct timeval tv;
        if (gettimeofday(&tv, NULL) < 0) crash("no working clock");

        int64_t ms1 = tv.tv_sec;  ms1 *= 1000;
        int64_t ms2 = tv.tv_usec; ms2 /= 1000;
        return ms1 + ms2;
}

void usage(void) {
	fprintf(stderr,
		"usage: %s [options] /path/to/modemdevice \n"
		"options:\n"
		"\t-d debug   (don't go daemon)\n"
		"\t-p phonenr destination for outgoing sms'es (no leading +)\n"
		"\t-q queue   queue dir (/tmp/modem)\n"
		, argv0);
	exit(2);
}

}  // anonymous namespace

int main(int argc, char* argv[]) {

	string modem_device = "/dev/modem";
	string phone_number = "41798300890";
	string queue = "/tmp/modem";

	int ch;

	argv0 = strrchr(argv[0], '/');
	if (argv0) ++argv0; else argv0 = argv[0];

	while ((ch = getopt(argc, argv, "dhp:q:v")) != -1){
		switch (ch) {
		case 'd': ++debug; break;
		case 'v': ++verbose; break;
		case 'p': phone_number.assign(optarg); break;
		case 'q': queue.assign(optarg); break;
		case 'h':
		default:
			usage();
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 1) usage();

	modem_device.assign(argv[0]);

	signal(SIGBUS, fault);
        signal(SIGSEGV, fault);

	openlog(argv0, debug?LOG_PERROR:0, LOG_DAEMON);
	if(!debug) setlogmask(LOG_UPTO(LOG_NOTICE));

	MessageQueue inbox(queue + "/in");
	MessageQueue outbox(queue + "/out");

	for (;;) {
		Modem m;
		if (!m.Init(modem_device.c_str())) {
			syslog(LOG_ERR, "Cannot initialize the modem: %s", modem_device.c_str());
			break;
		}
		string modem_type, modem_sn;

		// Check that modem is alive by issuing a simple command.
		int retries = 10;
		for (retries = 10; retries; --retries) {
			if (m.GetModemType(&modem_type) == Modem::OK && m.GetModemSerialNumber(&modem_sn) == Modem::OK)
				break;
			sleep(3);
		}
		if (retries == 0) {
			syslog(LOG_ERR, "Modem not responsive");
			break;
		}
		syslog(LOG_INFO, "Found modem: %s SN: %s", modem_type.c_str(), modem_sn.c_str());

		// Check network registration and signal quality.
		bool nr = false;
		int sq = -1;
		if (m.GetNetworkRegistration(&nr) != Modem::OK) {
			syslog(LOG_ERR, "Checking network registration failed!");
		} else {
			if (m.GetSignalQuality(&sq) != Modem::OK)
				syslog(LOG_ERR, "Checking signal quality failed!");
			else
				syslog(LOG_INFO, "Iridium %sregistered, signal quality: %d", nr?"":"NOT ", sq);
		}

		// Send outbox messages.
		retries = 10;
		syslog(LOG_INFO, "Messages in the outbox queue: %d", outbox.NumMessages());
		while (outbox.NumMessages() && retries) {
			// Send SMS messages:
			string message;
			const MessageQueue::MessageId id = outbox.GetMessage(0, &message);
			if (id != MessageQueue::kInvalidId) {  // Read successfull.
				syslog(LOG_INFO, "Sending SMS: \"%s\" to phone number: %s ...\n", message.c_str(), phone_number.c_str());
				Modem::ResultCode rc = m.SendSMSMessage(phone_number, message);
				if (rc != Modem::OK) {
					syslog(LOG_ERR, "SMS sending error (%d)!\n", rc);
					retries--;
				} else {
					syslog(LOG_INFO, "SMS sending successfull.\n");
					outbox.DeleteMessage(id);  // Message sent. Delete it.
				}
			}
		}
		if (retries == 0) {
			syslog(LOG_ERR, "Failed to send SMS messages. Check network or signal!");
		}

		// Check for new SMS messages.
		list<Modem::SmsMessage> messages;
		if (m.ReceiveSMSMessages(&messages) == Modem::OK) {
			syslog(LOG_INFO, "Received SMS messages: %d messages.", (int)messages.size());
		} else {
			syslog(LOG_ERR, "Receive SMS messages: command failed!");
		}
		while (!messages.empty()) {
			syslog(LOG_INFO, "Received SMS Message time=%d, phone=%s text=\"%s\"\n",
			       static_cast<int>(messages.front().time),
			       messages.front().phone_number.c_str(),
			       messages.front().text.c_str());
			const MessageQueue::MessageId id =
				inbox.PushMessage(messages.front().text);
			if (id != MessageQueue::kInvalidId) {
				m.DeleteSMSMessage(messages.front());
			}
			messages.pop_front();        
		}

		ModemProto status = INIT_MODEMPROTO;
		if (m.GetGeolocation(&status.lat_deg, &status.lng_deg, &status.timestamp_ms) != Modem::OK) {
			syslog(LOG_ERR, "Error retrieving geolocation!");
		} else {
			printf(OFMT_MODEMPROTO(status));
		}

		// Idle loop.
		m.IdleLoop();
		sleep(1);
	}
	syslog(LOG_INFO, "Modem daemon terminated.");
}
