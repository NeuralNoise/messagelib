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

#ifndef WEBENGINEVIEW_H
#define WEBENGINEVIEW_H

#include <QWebEngineView>
#include "messageviewer_export.h"

namespace MessageViewer
{
class WebEngineViewPrivate;
class MESSAGEVIEWER_EXPORT WebEngineView : public QWebEngineView
{
    Q_OBJECT
public:
    explicit WebEngineView(QWidget *parent = Q_NULLPTR);
    ~WebEngineView();

protected:
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;

    void forwardWheelEvent(QWheelEvent *event);
    void forwardKeyPressEvent(QKeyEvent *event);
    void forwardKeyReleaseEvent(QKeyEvent *event);
    void forwardMousePressEvent(QMouseEvent *event);
    void forwardMouseMoveEvent(QMouseEvent *event);
    void forwardMouseReleaseEvent(QMouseEvent *event);
private:
    WebEngineViewPrivate *const d;
};
}
#endif // WEBENGINEVIEW_H
