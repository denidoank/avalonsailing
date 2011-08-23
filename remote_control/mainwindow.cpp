// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: jpilet@google.com (Julien Pilet)
//
// Implementation of the MainWindow behavior.

#include <math.h>

#include <QDebug>
#include <QGraphicsTextItem>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../proto/rudder.h"
#include "../proto/helmsman.h"

MainWindow::MainWindow(ClientState* state, QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow), state_(state), config_dialog_(state, this)
{
  ui->setupUi(this);
  drawBoat();

  connect(state_, SIGNAL(dataUpdate()), SLOT(updateView()));
  connect(state_, SIGNAL(consoleOutput(QString)), SLOT(printText(QString)));
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
  for (QMap<QString, QMap<QString, QString> >::const_iterator source = map.constBegin();
      source != map.constEnd(); ++source) {
    const QMap<QString,QString>& pairs = source.value();
    text += source.key() + ":\n";
    for (QMap<QString, QString>::const_iterator i = pairs.constBegin();
        i != pairs.constEnd(); ++i) {
      text += "    " + i.key() + ": " + i.value() + "\n";
    }
  }
  ui->dataview->setPlainText(text);

  // Update the drawing.

  compass_->setRotation(-state_->getDouble("imu", "yaw_deg"));
  target_heading_->setRotation(state_->getDouble("helm", "alpha_star_deg"));
  wind_->setRotation(state_->getDouble("wind", "angle_deg"));

  boom_->setRotation(state_->getDouble("ruddersts", "sail_deg"));
  target_boom_->setRotation(state_->getDouble("rudderctl", "sail_deg"));
  rudder_left_->setRotation(state_->getDouble("ruddersts", "rudder_l_deg"));
  rudder_right_->setRotation(state_->getDouble("ruddersts", "rudder_r_deg"));
  target_rudder_left_->setRotation(state_->getDouble("rudderctl", "rudder_l_deg"));
  target_rudder_right_->setRotation(state_->getDouble("rudderctl", "rudder_r_deg"));

  ui->graphicsView->update();
}

void MainWindow::printText(QString text) {
  ui->console->append(text);
}

