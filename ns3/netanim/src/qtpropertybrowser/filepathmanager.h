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

#ifndef FILEPATHMANAGER_H
#define FILEPATHMANAGER_H

#include "qtpropertybrowser.h"
#include <QMap>

class FilePathManager : public QtAbstractPropertyManager
{
    Q_OBJECT
public:
    FilePathManager(QObject *parent = 0)
        : QtAbstractPropertyManager(parent)
            { }

    QString value(const QtProperty *property) const;
    QString filter(const QtProperty *property) const;

public slots:
    void setValue(QtProperty *property, const QString &val);
    void setFilter(QtProperty *property, const QString &fil);
signals:
    void valueChanged(QtProperty *property, const QString &val);
    void filterChanged(QtProperty *property, const QString &fil);
protected:
    virtual QString valueText(const QtProperty *property) const { return value(property); }
    virtual void initializeProperty(QtProperty *property) { theValues[property] = Data(); }
    virtual void uninitializeProperty(QtProperty *property) { theValues.remove(property); }
private:

    struct Data
    {
        QString value;
        QString filter;
    };

    QMap<const QtProperty *, Data> theValues;
};


#endif
