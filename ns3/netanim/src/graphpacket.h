/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#ifndef GRAPHPACKET_H
#define GRAPHPACKET_H
#include "common.h"

namespace netanim {
class GraphPacket: public QGraphicsLineItem
{
public:
  GraphPacket (QPointF fromNodePos, QPointF toNodePos);
  void paint (QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
 // QRectF boundingRect ();
 // QPainterPath shape () const;
private:
  QPointF m_fromNodePos;
  QPointF m_toNodePos;
  QRectF m_boundingRect;
  QPainterPath m_shape;

};
}
#endif // GRAPHPACKET_H
