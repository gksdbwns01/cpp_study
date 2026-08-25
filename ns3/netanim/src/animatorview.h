/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#ifndef ANIMATORVIEW_H
#define ANIMATORVIEW_H

#include "common.h"
#include "animatorscene.h"


namespace netanim
{

class AnimatorView : public QGraphicsView
{
public:

  static AnimatorView * getInstance ();
  void systemReset ();
  void fitSceneWithinView ();
  void postParse ();
  void setCurrentZoomFactor (qreal factor);


protected:
  void paintEvent (QPaintEvent * event);
  void wheelEvent (QWheelEvent *event);


private:
  explicit AnimatorView (QGraphicsScene *);
  AnimatorScene * getAnimatorScene ();
  void updateTransform ();
  qreal m_currentZoomFactor;

signals:

public slots:


};
} // namespace netanim
#endif // ANIMATORVIEW_H
