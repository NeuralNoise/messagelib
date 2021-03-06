/*
 * Copyright (C) 2006 Dmitry Morozhnikov <dmiceman@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef TEMPLATEPARSER_TEMPLATESINSERTCOMMAND_H
#define TEMPLATEPARSER_TEMPLATESINSERTCOMMAND_H

#include <QPushButton>

class KActionMenu;

namespace TemplateParser
{

class TemplatesInsertCommand : public QPushButton
{
    Q_OBJECT

public:
    explicit TemplatesInsertCommand(QWidget *parent, const QString &name = QString());
    ~TemplatesInsertCommand();

public:
    //TODO: apidox for all these enums
    enum Command {
        CDnl = 1,
        CRem,
        CInsert,
        CSystem,
        CQuotePipe,
        CQuote,
        CQHeaders,
        CHeaders,
        CTextPipe,
        CMsgPipe,
        CBodyPipe,
        CClearPipe,
        CText,
        CToAddr,
        CToName,
        CFromAddr,
        CFromName,
        CFullSubject,
        CMsgId,
        COHeader,
        CHeader,
        COToAddr,
        COToName,
        COFromAddr,
        COFromName,
        COFullSubject,
        COMsgId,
        CDateEn,
        CDateShort,
        CDate,
        CDow,
        CTimeLongEn,
        CTimeLong,
        CTime,
        CODateEn,
        CODateShort,
        CODate,
        CODow,
        COTimeLongEn,
        COTimeLong,
        COTime,
        CBlank,
        CNop,
        CClear,
        CDebug,
        CDebugOff,
        CToFName,
        CToLName,
        CFromFName,
        CFromLName,
        COToFName,
        COToLName,
        COFromFName,
        COFromLName,
        CCursor,
        CCCAddr,
        CCCName,
        CCCFName,
        CCCLName,
        COCCAddr,
        COCCName,
        COCCFName,
        COCCLName,
        COAddresseesAddr,
        CSignature,
        CQuotePlain,
        CQuoteHtml,
        CDictionaryLanguage,
        CLanguage
    };

Q_SIGNALS:
    void insertCommand(TemplatesInsertCommand::Command cmd);
    void insertCommand(const QString &cmd, int adjustCursor = 0);

public Q_SLOTS:
    void slotMapped(int cmd);

protected:
    KActionMenu *mMenu;
};

}

#endif
