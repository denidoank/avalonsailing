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
  QGraphicsLineItem* rudder_left_;
  QGraphicsLineItem* rudder_right_;
  QGraphicsEllipseItem* compass_;

  void drawBoat();

public slots:

private slots:
  void on_actionConnect_triggered();
  void on_actionConfig_triggered();
  void updateView();
  void printText(QString text);

};

#endif // MAINWINDOW_H
