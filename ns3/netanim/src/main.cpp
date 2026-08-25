/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#include "netanim.h"

int main (int argc, char *argv[])
{
  ns3::LogComponentEnable ("AnimatorScene", ns3::LOG_LEVEL_ALL);
  ns3::LogComponentEnable ("AnimatorView", ns3::LOG_LEVEL_ALL);
  ns3::LogComponentEnable ("AnimNode", ns3::LOG_LEVEL_ALL);
  ns3::LogComponentEnable ("AnimPacket", ns3::LOG_LEVEL_ALL);
  ns3::LogComponentEnable ("AnimatorMode", ns3::LOG_LEVEL_ALL);
  ns3::LogComponentEnable ("ResizeableItem", ns3::LOG_LEVEL_ALL);
  ns3::LogComponentEnable ("Animxmlparser", ns3::LOG_LEVEL_ALL);
  ns3::LogComponentEnable ("AnimPropertyBroswer", ns3::LOG_LEVEL_ALL);
  //ns3::LogComponentEnable ("PacketsScene", ns3::LOG_LEVEL_ALL);
  //ns3::LogComponentEnable ("PacketsView", ns3::LOG_LEVEL_ALL);
  ns3::LogComponentEnable ("GraphPacket", ns3::LOG_LEVEL_ALL);
  ns3::LogComponentEnable ("CounterTablesScene", ns3::LOG_LEVEL_ALL);
  ns3::LogComponentEnable ("PacketsMode", ns3::LOG_LEVEL_ALL);
  //ns3::LogComponentEnable ("PacketsScene", ns3::LOG_LEVEL_ALL);


  QApplication app (argc, argv);
  app.setApplicationName ("NetAnim");
  app.setWindowIcon (QIcon (":/resources/netanim-logo.png"));
  netanim::NetAnim netAnim;
  return app.exec ();


}
