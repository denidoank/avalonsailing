// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: jpilet@google.com (Julien Pilet)
//
// Implementation of the MainWindow behavior.

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(ClientState* state, QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow), state_(state), config_dialog_(state, this)
{
  ui->setupUi(this);
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
