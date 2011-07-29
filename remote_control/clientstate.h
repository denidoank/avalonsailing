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

  Q_PROPERTY(float rudder_l_deg READ get_rudder_l_deg WRITE set_rudder_l_deg NOTIFY rudder_l_deg_changed)
  Q_PROPERTY(float rudder_r_deg READ get_rudder_r_deg WRITE set_rudder_r_deg NOTIFY rudder_r_deg_changed)
  Q_PROPERTY(float sail_deg READ get_sail_deg WRITE set_sail_deg NOTIFY sail_deg_changed)

public:
  explicit ClientState(QObject *parent = 0);

  QString& getCommand() { return command_; }

  const QMap<QString, QMap<QString, QString> >& getData() const { return data_; }

  void updateProperty(const QString& key, const QString& val);

  float get_rudder_l_deg() const { return rudder_l_deg_; }
  void set_rudder_l_deg(float angle) { rudder_l_deg_ = angle; emit rudder_l_deg_changed(angle); }
  float get_rudder_r_deg() const { return rudder_r_deg_; }
  void set_rudder_r_deg(float angle) { rudder_r_deg_ = angle; emit rudder_r_deg_changed(angle); }
  float get_sail_deg() const { return sail_deg_; }
  void set_sail_deg(float angle) { sail_deg_ = angle; emit sail_deg_changed(angle); }

signals:

  void dataUpdate();
  void consoleOutput(QString text);

  void rudder_l_deg_changed(float angle);
  void rudder_r_deg_changed(float angle);
  void sail_deg_changed(float angle);

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
  float rudder_r_deg_, rudder_l_deg_, sail_deg_;
};

#endif // CLIENTSTATE_H
