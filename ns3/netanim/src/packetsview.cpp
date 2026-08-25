/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#include "packetsview.h"
#include "packetsscene.h"
#include "logqt.h"

namespace netanim {

NS_LOG_COMPONENT_DEFINE ("PacketsView");
PacketsView * pPacketsView = 0;
PacketsView::PacketsView ():
  QGraphicsView (PacketsScene::getInstance ())
{
  setRenderHint (QPainter::Antialiasing);
  setViewportUpdateMode (BoundingRectViewportUpdate);
}

PacketsView *
PacketsView::getInstance ()
{
  if (!pPacketsView)
    {
      pPacketsView = new PacketsView;
    }
  return pPacketsView;
}

void
PacketsView::test()
{
}

void
PacketsView::postParse ()
{

  return;
}

void
PacketsView::wheelEvent(QWheelEvent *event)
{
  QGraphicsView::wheelEvent(event);
}

void
PacketsView::zoomIn ()
{
  scale (1.1, 1.1);
}

void
PacketsView::zoomOut ()
{
  scale (0.9, 0.9);
}


}
