/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#ifndef TEXTBUBBLE_H
#define TEXTBUBBLE_H

#include "common.h"

namespace netanim
{

class TextBubble : public QLabel
{
public:
  TextBubble (QString title, QString content);
  ~TextBubble ();

private:
signals:

public slots:

};

} // namespace netanim
#endif // TEXTBUBBLE_H
