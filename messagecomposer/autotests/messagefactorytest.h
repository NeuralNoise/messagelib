/*
  Copyright (C) 2010 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>

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

#ifndef MESSAGEFACTORY_TEST_H
#define MESSAGEFACTORY_TEST_H

#include <QObject>
#include <kmime/kmime_message.h>

class MessageFactoryTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();

    void testCreateReplyAll();
    void testCreateReplyHtml();
    void testCreateReplyUTF16Base64();
    void testCreateForward();
    void testCreateRedirect();
    void testCreateResend();
    void testCreateMDN();
    void testCreateRedirectToAndCC();
    void testCreateRedirectToAndCCAndBCC();

    //MultiPart
    void test_multipartAlternative_data();
    void test_multipartAlternative();

    void testCreateForwardMultiEmails();
    void testCreateReplyToAuthor();
    void testCreateReplyAllWithMultiEmails();
    void testCreateReplyToList();
    void testCreateReplyToAllWithUseSender();

private:
    KMime::Message::Ptr createPlainTestMessage();
    KMime::Message::Ptr loadMessageFromFile(QString filename);
    KMime::Message::Ptr createPlainTestMessageWithMultiEmails();
    KMime::Message::Ptr loadMessage(const QString &filename);
};

#endif
