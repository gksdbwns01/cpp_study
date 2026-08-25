/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#ifndef LOGQT_H
#define LOGQT_H

#include "common.h"
#include "animpacket.h"

namespace netanim
{
std::ostream & operator << (std::ostream & os, QPointF pt);
std::ostream & operator << (std::ostream & os, QRectF r);
std::ostream & operator << (std::ostream & os, QTransform t);
std::ostream & operator << (std::ostream & os, AnimPacket * p);

}

#endif // LOGQT_H
