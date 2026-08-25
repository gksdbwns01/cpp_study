/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#ifndef COUNTERTABLESSCENE_H
#define COUNTERTABLESSCENE_H

#include "common.h"
#include "table.h"
#include "qcustomplot.h"

namespace netanim {

class CounterTablesScene : public QGraphicsScene
{

public:
  static CounterTablesScene * getInstance ();
  void setCurrentCounterName (QString Name);
  void reloadContent (bool force = false);
  void setAllowedNodesVector (QVector <uint32_t> allowedNodes);
  void showChart (bool show);

private:
  CounterTablesScene ();
  QString m_currentCounterName;
  Table * m_table;
  QGraphicsProxyWidget * m_tableItem;
  QVector <uint32_t> m_allowedNodes;
  bool isAllowedNode (uint32_t);
  uint32_t getIndexForNode (uint32_t nodeId);
  QCustomPlot * m_plot;
  QGraphicsProxyWidget * m_plotItem;
  bool m_showChart;


};

} // namespace netanim




#endif // COUNTERTABLESSCENE_H
