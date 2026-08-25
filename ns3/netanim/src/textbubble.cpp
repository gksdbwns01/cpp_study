/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#include "textbubble.h"

namespace netanim
{

QRectF br;

TextBubble::TextBubble (QString title, QString content)
{
  content += '\0';
  QString str = title + "\n";
  QStringList list =  content.split ('^');
  foreach (QString s, list)
  {
    str += s + "\n";
  }
  setText (str);
  setFrameStyle (QFrame::Panel | QFrame::Sunken);
  adjustSize ();
  setTextInteractionFlags (Qt::TextSelectableByMouse);

}
TextBubble::~TextBubble ()
{

}


} //namespace netanim


