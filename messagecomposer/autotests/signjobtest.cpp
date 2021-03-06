/*
  Copyright (C) 2009 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>

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

#include "signjobtest.h"

#include <QDebug>
#include <qtest.h>
#include "qtest_messagecomposer.h"
#include "cryptofunctions.h"

#include <kmime/kmime_content.h>

#include <Libkleo/Enum>
#include <kjob.h>

#include <MessageComposer/Composer>
#include <MessageComposer/SignJob>
#include <MessageComposer/TransparentJob>

#include <MessageCore/NodeHelper>
#include <setupenv.h>

#include <stdlib.h>

QTEST_MAIN(SignJobTest)

void SignJobTest::initTestCase()
{
    MessageComposer::Test::setupEnv();
}

void SignJobTest::testContentDirect()
{

    std::vector< GpgME::Key > keys = MessageComposer::Test::getKeys();

    MessageComposer::Composer *composer = new MessageComposer::Composer;
    MessageComposer::SignJob *sJob = new MessageComposer::SignJob(composer);

    QVERIFY(composer);
    QVERIFY(sJob);

    QByteArray data(QString::fromLocal8Bit("one flew over the cuckoo's nest").toUtf8());
    KMime::Content *content = new KMime::Content;
    content->setBody(data);

    sJob->setContent(content);
    sJob->setCryptoMessageFormat(Kleo::OpenPGPMIMEFormat);
    sJob->setSigningKeys(keys);

    checkSignJob(sJob);
}

void SignJobTest::testContentChained()
{
    std::vector< GpgME::Key > keys = MessageComposer::Test::getKeys();

    QByteArray data(QString::fromLocal8Bit("one flew over the cuckoo's nest").toUtf8());
    KMime::Content *content = new KMime::Content;
    content->setBody(data);

    MessageComposer::TransparentJob *tJob =  new MessageComposer::TransparentJob;
    tJob->setContent(content);

    MessageComposer::Composer *composer = new MessageComposer::Composer;
    MessageComposer::SignJob *sJob = new MessageComposer::SignJob(composer);

    sJob->setCryptoMessageFormat(Kleo::OpenPGPMIMEFormat);
    sJob->setSigningKeys(keys);

    sJob->appendSubjob(tJob);

    checkSignJob(sJob);
}

void SignJobTest::testHeaders()
{
    std::vector< GpgME::Key > keys = MessageComposer::Test::getKeys();

    MessageComposer::Composer *composer = new MessageComposer::Composer;
    MessageComposer::SignJob *sJob = new MessageComposer::SignJob(composer);

    QVERIFY(composer);
    QVERIFY(sJob);

    QByteArray data(QString::fromLocal8Bit("one flew over the cuckoo's nest").toUtf8());
    KMime::Content *content = new KMime::Content;
    content->setBody(data);

    sJob->setContent(content);
    sJob->setCryptoMessageFormat(Kleo::OpenPGPMIMEFormat);
    sJob->setSigningKeys(keys);

    VERIFYEXEC(sJob);

    QByteArray mimeType("multipart/signed");
    QByteArray charset("ISO-8859-1");

    KMime::Content *result = sJob->content();
    result->assemble();
    qDebug() << result->encodedContent();

    QVERIFY(result->contentType(false));
    QCOMPARE(result->contentType()->mimeType(), mimeType);
    QCOMPARE(result->contentType()->charset(), charset);
    QVERIFY(result->contentType()->parameter(QString::fromLocal8Bit("micalg")).startsWith(QLatin1String("pgp-sha"))); // sha1 or sha256, depending on GnuPG version
    QCOMPARE(result->contentType()->parameter(QString::fromLocal8Bit("protocol")), QString::fromLocal8Bit("application/pgp-signature"));
    QCOMPARE(result->contentTransferEncoding()->encoding(), KMime::Headers::CE7Bit);
}

void SignJobTest::testRecommentationRFC3156()
{
    std::vector< GpgME::Key > keys = MessageComposer::Test::getKeys();

    QString data = QStringLiteral("=2D Magic foo\nFrom test\n\n-- quaak\nOhno");
    KMime::Headers::contentEncoding cte = KMime::Headers::CEquPr;

    MessageComposer::Composer *composer = new MessageComposer::Composer;
    MessageComposer::SignJob *sJob = new MessageComposer::SignJob(composer);

    QVERIFY(composer);
    QVERIFY(sJob);

    KMime::Content *content = new KMime::Content;
    content->setBody(data.toUtf8());

    sJob->setContent(content);
    sJob->setCryptoMessageFormat(Kleo::OpenPGPMIMEFormat);
    sJob->setSigningKeys(keys);

    VERIFYEXEC(sJob);

    KMime::Content *result = sJob->content();
    result->assemble();
    qDebug() << result->encodedContent();

    QByteArray body = MessageCore::NodeHelper::firstChild(result)->body();
    QCOMPARE(QString::fromUtf8(body),
             QStringLiteral("=3D2D Magic foo\n=46rom test\n\n=2D- quaak\nOhno"));

    ComposerTestUtil::verify(true, false, result, data.toUtf8(),
                             Kleo::OpenPGPMIMEFormat, cte);
}

void SignJobTest::testMixedContent()
{
    std::vector< GpgME::Key > keys = MessageComposer::Test::getKeys();

    QString data = QString::fromUtf8("=2D Magic foo\nFrom test\n\n-- quaak\nOhno");

    MessageComposer::Composer *composer = new MessageComposer::Composer;
    MessageComposer::SignJob *sJob = new MessageComposer::SignJob(composer);

    QVERIFY(composer);
    QVERIFY(sJob);

    KMime::Content *content = new KMime::Content;
    content->contentType()->setMimeType(QByteArrayLiteral("multipart/mixed"));
    content->contentType()->setBoundary(KMime::multiPartBoundary());
    KMime::Content *subcontent = new KMime::Content;
    subcontent->contentType()->setMimeType(QByteArrayLiteral("text/plain"));
    subcontent->setBody(data.toUtf8());
    KMime::Content *attachment = new KMime::Content;
    attachment->contentType()->setMimeType(QByteArrayLiteral("text/plain"));
    QByteArray attachmentData("an attachment");
    attachment->setBody(attachmentData);

    content->addContent(subcontent);
    content->addContent(attachment);
    content->assemble();

    sJob->setContent(content);
    sJob->setCryptoMessageFormat(Kleo::OpenPGPMIMEFormat);
    sJob->setSigningKeys(keys);

    VERIFYEXEC(sJob);

    KMime::Content *result = sJob->content();
    result->assemble();
    qDebug() << result->encodedContent();

    KMime::Content *firstChild = MessageCore::NodeHelper::firstChild(result);
    QCOMPARE(result->contents().count(), 2);
    QCOMPARE(firstChild->contents().count(), 2);
    QCOMPARE(firstChild->body(), QByteArray());
    QCOMPARE(firstChild->contentType()->mimeType(), QByteArrayLiteral("multipart/mixed"));
    QCOMPARE(firstChild->contents()[0]->body(), data.toUtf8());
    QCOMPARE(firstChild->contents()[1]->body(), attachmentData);

    ComposerTestUtil::verify(true, false, result, data.toUtf8(),
                             Kleo::OpenPGPMIMEFormat, KMime::Headers::CE7Bit);
}

void SignJobTest::checkSignJob(MessageComposer::SignJob *sJob)
{

    VERIFYEXEC(sJob);

    KMime::Content *result = sJob->content();
    Q_ASSERT(result);
    result->assemble();

    ComposerTestUtil::verifySignature(result, QString::fromLocal8Bit("one flew over the cuckoo's nest").toUtf8(), Kleo::OpenPGPMIMEFormat, KMime::Headers::CE7Bit);
}

