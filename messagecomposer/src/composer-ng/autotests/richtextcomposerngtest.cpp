/*
   Copyright (C) 2016 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#include "richtextcomposerngtest.h"
#include "../richtextcomposerng.h"

#include <PimCommon/AutoCorrection>
#include <QTest>

RichTextComposerNgTest::RichTextComposerNgTest(QObject *parent)
    : QObject(parent)
{

}

RichTextComposerNgTest::~RichTextComposerNgTest()
{

}

void RichTextComposerNgTest::shouldHaveDefaultValue()
{
    MessageComposer::RichTextComposerNg richtextComposerNg;
    QVERIFY(richtextComposerNg.composerSignature());
    QVERIFY(!richtextComposerNg.autocorrection());
}


void RichTextComposerNgTest::shouldForceAutoCorrection_data()
{
    QTest::addColumn<QString>("original");
    QTest::addColumn<QString>("expected");
    //FIXME first char !
    QTest::newRow("test1") << QStringLiteral("boo bla bli. foo faa") << QStringLiteral("boo bla bli. Foo faa");
    QTest::newRow("test2") << QStringLiteral("boo bla bli.\nfoo faa") << QStringLiteral("boo bla bli.\nFoo faa");
    QTest::newRow("test3") << QStringLiteral("\nboo bla bli.\nfoo faa") << QStringLiteral("\nBoo bla bli.\nFoo faa");

}

void RichTextComposerNgTest::shouldForceAutoCorrection()
{
    QFETCH(QString, original);
    QFETCH(QString, expected);

    MessageComposer::RichTextComposerNg richtextComposerNg;

    richtextComposerNg.setPlainText(original);
    PimCommon::AutoCorrection autocorrection;
    autocorrection.setEnabledAutoCorrection(true);
    autocorrection.setUppercaseFirstCharOfSentence(true);
    richtextComposerNg.setAutocorrection(&autocorrection);
    richtextComposerNg.forceAutoCorrection(false);

    QCOMPARE(richtextComposerNg.toPlainText(), expected);
}

void RichTextComposerNgTest::shouldForceAutoCorrectionWithSelection_data()
{
    QTest::addColumn<QString>("original");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<int>("selectionStart");
    QTest::addColumn<int>("selectionEnd");
    QTest::newRow("noselection") << QStringLiteral("\nboo bla bli.\nfoo faa") << QStringLiteral("\nBoo bla bli.\nFoo faa") << 0 << 0;
    QTest::newRow("noselection2") << QStringLiteral("\nboo bla bli.\nfoo faa") << QStringLiteral("\nBoo bla bli.\nFoo faa") << 1 << 1;
    QTest::newRow("fullselection") << QStringLiteral("\nboo bla bli.\nfoo faa") << QStringLiteral("\nBoo bla bli.\nFoo faa") << 0 << 22;
    QTest::newRow("selection1") << QStringLiteral("\nboo bla bli.\nfoo faa") << QStringLiteral("\nBoo bla bli.\nfoo faa") << 0 << 10;
    QTest::newRow("selection1") << QStringLiteral("\nboo bla bli.\nfoo faa") << QStringLiteral("\nboo bla bli.\nfoo faa") << 5 << 10;
}

void RichTextComposerNgTest::shouldForceAutoCorrectionWithSelection()
{
    QFETCH(QString, original);
    QFETCH(QString, expected);
    QFETCH(int, selectionStart);
    QFETCH(int, selectionEnd);

    MessageComposer::RichTextComposerNg richtextComposerNg;

    richtextComposerNg.setPlainText(original);
    QTextCursor cur = richtextComposerNg.textCursor();
    cur.setPosition(selectionStart);
    cur.setPosition(selectionEnd, QTextCursor::KeepAnchor);
    richtextComposerNg.setTextCursor(cur);
    PimCommon::AutoCorrection autocorrection;
    autocorrection.setEnabledAutoCorrection(true);
    autocorrection.setUppercaseFirstCharOfSentence(true);
    richtextComposerNg.setAutocorrection(&autocorrection);
    richtextComposerNg.forceAutoCorrection(true);

    QCOMPARE(richtextComposerNg.toPlainText(), expected);
}

QTEST_MAIN(RichTextComposerNgTest)