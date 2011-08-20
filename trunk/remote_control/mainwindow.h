// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: jpilet@google.com (Julien Pilet)
//
// MainWindow header.

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsPolygonItem>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include "anglecontroller.h"
#include "clientstate.h"
#include "config_dialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow(ClientState* state, QWidget *parent = 0);
  ~MainWindow();

protected:
  void changeEvent(QEvent *e);

private:
  Ui::MainWindow *ui;
  ClientState* state_;
  ConfigDialog config_dialog_;

  QGraphicsScene scene_;
  QGraphicsPolygonItem* boat_;
  QGraphicsPolygonItem* boom_;
  QGraphicsLineItem* target_boom_;
  QGraphicsLineItem* rudder_left_;
  QGraphicsLineItem* rudder_right_;
  QGraphicsLineItem* target_rudder_left_;
  QGraphicsLineItem* target_rudder_right_;
  QGraphicsEllipseItem* compass_;
  QGraphicsLineItem* target_heading_;
  QGraphicsLineItem* wind_;

  AngleController* boom_controller_;
  AngleController* heading_controller_;
  AngleController* rudder_controller_;

  void drawBoat();

public slots:
  void onRudderCtlActivated(double angle);
  void onTargetHeadingRotated(double angle);

private slots:
  void on_actionConnect_triggered();
  void on_actionConfig_triggered();
  void updateView();
  void printText(QString text);

};

#endif // MAINWINDOW_H