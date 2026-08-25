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

#ifndef FILEEDITFACTORY_H
#define FILEEDITFACTORY_H

#include "qtpropertybrowser.h"
#include "filepathmanager.h"

class FileEdit;

class FileEditFactory : public QtAbstractEditorFactory<FilePathManager>
{
    Q_OBJECT
public:
    FileEditFactory(QObject *parent = 0)
        : QtAbstractEditorFactory<FilePathManager>(parent)
            { }
    virtual ~FileEditFactory();
protected:
    virtual void connectPropertyManager(FilePathManager *manager);
    virtual QWidget *createEditor(FilePathManager *manager, QtProperty *property,
                QWidget *parent);
    virtual void disconnectPropertyManager(FilePathManager *manager);
private slots:
    void slotPropertyChanged(QtProperty *property, const QString &value);
    void slotFilterChanged(QtProperty *property, const QString &filter);
    void slotSetValue(const QString &value);
    void slotEditorDestroyed(QObject *object);
private:
    QMap<QtProperty *, QList<FileEdit *> > theCreatedEditors;
    QMap<FileEdit *, QtProperty *> theEditorToProperty;
};

#endif
