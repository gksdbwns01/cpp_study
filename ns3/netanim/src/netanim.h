/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#ifndef NETANIM_H
#define NETANIM_H

#include "common.h"
#include "mode.h"

namespace netanim
{

class NetAnim : public QObject
{
  Q_OBJECT
public:
  NetAnim ();
  QTabWidget * getTabWidget();
private:
  typedef std::map <int, Mode *> TabIndexModeMap_t;
  QTabWidget * m_tabWidget;
  TabIndexModeMap_t m_TabMode;

private slots:
  void currentTabChangedSlot (int currentIndex);

};
}

#endif // NETANIM_H
