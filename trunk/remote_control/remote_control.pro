# Copyright 2011 The Avalon Project Authors. All rights reserved.
# Use of this source code is governed by the Apache License 2.0
# that can be found in the LICENSE file.
# Author: jpilet@google.com (Julien Pilet)
# Build file for remote_control.
# process with qmake.
TARGET = remote_control
TEMPLATE = app
QT += network
SOURCES += main.cpp \
    mainwindow.cpp \
    clientstate.cpp \
    config_dialog.cpp \
    anglecontroller.cpp
HEADERS += mainwindow.h \
    clientstate.h \
    config_dialog.h \
    anglecontroller.h \
    ../proto/rudder.h \
    ../proto/helmsman.h \
    ../proto/remote.h
FORMS += mainwindow.ui \
    config_dialog.ui \
    config_dialog.ui
RESOURCES += icons.qrc
