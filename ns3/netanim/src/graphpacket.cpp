/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#include "graphpacket.h"
#include "packetsview.h"
#include "logqt.h"

namespace netanim {
#define PI 3.14159265

NS_LOG_COMPONENT_DEFINE ("GraphPacket");

GraphPacket::GraphPacket (QPointF fromNodePos, QPointF toNodePos):
  m_fromNodePos (fromNodePos),
  m_toNodePos (toNodePos)
{
  QLineF l (fromNodePos, toNodePos);
  setLine (l);
  setFlags (QGraphicsItem::ItemIsSelectable);
  QPen p = pen();
  p.setColor (Qt::blue);
  p.setWidthF (1.5);
  setPen(p);
}


void
GraphPacket::paint (QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{

  painter->save();
  QPen p;
  p.setColor (Qt::black);
  painter->setPen (p);
  painter->translate (line().p2());
  qreal angle = PI/4;
  qreal mag = 9;
  painter->rotate (360 - line().angle ());
  painter->drawLine(0, 0, -mag * cos (angle), mag * sin (angle));
  painter->drawLine(0, 0, -mag * cos (angle), -mag * sin (angle));
  painter->restore ();
  QGraphicsLineItem::paint(painter, option, widget);

}

}
