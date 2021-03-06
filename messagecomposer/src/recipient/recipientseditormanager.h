/*
   Copyright (C) 2017 Laurent Montel <montel@kde.org>

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


#ifndef RECIPIENTSEDITORMANAGER_H
#define RECIPIENTSEDITORMANAGER_H

#include <QObject>
#include "messagecomposer_export.h"

namespace Akonadi
{
class EmailAddressSelectionModel;
}

namespace MessageComposer
{
class MESSAGECOMPOSER_EXPORT RecipientsEditorManager : public QObject
{
    Q_OBJECT
public:
    explicit RecipientsEditorManager(QObject *parent = nullptr);
    ~RecipientsEditorManager();

    static RecipientsEditorManager *self();
    Akonadi::EmailAddressSelectionModel *model();

private:
    Akonadi::EmailAddressSelectionModel *mModel;
};
}

#endif // RECIPIENTSEDITORMANAGER_H
