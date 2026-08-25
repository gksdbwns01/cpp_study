/****************************************************************************
**
** Copyright (C) 2006 Trolltech ASA. All rights reserved.
**
** This file is part of the documentation of Qt. It was originally
** published as part of Qt Quarterly.
**
** SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-2.0-only
**
****************************************************************************/

#ifndef FILEEDIT_H
#define FILEEDIT_H

#include <QLineEdit>

class FileEdit : public QWidget
{
    Q_OBJECT
public:
    FileEdit(QWidget *parent = 0);
    void setFilePath(const QString &filePath) { if (theLineEdit->text() != filePath) theLineEdit->setText(filePath); }
    QString filePath() const { return theLineEdit->text(); }
    void setFilter(const QString &filter) { theFilter = filter; }
    QString filter() const { return theFilter; }
signals:
    void filePathChanged(const QString &filePath);
protected:
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
private slots:
    void buttonClicked();
private:
    QLineEdit *theLineEdit;
    QString theFilter;
};

#endif
