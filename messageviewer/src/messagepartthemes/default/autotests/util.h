/*
  Copyright (c) 2010 Thomas McGuire <thomas.mcguire@kdab.com>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/

#ifndef __MESSAGEVIEWER_TESTS_UTIL_H__
#define __MESSAGEVIEWER_TESTS_UTIL_H__

#include <MimeTreeParser/HtmlWriter>
#include <MimeTreeParser/CSSHelperBase>

#include <KMime/Message>

namespace MessageViewer
{

namespace Test
{

class HtmlWriter : public MimeTreeParser::HtmlWriter
{
public:
    explicit HtmlWriter() {}
    virtual ~HtmlWriter() {}

    void begin(const QString &) Q_DECL_OVERRIDE {}
    void write(const QString &) Q_DECL_OVERRIDE {}
    void end() Q_DECL_OVERRIDE {}
    void reset() Q_DECL_OVERRIDE {}
    void queue(const QString &str) Q_DECL_OVERRIDE {
        html.append(str);
    }
    void flush() Q_DECL_OVERRIDE {}
    void embedPart(const QByteArray &, const QString &) Q_DECL_OVERRIDE {}
    void extraHead(const QString &) Q_DECL_OVERRIDE {}

    QString html;
};

class CSSHelper : public MimeTreeParser::CSSHelperBase
{
public:
    CSSHelper() : MimeTreeParser::CSSHelperBase(0)
    {
        for (int i = 0; i < 3; ++i) {
            mQuoteColor[i] = QColor(0x00, 0x80 - i * 0x10, 0x00);
        }
    }
    virtual ~CSSHelper() {}

    QString nonQuotedFontTag() const
    {
        return QStringLiteral("<");
    }

    QString quoteFontTag(int) const
    {
        return QStringLiteral("<");
    }
};

KMime::Message::Ptr readAndParseMail(const QString &mailFile);

}
}

#endif //__MESSAGEVIEWER_TESTS_UTIL_H__