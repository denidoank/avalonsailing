// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: jpilet@google.com (Julien Pilet)
//
// Implementation of the MainWindow behavior.

#include <math.h>

#include <QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(ClientState* state, QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow), state_(state), config_dialog_(state, this)
{
  ui->setupUi(this);
  drawBoat();

  connect(state_, SIGNAL(dataUpdate()), SLOT(updateView()));
  connect(state_, SIGNAL(consoleOutput(QString)), SLOT(printText(QString)));
  connect(state_, SIGNAL(rudder_l_deg_changed(float)), SLOT(setRudderLeftAngle(float)));
  connect(state_, SIGNAL(rudder_r_deg_changed(float)), SLOT(setRudderRightAngle(float)));
  connect(state_, SIGNAL(sail_deg_changed(float)), SLOT(setBoomAngle(float)));
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
  QMainWindow::changeEvent(e);
  switch (e->type()) {
    case QEvent::LanguageChange:
      ui->retranslateUi(this);
      break;
    default:
      break;
  }
}

void MainWindow::on_actionConfig_triggered()
{
  config_dialog_.show();
}

void MainWindow::on_actionConnect_triggered()
{
  state_->tryToConnect();
}

void MainWindow::updateView() {
  QString text;
  const QMap<QString, QMap<QString,QString> >& map = state_->getData();
  for (QMap<QString, QMap<QString, QString> >::const_iterator source = map.constBegin(); source != map.constEnd(); ++source) {
    const QMap<QString,QString>& pairs = source.value();
    text += source.key() + ":\n";
    for (QMap<QString, QString>::const_iterator i = pairs.constBegin(); i != pairs.constEnd(); ++i) {
      text += "    " + i.key() + ": " + i.value() + "\n";
    }
  }
  ui->dataview->setPlainText(text);
}

void MainWindow::printText(QString text) {
  ui->console->append(text);
}

void MainWindow::drawBoat() {
  QPen black_border(QColor(0,0,0));

  QPolygonF boat_poly;
  boat_poly << QPointF(0, -5)  // tip of the boat
    // starboard side
    <<  QPointF(2, 0) <<  QPointF(2, 2) <<  QPointF(1.5, 6)
    // port side
    <<  QPointF(-1.5, 6) <<  QPointF(-2, 2) <<  QPointF(-2, 0);

  boat_ = scene_.addPolygon(boat_poly, black_border);
  boom_ = new QGraphicsPolygonItem(boat_);

  QPolygonF boom_poly;
  float width = .3;
  boom_poly << QPointF(0, -2)
          << QPointF(sqrt(.5), -sqrt(.5) - 1)
          << QPointF(1, -1)
          << QPointF(sqrt(.5), sqrt(.5) - 1)
          << QPointF(width, sqrt(.5) - 1)
          << QPointF(width, 4)
          << QPointF(0, 5.2 - 1)
          << QPointF(-width, 4)
          << QPointF(-width, sqrt(.5) - 1)
          << QPointF(-sqrt(.5), sqrt(.5) - 1)
          << QPointF(-1, -1)
          << QPointF(-sqrt(.5), -sqrt(.5) - 1);
  boom_->setPolygon(boom_poly);
  boom_->setPen(black_border);
  boom_->setPos(0, -1);
  boom_->setRotation(30);

  rudder_left_ = new QGraphicsLineItem(0, 0, 0, 2, boat_, &scene_);
  rudder_left_->setPos(-1, 6);
  rudder_left_->setPen(black_border);

  rudder_right_ = new QGraphicsLineItem(0, 0, 0, 2, boat_, &scene_);
  rudder_right_->setPos(1, 6);
  rudder_right_->setPen(black_border);

  ui->graphicsView->setScene(&scene_);
  //ui->graphicsView->setSceneRect(-16, -16, 16, 16);
  ui->graphicsView->scale(20, 20);
}

void MainWindow::setBoomAngle(float angle) {
  boom_->setRotation(angle);
  ui->graphicsView->update();
}

void MainWindow::setRudderLeftAngle(float angle) {
  rudder_left_->setRotation(angle);
  ui->graphicsView->update();
}

void MainWindow::setRudderRightAngle(float angle) {
  rudder_right_->setRotation(angle);
  ui->graphicsView->update();
}
