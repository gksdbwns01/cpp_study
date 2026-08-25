/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */


#ifndef TABLE_H
#define TABLE_H

#include "common.h"
namespace netanim {
class Table: public QWidget
{
Q_OBJECT
public:
  Table ();
  void setHeaderList (QStringList headerList);
  void addRow (QStringList rowContents, bool autoAdjust = false);
  void addCell (uint32_t cellIndex, QString value);
  void incrRowCount ();
  void removeAllRows ();
  void adjust ();
  void clear ();
  QString stringListToRowString (QStringList strList);
private:
  QTableWidget * m_table;
  QVBoxLayout * m_vLayout;
  QPushButton * m_exportTableButton;
  QStringList m_headerList;

protected slots:
  void exportButtonClickedSlot ();

};

}

#endif // TABLE_H
