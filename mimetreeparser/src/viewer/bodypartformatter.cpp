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

#include "bodypartformatterbasefactory.h"
#include "viewer/bodypartformatterbasefactory_p.h"
#include "viewer/attachmentstrategy.h"
#include "interfaces/bodypartformatter.h"
#include "interfaces/bodypart.h"

#include "viewer/objecttreeparser.h"
#include "messagepart.h"

#include <KMime/Content>

using namespace MessageViewer;

namespace
{
class AnyTypeBodyPartFormatter
    : public MessageViewer::Interface::BodyPartFormatter
{
    static const AnyTypeBodyPartFormatter *self;
public:
    Result format(Interface::BodyPart *, HtmlWriter *) const Q_DECL_OVERRIDE
    {
        qCDebug(MIMETREEPARSER_LOG) << "Acting as a Interface::BodyPartFormatter!";
        return AsIcon;
    }

    // unhide the overload with three arguments
    using MessageViewer::Interface::BodyPartFormatter::format;

    void adaptProcessResult(ProcessResult &result) const Q_DECL_OVERRIDE
    {
        result.setNeverDisplayInline(true);
    }
    static const MessageViewer::Interface::BodyPartFormatter *create()
    {
        if (!self) {
            self = new AnyTypeBodyPartFormatter();
        }
        return self;
    }
};

const AnyTypeBodyPartFormatter *AnyTypeBodyPartFormatter::self = 0;

class ImageTypeBodyPartFormatter
    : public MessageViewer::Interface::BodyPartFormatter
{
    static const ImageTypeBodyPartFormatter *self;
public:
    Result format(Interface::BodyPart *, HtmlWriter *) const Q_DECL_OVERRIDE
    {
        return AsIcon;
    }

    // unhide the overload with three arguments
    using MessageViewer::Interface::BodyPartFormatter::format;

    void adaptProcessResult(ProcessResult &result) const Q_DECL_OVERRIDE
    {
        result.setNeverDisplayInline(false);
        result.setIsImage(true);
    }
    static const MessageViewer::Interface::BodyPartFormatter *create()
    {
        if (!self) {
            self = new ImageTypeBodyPartFormatter();
        }
        return self;
    }
};

const ImageTypeBodyPartFormatter *ImageTypeBodyPartFormatter::self = 0;

class MessageRfc822BodyPartFormatter
    : public MessageViewer::Interface::BodyPartFormatter
{
    static const MessageRfc822BodyPartFormatter *self;
public:
    Interface::MessagePart::Ptr process(Interface::BodyPart &) const Q_DECL_OVERRIDE;
    MessageViewer::Interface::BodyPartFormatter::Result format(Interface::BodyPart *, HtmlWriter *) const Q_DECL_OVERRIDE;
    using MessageViewer::Interface::BodyPartFormatter::format;
    static const MessageViewer::Interface::BodyPartFormatter *create();
};

const MessageRfc822BodyPartFormatter *MessageRfc822BodyPartFormatter::self;

const MessageViewer::Interface::BodyPartFormatter *MessageRfc822BodyPartFormatter::create()
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

#define CREATE_BODY_PART_FORMATTER(subtype) \
    class subtype##BodyPartFormatter \
        : public MessageViewer::Interface::BodyPartFormatter \
    { \
        static const subtype##BodyPartFormatter *self; \
    public: \
        Interface::MessagePart::Ptr process(Interface::BodyPart &part) const Q_DECL_OVERRIDE; \
        MessageViewer::Interface::BodyPartFormatter::Result format(Interface::BodyPart *, HtmlWriter *) const Q_DECL_OVERRIDE; \
        using MessageViewer::Interface::BodyPartFormatter::format; \
        static const MessageViewer::Interface::BodyPartFormatter *create(); \
    }; \
    \
    const subtype##BodyPartFormatter *subtype##BodyPartFormatter::self; \
    \
    const MessageViewer::Interface::BodyPartFormatter *subtype##BodyPartFormatter::create() { \
        if ( !self ) { \
            self = new subtype##BodyPartFormatter(); \
        } \
        return self; \
    } \
    Interface::MessagePart::Ptr subtype##BodyPartFormatter::process(Interface::BodyPart &part) const { \
        return part.objectTreeParser()->process##subtype##Subtype(part.content(), *part.processResult()); \
    } \
    \
    MessageViewer::Interface::BodyPartFormatter::Result subtype##BodyPartFormatter::format(Interface::BodyPart *part, HtmlWriter *writer) const { \
        Q_UNUSED(writer) \
        const auto p = process(*part);\
        const auto mp = static_cast<MessagePart *>(p.data());\
        if (mp) { \
            mp->html(false);\
            return Ok;\
        }\
        return Failed;\
    }

CREATE_BODY_PART_FORMATTER(TextPlain)
CREATE_BODY_PART_FORMATTER(Mailman)
CREATE_BODY_PART_FORMATTER(TextHtml)

CREATE_BODY_PART_FORMATTER(ApplicationPkcs7Mime)

CREATE_BODY_PART_FORMATTER(MultiPartMixed)
CREATE_BODY_PART_FORMATTER(MultiPartAlternative)
CREATE_BODY_PART_FORMATTER(MultiPartSigned)
CREATE_BODY_PART_FORMATTER(MultiPartEncrypted)

typedef TextPlainBodyPartFormatter ApplicationPgpBodyPartFormatter;

#undef CREATE_BODY_PART_FORMATTER
} // anon namespace

void BodyPartFormatterBaseFactoryPrivate::messageviewer_create_builtin_bodypart_formatters()
{
    insert("application", "octet-stream", AnyTypeBodyPartFormatter::create());
    insert("application", "pgp", ApplicationPgpBodyPartFormatter::create());
    insert("application", "pkcs7-mime", ApplicationPkcs7MimeBodyPartFormatter::create());
    insert("application", "x-pkcs7-mime", ApplicationPkcs7MimeBodyPartFormatter::create());
    insert("application", "*", AnyTypeBodyPartFormatter::create());

    insert("text", "html", TextHtmlBodyPartFormatter::create());
    insert("text", "rtf", AnyTypeBodyPartFormatter::create());
    insert("text", "vcard", AnyTypeBodyPartFormatter::create());
    insert("text", "x-vcard", AnyTypeBodyPartFormatter::create());
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
