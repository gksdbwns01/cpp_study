/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#ifndef PACKETSVIEW_H
#define PACKETSVIEW_H

#include "common.h"
namespace netanim {
class PacketsView : public QGraphicsView
{
public:
  static PacketsView * getInstance ();
  void test ();
  void postParse ();
  void zoomIn ();
  void zoomOut ();
  void wheelEvent(QWheelEvent *event);
private:
  PacketsView ();

};

}

#endif // PACKETSVIEW_H
