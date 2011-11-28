// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: jpilet@google.com (Julien Pilet)
//
// Implementation of the MainWindow behavior.

#include <math.h>

#include <QDebug>
#include <QGraphicsTextItem>
#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QKeyEvent>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../proto/rudder.h"
#include "../proto/helmsman.h"
#include "../proto/remote.h"

MainWindow::MainWindow(ClientState* state, QWidget *parent) :
  QMainWindow(parent),
    ui(new Ui::MainWindow), state_(state), config_dialog_(state, this), scroll_pos_(0, 0)
{
  ui->setupUi(this);
  drawBoat();

  connect(state_, SIGNAL(dataUpdate()), SLOT(updateView()));
  connect(state_, SIGNAL(consoleOutput(QString)), SLOT(printText(QString)));
  ui->actionConnect->connect(state_, SIGNAL(connectionWantedChanged(bool)),
                             SLOT(setChecked(bool)));

  connect(&update_timer_, SIGNAL(timeout()), SLOT(updateGraphics()));
  update_timer_.setInterval(50);
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

void MainWindow::updateView() {
  // Stop updating the text when scrolling. It is less perturbating, both for qt
  // and for users.
  if (!ui->dataview->verticalScrollBar()->isSliderDown()
      && !ui->dataview->verticalScrollBar()->isSliderDown()) {
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
    int original_scroll = ui->dataview->verticalScrollBar()->value();
    ui->dataview->setPlainText(text);
    ui->dataview->verticalScrollBar()->setValue(original_scroll);
  }

  updateGraphics();
}

void MainWindow::updateGraphics() {
  compass_->setRotation(-state_->getDouble("imu", "yaw_deg"));
  target_heading_->setRotation(state_->getDouble("helm", "alpha_star_deg"));
  wind_->setRotation(state_->getDouble("wind", "angle_deg"));

  boom_->setRotation(state_->getDouble("ruddersts", "sail_deg"));
  target_boom_->setRotation(state_->getDouble("rudderctl", "sail_deg"));
  rudder_left_->setRotation(state_->getDouble("ruddersts", "rudder_l_deg"));
  rudder_right_->setRotation(state_->getDouble("ruddersts", "rudder_r_deg"));
  target_rudder_left_->setRotation(state_->getDouble("rudderctl", "rudder_l_deg"));
  target_rudder_right_->setRotation(state_->getDouble("rudderctl", "rudder_r_deg"));
  skipper_disc_->setRotation(-state_->getDouble("imu", "yaw_deg"));
  true_wind_->setRotation(state_->getDouble("skipper_input", "angle_true_deg"));

  QPointF speed(state_->getDouble("imu", "vel_y_m_s"), state_->getDouble("imu", "vel_x_m_s"));

  // The boat length is 220 pixels and 4m. Convert m/s into pixels/sec.
  speed *=  220 / 4.0;
  scroll_pos_ += speed * (scroll_update_time_.restart() / 1000.0);

  scroll_pos_.setX(fmod(scroll_pos_.x(), 4 * 256));
  scroll_pos_.setY(fmod(scroll_pos_.y(), 4 * 256));

  QBrush brush(scene_.backgroundBrush());
  QMatrix mat;
  mat.translate(scroll_pos_.x(), scroll_pos_.y());
  brush.setMatrix(mat);
  scene_.setBackgroundBrush(brush);

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
  target_heading_ = new QGraphicsLineItem(0, 0, 0, -60, compass_, &scene_);
  target_heading_->setPen(target_pen);
  heading_controller_ = new AngleController(QPointF(0, -60), 5, compass_);
  connect(heading_controller_, SIGNAL(turned(double)), SLOT(onTargetHeadingRotated(double)));

  // Speed and true wind display
  skipper_disc_ = new QGraphicsEllipseItem(-40, -40, 80, 80);
  scene_.addItem(skipper_disc_);
  skipper_disc_->setPos(-160, 0);
  true_wind_ = new QGraphicsLineItem(0, 0, 0, -45, skipper_disc_, &scene_);
  true_wind_->setPen(wind_pen);

  QPixmap background(":/images/water_texture.png");
  QBrush background_brush(background);
  QMatrix background_mat;
  background_brush.setMatrix(background_mat);
  scene_.setBackgroundBrush(background_brush);

  ui->graphicsView->setScene(&scene_);
  update_timer_.start();
}

void MainWindow::onRudderCtlActivated(double) {
  RudderProto command = INIT_RUDDERPROTO;
  command.rudder_l_deg = rudder_controller_->rotation();
  command.rudder_r_deg = command.rudder_l_deg;
  command.sail_deg = boom_controller_->rotation();

  size_t BufSize = 256;
  char buf[BufSize];
  ::snprintf(buf, BufSize, OFMT_RUDDERPROTO_CTL(command));
  state_->writeToBus(buf);
}

void MainWindow::onTargetHeadingRotated(double angle) {
  HelmsmanCtlProto proto = INIT_HELMSMANCTLPROTO;
  proto.alpha_star_deg = angle;

  size_t BufSize = 256;
  char buf[BufSize];
  ::snprintf(buf, BufSize, OFMT_HELMSMANCTLPROTO(proto));
  state_->writeToBus(buf);
}

void MainWindow::on_actionConnect_triggered(bool checked)
{
  if (checked)
          state_->tryToConnect();
  else
          state_->disconnect();
}


// Force angle into [0, 360).
double NormalizeDeg(double alpha_deg) {
  double x = fmod(alpha_deg, 360.0);
  if (x >= 0)
    return x;
  else
    return 360.0 + x;
}

// Force result into [-180, 180).
double SymmetricDeg(double alpha_deg) {
  return NormalizeDeg(alpha_deg + 180.0) - 180.0;
}


void MainWindow::keyPressEvent(QKeyEvent* event) {
  switch(event->key()) {
  case Qt::Key_C:
  case Qt::Key_Z:
  case Qt::Key_X:
  case Qt::Key_A:
  case Qt::Key_D:
          sendRemoteProto(kIdleHelmsmanMode); break;

  case Qt::Key_H:
  case Qt::Key_J:
  case Qt::Key_K:
  case Qt::Key_L:
          sendRemoteProto(kOverrideSkipperMode); break;

  case Qt::Key_Space:
          sendRemoteProto(kBrakeControlMode); break;
  }
  switch(event->key()) {
  case Qt::Key_C:
          rudder_controller_->setAngle(rudder_controller_->angle() - 2); break;
  case Qt::Key_Z:
          rudder_controller_->setAngle(rudder_controller_->angle() + 2); break;
  case Qt::Key_X:
          rudder_controller_->setAngle(0); break;

  case Qt::Key_A:
          boom_controller_->setAngle(
                SymmetricDeg(boom_controller_->angle() - 5)); break;
  case Qt::Key_D:
          boom_controller_->setAngle(
                SymmetricDeg(boom_controller_->angle() + 5)); break;

  case Qt::Key_H:
          heading_controller_->setAngle(
                SymmetricDeg(heading_controller_->angle() - 10)); break;
  case Qt::Key_J:
          heading_controller_->setAngle(
                SymmetricDeg(heading_controller_->angle() - 2)); break;
  case Qt::Key_K:
          heading_controller_->setAngle(
                SymmetricDeg(heading_controller_->angle() + 2)); break;
  case Qt::Key_L:
          heading_controller_->setAngle(
                SymmetricDeg(heading_controller_->angle() + 10)); break;
  }
}

void MainWindow::sendRemoteProto(int command) {
  RemoteProto proto = INIT_REMOTEPROTO;
  proto.command = command;
  proto.alpha_star_deg = heading_controller_->angle();
  size_t BufSize = 256;
  char buf[BufSize];
  ::snprintf(buf, BufSize, OFMT_REMOTEPROTO(proto));
  state_->writeToBus(buf);
}

void MainWindow::on_actionAuto_pilot_triggered() {
  sendRemoteProto(kNormalControlMode);
}

void MainWindow::on_actionDocking_triggered()
{
  sendRemoteProto(kDockingControlMode);
}

void MainWindow::on_actionBrake_triggered()
{
  sendRemoteProto(kBrakeControlMode);
}

void MainWindow::on_actionOverride_bearing_triggered()
{
  sendRemoteProto(kOverrideSkipperMode);
}

void MainWindow::on_actionIdleHelmsman_triggered()
{
  sendRemoteProto(kIdleHelmsmanMode);
}
