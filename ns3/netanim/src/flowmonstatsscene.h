/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */
#ifndef FLOWMONSTATSSCENE_H
#define FLOWMONSTATSSCENE_H

#include "common.h"
#include "timevalue.h"
#include "flowmonxmlparser.h"


namespace netanim
{
class FlowMonStatsScene : public QGraphicsScene
{

public:
  static FlowMonStatsScene * getInstance ();
  void test ();
  void addFlowStat (uint32_t flowId, FlowStatsFlow_t flowStats);
  void addIpv4Classifier (uint32_t flowId, Ipv4Classifier_t ipv4Classifier);
  void addFlowProbes (FlowProbes_t flowProbes);
  void adjustRect ();
  void systemReset ();
  void reloadContent (bool force = false);
  uint32_t getNodeCount ();
  void showInfoWidget (bool show = true);
  void clearProxyWidgetsMap ();
  void addProxyWidgets ();
  QString flowStatsToString (FlowStatsFlow_t flowStats);
  QString ipv4ClassifierToString (Ipv4Classifier_t ipv4Classifier);

private:
  typedef std::map<uint32_t, FlowStatsFlow_t> FlowIdFlowStatsMap_t;
  typedef std::map<uint32_t, Ipv4Classifier_t> FlowIdIpv4ClassifierMap_t;
  typedef std::map <uint32_t, QGraphicsProxyWidget *> FlowIdProxyWidgetMap_t;

  FlowMonStatsScene ();
  void align ();
  QGraphicsProxyWidget * m_infoWidget;
  FlowIdFlowStatsMap_t m_flowIdFlowStats;
  FlowIdIpv4ClassifierMap_t m_flowIdIpv4Classifiers;
  FlowIdProxyWidgetMap_t m_flowIdProxyWidgets;
  QGraphicsProxyWidget * m_flowProbeWidget;
  qreal m_lastX;
  qreal m_lastY;
  qreal m_bottomY;
  FlowProbes_t m_flowProbes;

};


} // namespace netanim

#endif // FLOWMONSTATSSCENE_H
