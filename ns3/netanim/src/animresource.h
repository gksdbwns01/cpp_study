/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#ifndef ANIMRESOURCE_H
#define ANIMRESOURCE_H

#include "common.h"


class AnimResourceManager
{
public:
  static AnimResourceManager * getInstance ();
  void add (uint32_t resourceId, QString resourcePath);
  QString get (uint32_t resourceid);
  uint32_t getNewResourceId ();
private:
  AnimResourceManager ();
  std::map <uint32_t, QString> m_resources;

  uint32_t m_maxResourceId;

};
#endif // ANIMRESOURCE_H
