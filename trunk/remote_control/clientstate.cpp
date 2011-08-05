// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: jpilet@google.com (Julien Pilet)
//
// ClientState implementation. Implements the application logic.

#include "clientstate.h"

#include <QVariant>

ClientState::ClientState(QObject *parent) :
    QObject(parent)
{
  setCommand("/usr/bin/ssh root@192.168.1.29 plug /var/run/lbus");

  connect(&ssh_process_, SIGNAL(readyReadStandardOutput()), SLOT(gotData()));
  connect(&ssh_process_, SIGNAL(readyReadStandardError()), SLOT(gotErrorData()));
  connect(&ssh_process_, SIGNAL(error(QProcess::ProcessError)),
          SLOT(processError(QProcess::ProcessError)));
  connect(&ssh_process_, SIGNAL(finished(int, QProcess::ExitStatus)),
          SLOT(processFinished(int, QProcess::ExitStatus)));

  connect(&restart_timer_, SIGNAL(timeout()), SLOT(tryToConnect()));
}

void ClientState::setCommand(const QString& command) {
  this->command_ = command;
}

void ClientState::tryToConnect() {
  ssh_process_.setProcessChannelMode(QProcess::SeparateChannels);
  QString& cmd = getCommand();
  ssh_process_.start(cmd);
  emit consoleOutput(QString("Running: ") + cmd);
}

void ClientState::gotData() {
  ssh_process_.setReadChannel(QProcess::StandardOutput);
  while (ssh_process_.canReadLine()) {
    QString line(ssh_process_.readLine(8192));
    if( line.size() <= 0 ) {
      continue;
    }

    QStringList key_vals = line.split(" ", QString::SkipEmptyParts);
    if (key_vals.size() == 0) {
      emit consoleOutput("Can't parse: " + line);
      continue;
    }
    QStringList first = key_vals[0].split(":", QString::SkipEmptyParts);
    if (first.size() != 1) {
      emit consoleOutput("Can't parse: " + line);
      continue;
    }

    QString source = first[0];
    QStringList keys, values;
    for (int i = 1; i < key_vals.size(); ++i) {
      QStringList entry = key_vals[i].split(":", QString::SkipEmptyParts);
      if (entry.size() != 2) {
        if (key_vals[i].trimmed().size() > 0) {
          emit consoleOutput("Can't understand: '" + key_vals[i] + "'");
        }
        continue;
      }
      keys.append(entry[0]);
      data_[source][entry[0]] = entry[1].trimmed();
    }
    emit dataUpdate();
  }
}

void ClientState::gotErrorData() {
  ssh_process_.setReadChannel(QProcess::StandardError);

  QString out;
  while (ssh_process_.canReadLine()) {
    QString line(ssh_process_.readLine(8192));
    if( line.size() > 0 ) {
      out += line;
    }
  }
  emit consoleOutput(out);
}

void ClientState::processError(QProcess::ProcessError error) {
  emit consoleOutput("Process error: " + error);
}

void ClientState::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
  emit consoleOutput("Process finished. Restarting it in 1 second.");
  restart_timer_.setSingleShot(true);
  restart_timer_.start(1000);
}
