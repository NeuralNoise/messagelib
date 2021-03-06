/*
  Copyright (c) 2016-2017 Montel Laurent <montel@kde.org>

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

#include "webengineembedpart.h"

using namespace MessageViewer;

class WebEngineEmbedPartInstancePrivate
{
public:
    WebEngineEmbedPartInstancePrivate()
        : webEngineEmbedPart(new WebEngineEmbedPart)
    {
    }

    ~WebEngineEmbedPartInstancePrivate()
    {
        delete webEngineEmbedPart;
    }

    WebEngineEmbedPart *webEngineEmbedPart;
};

Q_GLOBAL_STATIC(WebEngineEmbedPartInstancePrivate, sInstance)

WebEngineEmbedPart::WebEngineEmbedPart(QObject *parent)
    : QObject(parent)
{
}

WebEngineEmbedPart::~WebEngineEmbedPart()
{
}

WebEngineEmbedPart *WebEngineEmbedPart::self()
{
    return sInstance->webEngineEmbedPart;
}

QString WebEngineEmbedPart::contentUrl(const QString &contentId) const
{
    return mEmbeddedPartMap.value(contentId);
}

void WebEngineEmbedPart::addEmbedPart(const QByteArray &contentId, const QString &contentURL)
{
    mEmbeddedPartMap[QLatin1String(contentId)] = contentURL;
}

void WebEngineEmbedPart::clear()
{
    mEmbeddedPartMap.clear();
}

bool WebEngineEmbedPart::isEmpty() const
{
    return mEmbeddedPartMap.isEmpty();
}

QMap<QString, QString> WebEngineEmbedPart::embeddedPartMap() const
{
    return mEmbeddedPartMap;
}
