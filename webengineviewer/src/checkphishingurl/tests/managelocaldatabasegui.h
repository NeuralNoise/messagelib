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

#ifndef MANAGELOCALDATABASEGUI_H
#define MANAGELOCALDATABASEGUI_H

#include <QWidget>
class QPlainTextEdit;
namespace WebEngineViewer {
class LocalDataBaseManager;
}

class ManageLocalDataBaseGui : public QWidget
{
    Q_OBJECT
public:
    explicit ManageLocalDataBaseGui(QWidget *parent = nullptr);
    ~ManageLocalDataBaseGui();
private Q_SLOTS:
    void slotDownloadFullDatabase();
private:
    QPlainTextEdit *mResult;
    WebEngineViewer::LocalDataBaseManager *mDbManager;
};

#endif // MANAGELOCALDATABASEGUI_H
