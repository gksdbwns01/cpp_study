/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#ifndef MODE_H
#define MODE_H

#include "common.h"

class Mode: public QWidget
{
public:
  virtual QWidget * getCentralWidget () = 0;
  virtual QString getTabName () = 0;
  virtual void setFocus (bool focus) = 0;
};
#endif // MODE_H
