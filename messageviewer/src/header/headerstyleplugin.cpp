/*
   Copyright (c) 2015-2017 Laurent Montel <montel@kde.org>

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

#include "headerstyleplugin.h"

using namespace MessageViewer;
class MessageViewer::HeaderStylePluginPrivate
{
public:
    HeaderStylePluginPrivate()
        : mIsEnabled(false),
          mShowEmoticons(true)
    {
    }

    bool mIsEnabled;
    bool mShowEmoticons;
};

HeaderStylePlugin::HeaderStylePlugin(QObject *parent)
    : QObject(parent)
    , d(new MessageViewer::HeaderStylePluginPrivate)
{
}

HeaderStylePlugin::~HeaderStylePlugin()
{
    delete d;
}

bool HeaderStylePlugin::hasMargin() const
{
    return true;
}

QString HeaderStylePlugin::alignment() const
{
    return QStringLiteral("left");
}

int HeaderStylePlugin::elidedTextSize() const
{
    return -1;
}

void HeaderStylePlugin::setIsEnabled(bool enabled)
{
    d->mIsEnabled = enabled;
}

bool HeaderStylePlugin::isEnabled() const
{
    return d->mIsEnabled;
}

bool HeaderStylePlugin::hasConfigureDialog() const
{
    return false;
}

void HeaderStylePlugin::showConfigureDialog(QWidget *parent)
{
    Q_UNUSED(parent);
    //Reimplement
}

