/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#ifndef STATSVIEW_H
#define STATSVIEW_H

#include "common.h"

namespace netanim
{

class StatsView : public QGraphicsView
{
public:
  static StatsView * getInstance ();
private:
  StatsView ();

};

} // namespace netanim
#endif // STATSVIEW_H
