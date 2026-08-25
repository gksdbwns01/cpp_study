/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */
#include "netanim.h"
#include "animatormode.h"
#include "statsmode.h"
#include "packetsmode.h"
#ifdef WITH_NS3
#include "designer/designermode.h"
#endif

namespace netanim
{

NetAnim::NetAnim ():
  m_tabWidget (new QTabWidget)
{

  AnimatorMode * animatorTab = AnimatorMode::getInstance ();
  m_tabWidget->addTab (animatorTab->getCentralWidget (), animatorTab->getTabName ());
  m_TabMode[0] = animatorTab;


  StatsMode * statsTab = StatsMode::getInstance ();
  m_tabWidget->addTab (statsTab->getCentralWidget (), statsTab->getTabName ());
  m_TabMode[1] = statsTab;


  PacketsMode * packetsTab = PacketsMode::getInstance ();
  m_tabWidget->addTab (packetsTab->getCentralWidget (), packetsTab->getTabName ());
  m_TabMode[2] = packetsTab;

#ifdef WITH_NS3
  DesignerMode * designerMode = new DesignerMode;
  m_tabWidget->addTab (designerMode->getCentralWidget (), designerMode->getTabName ());
#endif
  QObject::connect(m_tabWidget, &QTabWidget::currentChanged, this, &NetAnim::currentTabChangedSlot);
  m_tabWidget->setCurrentIndex (0);
  m_tabWidget->showMaximized ();
  m_tabWidget->show ();
  animatorTab->start ();
}


void
NetAnim::currentTabChangedSlot (int currentIndex)
{
  for (TabIndexModeMap_t::const_iterator i = m_TabMode.begin ();
      i != m_TabMode.end ();
      ++i)
    {
      if (currentIndex == i->first)
        {
          i->second->setFocus (true);
          continue;
        }
      i->second->setFocus (false);
    }

}

QTabWidget *
NetAnim::getTabWidget()
{
  return m_tabWidget;
}

} // namespace netanim

