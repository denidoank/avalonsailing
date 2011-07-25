// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: jpilet@google.com (Julien Pilet)
//
// Implementation for the configuration dialog.

#include "config_dialog.h"
#include "ui_config_dialog.h"

ConfigDialog::ConfigDialog(ClientState* state, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ConfigDialog), state_(state)
{
  ui->setupUi(this);
  ui->commandEdit->setText(state_->getCommand());
}

ConfigDialog::~ConfigDialog()
{
  delete ui;
}

void ConfigDialog::changeEvent(QEvent *e)
{
  QDialog::changeEvent(e);
  switch (e->type()) {
    case QEvent::LanguageChange:
      ui->retranslateUi(this);
      break;
    default:
      break;
  }
}

void ConfigDialog::on_buttonBox_accepted()
{
  state_->setCommand(ui->commandEdit->text());
}
