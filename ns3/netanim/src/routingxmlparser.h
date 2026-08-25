/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */
#ifndef ROUTINGXMLPARSER_H
#define ROUTINGXMLPARSER_H

#include "common.h"

namespace netanim
{

typedef struct
{
  uint32_t nodeId;
  QString nextHop;
} RoutePathElement;

typedef std::vector <RoutePathElement> RoutePathElementsVector_t;
struct RoutingParsedElement
{
  enum RoutingParsedElementType
  {
    XML_INVALID,
    XML_ANIM,
    XML_RT,
    XML_RP,
    XML_RPE
  };
  RoutingParsedElementType type;

  // Anim

  double version;


  // Update time

  double updateTime;

  // Node

  uint32_t nodeId;

  // Routing table

  QString rt;

  // Route Path

  uint32_t rpElementCount;
  RoutePathElementsVector_t rpes;
  QString destination;

};


class RoutingXmlparser
{
public:
  RoutingXmlparser (QString traceFileName);
  ~RoutingXmlparser ();
  RoutingParsedElement parseNext ();
  bool isParsingComplete ();
  double getMaxSimulationTime ();
  double getMinSimulationTime ();
  bool isFileValid ();
  uint64_t getRtCount ();
  void doParse ();


private:
  QString m_traceFileName;
  bool m_parsingComplete;
  QXmlStreamReader * m_reader;
  QFile * m_traceFile;
  double m_maxSimulationTime;
  double m_minSimulationTime;
  bool m_fileIsValid;
  double m_version;
  RoutingParsedElement parseAnim ();
  RoutingParsedElement parseRt ();
  RoutingParsedElement parseRp ();
  RoutePathElement parseRpe ();
  void parseGeneric (RoutingParsedElement &);

  void searchForVersion ();
  void debugElement (RoutingParsedElement element);
};




}  // namespace netanim

#endif // ROUTINGXMLPARSER_H
