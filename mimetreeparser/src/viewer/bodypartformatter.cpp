/*  -*- c++ -*-
    bodypartformatter.cpp

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

#include "mimetreeparser_debug.h"

#include "bodyformatter/applicationpgpencrypted.h"
#include "bodyformatter/applicationpkcs7mime.h"
#include "bodyformatter/mailman.h"
#include "bodyformatter/multipartalternative.h"
#include "bodyformatter/multipartmixed.h"
#include "bodyformatter/multipartencrypted.h"
#include "bodyformatter/multipartsigned.h"
#include "bodyformatter/texthtml.h"
#include "bodyformatter/textplain.h"

#include "interfaces/bodypartformatter.h"
#include "interfaces/bodypart.h"

#include "viewer/bodypartformatterbasefactory.h"
#include "viewer/bodypartformatterbasefactory_p.h"

#include "viewer/attachmentstrategy.h"
#include "viewer/objecttreeparser.h"
#include "viewer/messagepart.h"

#include <KMime/Content>

using namespace MimeTreeParser;

namespace
{
class AnyTypeBodyPartFormatter
    : public MimeTreeParser::Interface::BodyPartFormatter
{
    static const AnyTypeBodyPartFormatter *self;
public:
    Result format(Interface::BodyPart *, HtmlWriter *) const override
    {
        qCDebug(MIMETREEPARSER_LOG) << "Acting as a Interface::BodyPartFormatter!";
        return AsIcon;
    }

    // unhide the overload with three arguments
    using MimeTreeParser::Interface::BodyPartFormatter::format;

    void adaptProcessResult(ProcessResult &result) const override
    {
        result.setNeverDisplayInline(true);
    }
    static const MimeTreeParser::Interface::BodyPartFormatter *create()
    {
        if (!self) {
            self = new AnyTypeBodyPartFormatter();
        }
        return self;
    }
};

const AnyTypeBodyPartFormatter *AnyTypeBodyPartFormatter::self = nullptr;

class ImageTypeBodyPartFormatter
    : public MimeTreeParser::Interface::BodyPartFormatter
{
    static const ImageTypeBodyPartFormatter *self;
public:
    Result format(Interface::BodyPart *, HtmlWriter *) const override
    {
        return AsIcon;
    }

    // unhide the overload with three arguments
    using MimeTreeParser::Interface::BodyPartFormatter::format;

    void adaptProcessResult(ProcessResult &result) const override
    {
        result.setNeverDisplayInline(false);
        result.setIsImage(true);
    }
    static const MimeTreeParser::Interface::BodyPartFormatter *create()
    {
        if (!self) {
            self = new ImageTypeBodyPartFormatter();
        }
        return self;
    }
};

const ImageTypeBodyPartFormatter *ImageTypeBodyPartFormatter::self = nullptr;

class MessageRfc822BodyPartFormatter
    : public MimeTreeParser::Interface::BodyPartFormatter
{
    static const MessageRfc822BodyPartFormatter *self;
public:
    Interface::MessagePart::Ptr process(Interface::BodyPart &) const override;
    MimeTreeParser::Interface::BodyPartFormatter::Result format(Interface::BodyPart *, HtmlWriter *) const override;
    using MimeTreeParser::Interface::BodyPartFormatter::format;
    static const MimeTreeParser::Interface::BodyPartFormatter *create();
};

const MessageRfc822BodyPartFormatter *MessageRfc822BodyPartFormatter::self;

const MimeTreeParser::Interface::BodyPartFormatter *MessageRfc822BodyPartFormatter::create()
{
    if (!self) {
        self = new MessageRfc822BodyPartFormatter();
    }
    return self;
}

Interface::MessagePart::Ptr MessageRfc822BodyPartFormatter::process(Interface::BodyPart &part) const
{
    const KMime::Message::Ptr message = part.content()->bodyAsMessage();
    return MessagePart::Ptr(new EncapsulatedRfc822MessagePart(part.objectTreeParser(), part.content(), message));
}

Interface::BodyPartFormatter::Result MessageRfc822BodyPartFormatter::format(Interface::BodyPart *part, HtmlWriter *writer) const
{
    Q_UNUSED(writer)
    const ObjectTreeParser *otp = part->objectTreeParser();
    const auto p = process(*part);
    const auto mp =  static_cast<MessagePart *>(p.data());
    if (mp) {
        if (!otp->attachmentStrategy()->inlineNestedMessages() && !otp->showOnlyOneMimePart()) {
            return Failed;
        } else {
            mp->html(true);
            return Ok;
        }
    } else {
        return Failed;
    }
}

typedef TextPlainBodyPartFormatter ApplicationPgpBodyPartFormatter;

} // anon namespace

void BodyPartFormatterBaseFactoryPrivate::messageviewer_create_builtin_bodypart_formatters()
{
    insert("application", "octet-stream", AnyTypeBodyPartFormatter::create());
    insert("application", "pgp", ApplicationPgpBodyPartFormatter::create());
    insert("application", "pkcs7-mime", ApplicationPkcs7MimeBodyPartFormatter::create());
    insert("application", "x-pkcs7-mime", ApplicationPkcs7MimeBodyPartFormatter::create());
    insert("application", "pgp-encrypted", ApplicationPGPEncryptedBodyPartFormatter::create());
    insert("application", "*", AnyTypeBodyPartFormatter::create());

    insert("text", "html", TextHtmlBodyPartFormatter::create());
    insert("text", "rtf", AnyTypeBodyPartFormatter::create());
    insert("text", "plain", MailmanBodyPartFormatter::create());
    insert("text", "plain", TextPlainBodyPartFormatter::create());
    insert("text", "*", MailmanBodyPartFormatter::create());
    insert("text", "*", TextPlainBodyPartFormatter::create());

    insert("image", "*", ImageTypeBodyPartFormatter::create());

    insert("message", "rfc822", MessageRfc822BodyPartFormatter::create());
    insert("message", "*", AnyTypeBodyPartFormatter::create());

    insert("multipart", "alternative", MultiPartAlternativeBodyPartFormatter::create());
    insert("multipart", "encrypted", MultiPartEncryptedBodyPartFormatter::create());
    insert("multipart", "signed", MultiPartSignedBodyPartFormatter::create());
    insert("multipart", "*", MultiPartMixedBodyPartFormatter::create());
    insert("*", "*", AnyTypeBodyPartFormatter::create());
}
