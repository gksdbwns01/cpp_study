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

#include "filepathmanager.h"

QString FilePathManager::value(const QtProperty *property) const
{
    if (!theValues.contains(property))
        return QString();
    return theValues[property].value;
}

QString FilePathManager::filter(const QtProperty *property) const
{
    if (!theValues.contains(property))
        return QString();
    return theValues[property].filter;
}

void FilePathManager::setValue(QtProperty *property, const QString &val)
{
    if (!theValues.contains(property))
        return;

    Data data = theValues[property];

    if (data.value == val)
        return;

    data.value = val;

    theValues[property] = data;

    emit propertyChanged(property);
    emit valueChanged(property, data.value);
}

void FilePathManager::setFilter(QtProperty *property, const QString &fil)
{
    if (!theValues.contains(property))
        return;

    Data data = theValues[property];

    if (data.filter == fil)
        return;

    data.filter = fil;

    theValues[property] = data;

    emit filterChanged(property, data.filter);
}