void MainWindow::drawBoat() {
  QPen black_border(QColor(0,0,0));

  QPolygonF boat_poly;
  boat_poly << QPointF(0, -100)  // tip of the boat
    // starboard side
    <<  QPointF(40, 0) <<  QPointF(40, 40) <<  QPointF(30, 120)
    // port side
    <<  QPointF(-30, 120) <<  QPointF(-40, 40) <<  QPointF(-40, 0);

  boat_ = scene_.addPolygon(boat_poly, black_border);
  boom_ = new QGraphicsPolygonItem(boat_);

  QPolygonF boom_poly;
  const float width = 6;
  const float boom_radius = 20;
  const float sqrt_r2(sqrt(.5)*boom_radius);
  boom_poly << QPointF(0, -40)
          << QPointF(sqrt_r2, -sqrt_r2 - 20)
          << QPointF(20, -20)
          << QPointF(sqrt_r2, sqrt_r2 - 20)
          << QPointF(width, sqrt_r2 - 20)
          << QPointF(width, 80)
          << QPointF(0, 104 - 20)
          << QPointF(-width, 80)
          << QPointF(-width, sqrt_r2 - 20)
          << QPointF(-sqrt_r2, sqrt_r2 - 20)
          << QPointF(-20, -20)
          << QPointF(-sqrt_r2, -sqrt_r2 - 20);
  boom_->setPolygon(boom_poly);
  boom_->setPen(black_border);
  boom_->setPos(0, -20);

  wind_ = new QGraphicsLineItem(0, 0, 0, -30, boom_, &scene_);
  wind_->setPos(0, -20);
  QPen wind_pen(QColor(0, 99, 0));
  wind_pen.setWidth(2);
  wind_->setPen(wind_pen);

  target_boom_ = new QGraphicsLineItem(0,0, 0, 60, boat_, &scene_);
  QPen target_pen(QColor(255,0,0));
  target_pen.setWidth(2);
  target_pen.setStyle(Qt::DashLine);
  target_boom_->setPen(target_pen);
  target_boom_->setPos(0, -20);

  boom_controller_ = new AngleController(QPointF(0, 60), 5, boat_);
  boom_controller_->setPos(0, -20);
  connect(boom_controller_, SIGNAL(turned(double)), SLOT(onRudderCtlActivated(double)));

  rudder_left_ = new QGraphicsLineItem(0, 0, 0, 40, boat_, &scene_);
  rudder_left_->setPos(-20, 120);
  rudder_left_->setPen(black_border);
  target_rudder_left_ = new QGraphicsLineItem(0, 0, 0, 40, boat_, &scene_);
  target_rudder_left_->setPos(-20, 120);
  target_rudder_left_->setPen(target_pen);

  rudder_right_ = new QGraphicsLineItem(0, 0, 0, 40, boat_, &scene_);
  rudder_right_->setPos(20, 120);
  rudder_right_->setPen(black_border);
  target_rudder_right_ = new QGraphicsLineItem(0, 0, 0, 40, boat_, &scene_);
  target_rudder_right_->setPos(20, 120);
  target_rudder_right_->setPen(target_pen);

  rudder_controller_ = new AngleController(QPointF(0, 40), 5, boat_);
  rudder_controller_->setPos(20,120);
  rudder_controller_->setBounds(-30, 30);
  connect(rudder_controller_, SIGNAL(turned(double)), SLOT(onRudderCtlActivated(double)));


  // Draw the compass.
  compass_ = new QGraphicsEllipseItem(-40, -40, 80, 80);
  scene_.addItem(compass_);
  QGraphicsTextItem* north = new QGraphicsTextItem("N", compass_);
  north->setPos(-north->boundingRect().width()/2, -40);

  QGraphicsTextItem* east = new QGraphicsTextItem("E", compass_);
  east->setRotation(90);
  east->setPos(40, -east->boundingRect().width()/2);

  QGraphicsTextItem* south = new QGraphicsTextItem("S", compass_);
  south->setRotation(180);
  south->setPos(south->boundingRect().width()/2, 40);

  QGraphicsTextItem* west = new QGraphicsTextItem("W", compass_);
  west->setRotation(-90);
  west->setPos(-40, west->boundingRect().width()/2);
  new QGraphicsLineItem(0, -20, 0, 20, compass_, &scene_);
  new QGraphicsLineItem(-20, 0, 20, 0, compass_, &scene_);

  compass_->setPos(-160, -100);
  compass_->setRotation(45);
  target_heading_ = new QGraphicsLineItem(0, 0, 0, -60, compass_, &scene_);
  target_heading_->setPen(target_pen);
  heading_controller_ = new AngleController(QPointF(0, -60), 5, compass_);
  connect(heading_controller_, SIGNAL(turned(double)), SLOT(onTargetHeadingRotated(double)));

  ui->graphicsView->setScene(&scene_);
}

void MainWindow::onRudderCtlActivated(double) {
  RudderProto command;
  command.timestamp_ms = 0;
  command.rudder_l_deg = rudder_controller_->rotation();
  command.rudder_r_deg = command.rudder_l_deg;
  command.sail_deg = boom_controller_->rotation();

  size_t BufSize = 256;
  char buf[BufSize];
  int length;
  ::snprintf(buf, BufSize, OFMT_RUDDERPROTO_CTL(command, &length));
  state_->writeToBus(buf);
}

void MainWindow::onTargetHeadingRotated(double angle) {
  HelmsmanCtlProto proto;
  proto.timestamp_ms = 0;
  proto.alpha_star_deg = angle;

  size_t BufSize = 256;
  char buf[BufSize];
  int length;
  ::snprintf(buf, BufSize, OFMT_HELMSMANCTLPROTO(proto, &length));
  state_->writeToBus(buf);
}
