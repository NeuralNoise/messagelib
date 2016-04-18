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

#ifndef MAILNETWORKACCESSMANAGER_H
#define MAILNETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>
#include "webengineviewer_export.h"
#include "webengineviewer/networkpluginurlinterceptor.h"
class KActionCollection;
class QWebEngineView;
class QAction;
namespace WebEngineViewer
{
class WebHitTestResult;
class NetworkPluginUrlInterceptorInterface;
class NetworkAccessManagerWebEnginePrivate;
class NetworkPluginUrlInterceptorConfigureWidget;
class WEBENGINEVIEWER_EXPORT NetworkAccessManagerWebEngine : public QNetworkAccessManager
{
    Q_OBJECT
public:
    explicit NetworkAccessManagerWebEngine(QWebEngineView *webEngine, KActionCollection *ac, QObject *parent = Q_NULLPTR);
    ~NetworkAccessManagerWebEngine();
    void addInterceptor(WebEngineViewer::NetworkPluginUrlInterceptorInterface *interceptor);
    QList<QAction *> interceptorUrlActions(const WebEngineViewer::WebHitTestResult &result) const;
    QVector<WebEngineViewer::NetworkPluginUrlInterceptorConfigureWidgetSetting> configureInterceptorList(QWidget *parent = Q_NULLPTR) const;
private:
    NetworkAccessManagerWebEnginePrivate *const d;
};
}
#endif // MAILNETWORKACCESSMANAGER_H