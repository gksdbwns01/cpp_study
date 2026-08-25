/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#include "logqt.h"

namespace netanim
{

std::ostream & operator << (std::ostream & os, QPointF pt)
{
  os << "QPointF [x,y]:[" << pt.x () << "," << pt.y () << "]";
  return os;
}

std::ostream & operator << (std::ostream & os, QRectF r)
{
  os << "QRectF topLeft:" << r.topLeft () << " bottomRight:" << r.bottomRight ();
  return os;
}


std::ostream & operator << (std::ostream & os, QTransform t)
{
  os << "QTransform m11:" << t.m11 () << " m22:" << t.m22 ();
  return os;
}


std::ostream & operator << (std::ostream & os, AnimPacket * p)
{
  os << "AnimPacket:";
  os << " From Node Id:" << p->getFromNodeId ();
  os << " To Node Id:" << p->getToNodeId ();
  os << " First Bit Tx:" << p->getFirstBitTx () << std::endl;
  //os << "First Bit Rx:" << p->getFirstBitRx () << endl;
  //os << "Last Bit Tx:" << p->getLastBitTx () << endl;
  //os << "Last Bit Rx:" << p->getLastBitRx () << endl;
  return os;
}


}
