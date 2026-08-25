/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 * Contributions: Makhtar Diouf <makhtar.diouf@gmail.com>
 */

#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>
#include <math.h>

#include <QWidget>

#include <QToolButton>
#include <QSpinBox>
#include <QSlider>
#include <QLCDNumber>
#include <QVBoxLayout>
#include <QSplitter>
#include <QToolBar>
#include <QStatusBar>
#include <QTime>
#include <QThread>
#include <QLabel>
#include <QComboBox>
#include <QProgressBar>
#include <QGraphicsPixmapItem>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QtCore/QXmlStreamReader>
#include <QFile>
#include <QTimer>
#include <QDialog>
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsProxyWidget>
#include <QFontMetrics>
#include <QLineEdit>
#include <QScrollBar>
#include <QTableWidget>
#include <QCheckBox>
#include <QElapsedTimer>

#if 0// if with ns3
#include <ns3/log.h>
#include <ns3/fatal-error.h>
#else
#include "log.h"
#include "fatal-error.h"
#endif

// Utilities to support porting to Qt5
#  define GET_ASCII(x)  x.toLatin1 ()
#  define GET_DATA(x)  x.toLatin1 ().data ()
#  define GET_DATA_PTR(x)  x->toLatin1 ().data ()

#endif // COMMON_H
