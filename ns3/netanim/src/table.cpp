/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: John Abraham <john.abraham.in@gmail.com>
 */

#include "table.h"

namespace netanim {

Table::Table ()
{
  m_exportTableButton = new QPushButton ("Export Table");
  m_exportTableButton->setMaximumWidth (100);
  m_table = new QTableWidget;
  m_vLayout = new QVBoxLayout;
  m_vLayout->addWidget (m_exportTableButton);
  m_vLayout->addWidget (m_table);
  connect(m_exportTableButton, &QPushButton::clicked, this, &Table::exportButtonClickedSlot);
  setLayout (m_vLayout);
}

void
Table::setHeaderList (QStringList headerList)
{
  m_table->setColumnCount (headerList.size ());
  m_table->setHorizontalHeaderLabels (headerList);
  m_headerList = headerList;

}

void
Table::addCell (uint32_t cellIndex, QString value)
{
  uint32_t rows = m_table->rowCount ();
  uint32_t column = cellIndex;
  QTableWidgetItem * item = new QTableWidgetItem (value);
  m_table->setItem (rows-1, column, item);
}

void
Table::incrRowCount ()
{
  uint32_t rows = m_table->rowCount ();
  m_table->setRowCount (rows + 1);
}

void
Table::addRow (QStringList rowContents, bool autoAdjust)
{
  uint32_t rows = m_table->rowCount ();
  m_table->setRowCount (rows + 1);
  uint32_t column = 0;
  foreach (QString st, rowContents)
    {
      QTableWidgetItem * item = new QTableWidgetItem (st);
      m_table->setItem (rows, column++, item);
    }
  if (autoAdjust)
    {
      m_table->resizeRowsToContents();
      m_table->resizeColumnsToContents ();
    }
}


void
Table::removeAllRows ()
{
  for (int i = 0; i < m_table->rowCount (); ++i)
    {
      m_table->removeRow (i);
    }
  m_table->setRowCount (0);
}

void
Table::clear ()
{
  removeAllRows ();
  //clearContents ();
}

void
Table::adjust ()
{
  m_table->resizeRowsToContents();
  m_table->resizeColumnsToContents ();
}

QString
Table::stringListToRowString (QStringList strList)
{
  QString str = "";
  for (int i = 0; i < strList.count (); ++i)
    {
      str += strList.at (i);
      if (i < (strList.count () - 1))
        {
          str += "\t";
        }
    }
  str += "\n";
  return str;
}

void
Table::exportButtonClickedSlot ()
{
  QFileDialog fd;
  QString exportFileName = QFileDialog::getSaveFileName ();
  QFile f (exportFileName);
  if (f.open (QIODevice::WriteOnly))
    {
      QString headerString = stringListToRowString (m_headerList);
      f.write (GET_DATA (headerString));
      int rowCount = m_table->rowCount ();
      int columnCount = m_table->columnCount ();
      for (int i = 0; i < rowCount; ++i)
        {
          QStringList rowStringList;
          for (int j = 0; j < columnCount; ++j)
            {
              QTableWidgetItem * item = m_table->item (i, j);
              if (!item)
                {
                  rowStringList << "N/A";
                }
              else
                {
                  rowStringList << item->text ();
                }
            }
          f.write (GET_DATA (stringListToRowString (rowStringList)));
        }
    }
  f.close();
}


}
