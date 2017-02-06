/*
    partnodebodypart.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2004 Marc Mutz <mutz@kde.org>,
                       Ingo Kloecker <kloecker@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

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

#ifndef __MIMETREEPARSER_PARTNODEBODYPART_H__
#define __MIMETREEPARSER_PARTNODEBODYPART_H__

#include "mimetreeparser_export.h"
#include "mimetreeparser/bodypart.h"

namespace KMime
{
class Content;
}

namespace MimeTreeParser
{
class NodeHelper;
}

namespace MimeTreeParser
{

/**
    @short an implementation of the BodyPart interface using KMime::Content's
*/
class MIMETREEPARSER_EXPORT PartNodeBodyPart : public Interface::BodyPart
{
public:
    explicit PartNodeBodyPart(ObjectTreeParser *otp, ProcessResult *result, KMime::Content *topLevelContent, KMime::Content *content,
                              NodeHelper *nodeHelper);

    QString makeLink(const QString &path) const Q_DECL_OVERRIDE;
    QString asText() const Q_DECL_OVERRIDE;
    QByteArray asBinary() const Q_DECL_OVERRIDE;
    QString contentTypeParameter(const char *param) const Q_DECL_OVERRIDE;
    QString contentDescription() const Q_DECL_OVERRIDE;
    QString contentDispositionParameter(const char *param) const Q_DECL_OVERRIDE;
    bool hasCompleteBody() const Q_DECL_OVERRIDE;

    Interface::BodyPartMemento *memento() const Q_DECL_OVERRIDE;
    void setBodyPartMemento(Interface::BodyPartMemento *memento) Q_DECL_OVERRIDE;
    BodyPart::Display defaultDisplay() const Q_DECL_OVERRIDE;
    void setDefaultDisplay(BodyPart::Display);
    KMime::Content *content() const Q_DECL_OVERRIDE
    {
        return mContent;
    }
    KMime::Content *topLevelContent() const Q_DECL_OVERRIDE
    {
        return mTopLevelContent;
    }
    NodeHelper *nodeHelper() const Q_DECL_OVERRIDE
    {
        return mNodeHelper;
    }

    ObjectTreeParser *objectTreeParser() const Q_DECL_OVERRIDE
    {
        return mObjectTreeParser;
    }

    ProcessResult *processResult() const Q_DECL_OVERRIDE
    {
        return mProcessResult;
    }

    Interface::ObjectTreeSource *source() const Q_DECL_OVERRIDE;
private:
    KMime::Content *mTopLevelContent;
    KMime::Content *mContent;
    BodyPart::Display mDefaultDisplay;
    NodeHelper *mNodeHelper;
    ObjectTreeParser *mObjectTreeParser;
    ProcessResult *mProcessResult;
};

}

#endif // __MIMETREEPARSER_PARTNODEBODYPART_H__
