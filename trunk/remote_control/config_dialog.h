// Copyright 2011 The Avalon Project Authors. All rights reserved.
// Use of this source code is governed by the Apache License 2.0
// that can be found in the LICENSE file.
// Author: jpilet@google.com (Julien Pilet)
//
// Configuration dialog header.

#ifndef CONFIG_DIALOG_H
#define CONFIG_DIALOG_H

#include <QDialog>
#include "clientstate.h"

namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog {
  Q_OBJECT
public:
  ConfigDialog(ClientState* state, QWidget *parent = 0);
  ~ConfigDialog();

protected:
  void changeEvent(QEvent *e);

private:
  Ui::ConfigDialog *ui;
  ClientState* state_;

private slots:
  void on_buttonBox_accepted();
};

#endif // CONFIG_DIALOG_H
