// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: jpilet@google.com (Julien Pilet)
//
// ClientState header. This class fetch data from the boat and remembers it.

#ifndef CLIENTSTATE_H
#define CLIENTSTATE_H

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

signals:

  void dataUpdate();
  void consoleOutput(QString text);

  public slots:
      void setCommand(const QString& command);
  void tryToConnect();

  void gotData();
  void gotErrorData();

  void processError(QProcess::ProcessError error);
  void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
  QString command_;

  QProcess ssh_process_;

  QMap<QString, QMap<QString, QString> > data_;

  QTimer restart_timer_;
};

#endif // CLIENTSTATE_H