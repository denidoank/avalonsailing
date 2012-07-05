// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: jpilet@google.com (Julien Pilet)
//
// ClientState header. This class fetch data from the boat and remembers it.

#ifndef CLIENTSTATE_H
#define CLIENTSTATE_H

#include <math.h>
#include <QObject>
#include <QString>
#include <QProcess>
#include <QMap>
#include <QTimer>

class ClientState : public QObject
{
  Q_OBJECT

public:
  explicit ClientState(QObject *parent = 0);

  QString& getCommand() { return command_; }

  const QMap<QString, QMap<QString, QString> >& getData() const { return data_; }

  QString getVar(const QString& daemon, const QString& var) const {
        return data_.value(daemon).value(var);
  }

  double getDouble(const QString& daemon, const QString& var) const {
    QString value = getVar(daemon, var);
    if (value.isEmpty() || value == "nan") {
      return 0;
    } else {
      return value.toDouble();
    }
  }

  double getDoubleOrNan(const QString& daemon, const QString& var) const {
    QString value = getVar(daemon, var);
    if (value.isEmpty() || value == "nan") {
      return NAN;
    } else {
      return value.toDouble();
    }
  }

  double getLong(const QString& daemon, const QString& var) const {
    QString value = getVar(daemon, var);
    if (value.isEmpty() || value == "nan") {
      return 0;
    } else {
      return value.toLong();
    }
  }

  void writeToBus(const char* data);

signals:

  void dataUpdate();
  void consoleOutput(QString text);
  void connectionWantedChanged(bool connection_wanted);

public slots:
  void setCommand(const QString& command);
  void tryToConnect();
  void disconnect();

  void gotData();
  void gotErrorData();

  void processError(QProcess::ProcessError error);
  void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
  void processLine(const QString& line);

  QString command_;
  QProcess ssh_process_;
  QMap<QString, QMap<QString, QString> > data_;
  QTimer restart_timer_;
  bool connection_wanted_;
  uint update_counter_;
};

#endif // CLIENTSTATE_H
