/*
   Copyright (C) 2015-2017 Laurent Montel <montel@kde.org>

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

#ifndef VIEWERPLUGIN_H
#define VIEWERPLUGIN_H

#include <QObject>
#include "messageviewer_export.h"
class KActionCollection;

namespace MessageViewer {
class ViewerPluginPrivate;
class ViewerPluginInterface;
class MESSAGEVIEWER_EXPORT ViewerPlugin : public QObject
{
    Q_OBJECT
public:
    explicit ViewerPlugin(QObject *parent = nullptr);
    ~ViewerPlugin();

    virtual MessageViewer::ViewerPluginInterface *createView(QWidget *parent,
                                                             KActionCollection *ac) = 0;
    virtual QString viewerPluginName() const = 0;
    virtual void showConfigureDialog(QWidget *parent = nullptr);
    virtual bool hasConfigureDialog() const;

    void setIsEnabled(bool enabled);
    bool isEnabled() const;

Q_SIGNALS:
    void configChanged();

private:
    ViewerPluginPrivate *const d;
};
}
#endif // VIEWERPLUGIN_H
