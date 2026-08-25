/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#ifndef PACKETSSCENE_H
#define PACKETSSCENE_H

#include "common.h"
#include "textbubble.h"
#include "table.h"


namespace netanim {

class PacketsScene: public QGraphicsScene {

public:
  static PacketsScene * getInstance ();
  void addPackets ();
  void redraw (qreal fromTime, qreal toTime , QVector <uint32_t> allowedNodes, bool showGrid);
  void setFilter (int ft);
  void setRegexFilter (QString reg);
  void showGraph (bool show);

private:
  PacketsScene ();
  bool setUpNodeLines ();
  qreal timeToY (qreal t);
  void resetLines ();
  bool isAllowedNode (uint32_t nodeId);

  void addPacket (qreal tx, qreal rx, uint32_t fromNodeId, uint32_t toNodeId, QString metaInfo, bool drawPacket);
  std::map <uint32_t, QGraphicsLineItem *> m_nodeLines;
  std::map <uint32_t, uint32_t> m_lineIndex;

  std::vector <QGraphicsLineItem *> m_packetLines;
  std::vector <QGraphicsSimpleTextItem *> m_packetTexts;
  std::vector <QGraphicsSimpleTextItem *> m_nodeIdTexts;
  std::vector <QGraphicsSimpleTextItem *> m_rulerTexts;
  std::vector <QGraphicsLineItem *> m_horizontalRulerLines;
  std::vector <QGraphicsSimpleTextItem *> m_packetInfoTexts;

  qreal m_interNodeSpacing;
  qreal m_fromTime;
  qreal m_toTime;
  QVector <uint32_t> m_allowedNodes;
  QGraphicsProxyWidget * m_infoWidget;
  qreal m_borderHeight;
  qreal m_lineLength;
  TextBubble * m_textBubble;
  bool m_showGrid;
  //bool m_showTable;
  bool m_showGraph;
  int m_filter;

  QGraphicsLineItem * m_rulerLine;
  QString m_filterRegex;
  QGraphicsPathItem * m_packetPathItem;
  QPainterPath m_packetPath;

};
}

#endif // PACKETSSCENE_H
