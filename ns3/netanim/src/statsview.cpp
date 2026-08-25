/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#include "statsview.h"

namespace netanim
{

StatsView * pStatsView = 0;

StatsView::StatsView ()
{

}

StatsView *
StatsView::getInstance ()
{
  if (!pStatsView)
    {
      pStatsView = new StatsView;
    }
  return pStatsView;
}


} // namespace netanim
