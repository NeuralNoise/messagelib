/*
  Copyright (c) 2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef SEARCHLINESTATUS_H
#define SEARCHLINESTATUS_H

#include <PimCommon/LineEditWithCompleter>
#include <Akonadi/KMime/MessageStatus>
#include "messagelist_export.h"
class QAction;
namespace MessageList
{
namespace Core
{
class MESSAGELIST_EXPORT SearchLineStatus : public PimCommon::LineEditWithCompleter
{
    Q_OBJECT
public:
    explicit SearchLineStatus(QWidget *parent = Q_NULLPTR);
    ~SearchLineStatus();

    void setLocked(bool b);
    bool locked() const;

    void clearFilterAction();

Q_SIGNALS:
    void filterActionChanged(const QList<Akonadi::MessageStatus> &lst);

private Q_SLOTS:
    void slotToggledLockAction();
    void showMenu();
private:
    void createFilterAction(const QIcon &icon, const QString &text, int value);
    void createMenuSearch();
    void updateLockAction();
    void initializeActions();
    bool mLocked;
    QAction *mLockAction;
    QAction *mFiltersAction;
    QMenu *mFilterMenu;
    QList<QAction *> mFilterListActions;
};

}
}

#endif // SEARCHLINESTATUS_H
