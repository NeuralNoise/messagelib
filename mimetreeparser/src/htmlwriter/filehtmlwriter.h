/*  -*- c++ -*-
    filehtmlwriter.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2003 Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef __MIMETREEPARSER_FILEHTMLWRITER_H__
#define __MIMETREEPARSER_FILEHTMLWRITER_H__

#include "mimetreeparser_export.h"
#include "mimetreeparser/htmlwriter.h"

#include <QFile>
#include <QTextStream>

class QString;

namespace MimeTreeParser
{

class MIMETREEPARSER_EXPORT FileHtmlWriter : public HtmlWriter
{
public:
    explicit FileHtmlWriter(const QString &filename);
    virtual ~FileHtmlWriter();

    void begin(const QString &cssDefs) override;
    void end() override;
    void reset() override;
    void write(const QString &str) override;
    void queue(const QString &str) override;
    void flush() override;
    void embedPart(const QByteArray &contentId, const QString &url) override;
    void extraHead(const QString &str) override;
private:
    void openOrWarn();

private:
    QFile mFile;
    QTextStream mStream;
};

} // namespace MimeTreeParser

#endif // __MIMETREEPARSER_FILEHTMLWRITER_H__
