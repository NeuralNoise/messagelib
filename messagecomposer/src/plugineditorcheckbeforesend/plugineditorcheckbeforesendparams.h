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

#ifndef PLUGINEDITORCHECKBEFORESENDPARAMS_H
#define PLUGINEDITORCHECKBEFORESENDPARAMS_H

#include "messagecomposer_export.h"
#include <QString>

namespace MessageComposer
{
class PluginEditorCheckBeforeSendParamsPrivate;
class MESSAGECOMPOSER_EXPORT PluginEditorCheckBeforeSendParams
{
public:
    PluginEditorCheckBeforeSendParams();
    PluginEditorCheckBeforeSendParams(const PluginEditorCheckBeforeSendParams &other);
    ~PluginEditorCheckBeforeSendParams();

    void setSubject(const QString &subject);
    QString subject() const;

    void setIdentity(uint currentIdentity);
    uint identity() const;

    bool isHtmlMail() const;
    void setHtmlMail(bool html);

    void setPlainText(const QString &text);
    QString plainText() const;

    void setBccAddresses(const QString &lst);
    QString bccAddresses() const;

    void setToAddresses(const QString &lst);
    QString toAddresses() const;

    void setCcAddresses(const QString &lst);
    QString ccAddresses() const;

    void setDefaultDomain(const QString &domain);
    QString defaultDomain() const;

    bool hasAttachment() const;
    void setHasAttachment(bool b);

    int transportId() const;
    void setTransportId(int id);

    PluginEditorCheckBeforeSendParams &operator =(const PluginEditorCheckBeforeSendParams &other);
    bool operator ==(const PluginEditorCheckBeforeSendParams &other) const;
private:
    PluginEditorCheckBeforeSendParamsPrivate *const d;
};
}
#endif // PLUGINEDITORCHECKBEFORESENDPARAMS_H
