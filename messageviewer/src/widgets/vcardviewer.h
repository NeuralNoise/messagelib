/* This file is part of the KDE project
   Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>
   Copyright (C) 2013-2017 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#ifndef MESSAGEVIEWER_VCARDVIEWER_H
#define MESSAGEVIEWER_VCARDVIEWER_H
#include <QDialog>
#include <kcontacts/addressee.h>

namespace KAddressBookGrantlee {
class GrantleeContactViewer;
}
class QPushButton;
namespace MessageViewer {
class VCardViewer : public QDialog
{
    Q_OBJECT
public:
    explicit VCardViewer(QWidget *parent, const QByteArray &vCard);
    ~VCardViewer();

private:
    void slotUser1();
    void slotUser2();
    void slotUser3();
    void readConfig();
    void writeConfig();
    KAddressBookGrantlee::GrantleeContactViewer *mContactViewer;

    KContacts::Addressee::List mAddresseeList;
    int mAddresseeListIndex;
    QPushButton *mUser2Button;
    QPushButton *mUser3Button;
};
}

#endif // VCARDVIEWER_H
