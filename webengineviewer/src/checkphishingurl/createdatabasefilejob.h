/*
   Copyright (C) 2016-2017 Laurent Montel <montel@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef CREATEDATABASEFILEJOB_H
#define CREATEDATABASEFILEJOB_H

#include <QObject>
#include "webengineviewer_export.h"
#include "updatedatabaseinfo.h"

namespace WebEngineViewer
{
class CreateDatabaseFileJobPrivate;
class WEBENGINEVIEWER_EXPORT CreateDatabaseFileJob : public QObject
{
    Q_OBJECT
public:
    explicit CreateDatabaseFileJob(QObject *parent = nullptr);
    ~CreateDatabaseFileJob();
    void start();
    void setFileName(const QString &filename);
    bool canStart() const;
    void setUpdateDataBaseInfo(const WebEngineViewer::UpdateDataBaseInfo &infoDataBase);

Q_SIGNALS:
    void finished(bool success, const QString &newStateDatabase, const QString &minimumWaitDurationStr);

private:
    CreateDatabaseFileJobPrivate *const d;
};
}

#endif // CREATEDATABASEFILEJOB_H
