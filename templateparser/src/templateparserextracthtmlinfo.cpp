/*
   Copyright (C) 2017 Laurent Montel <montel@kde.org>

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

#include "templateparserextracthtmlinfo.h"
#include "templateextracthtmlelementwebengineview.h"
#include "templatewebengineview.h"
#include "templateparser_debug.h"

using namespace TemplateParser;

TemplateParserExtractHtmlInfo::TemplateParserExtractHtmlInfo(QObject *parent)
    : QObject(parent),
      mTemplateWebEngineView(nullptr),
      mExtractHtmlElementWebEngineView(nullptr)
{

}

TemplateParserExtractHtmlInfo::~TemplateParserExtractHtmlInfo()
{
    delete mTemplateWebEngineView;
    delete mExtractHtmlElementWebEngineView;
}

void TemplateParserExtractHtmlInfo::setHtmlForExtractingTextPlain(const QString &html)
{
    mHtmlForExtractingTextPlain = html;
}

void TemplateParserExtractHtmlInfo::setHtmlForExtractionHeaderAndBody(const QString &html)
{
    mHtmlForExtractionHeaderAndBody = html;
}

void TemplateParserExtractHtmlInfo::setTemplate(const QString &str)
{
    mTemplateStr = str;
}

void TemplateParserExtractHtmlInfo::start()
{
    mResult.clear();
    mResult.mTemplate = mTemplateStr;
    if (!mHtmlForExtractingTextPlain.isEmpty()) {
        mTemplateWebEngineView = new TemplateWebEngineView;
        connect(mTemplateWebEngineView, &TemplateWebEngineView::loadContentDone, this, &TemplateParserExtractHtmlInfo::slotExtractToPlainTextFinished);
        mTemplateWebEngineView->setHtmlContent(mHtmlForExtractingTextPlain);
    } else {
        qCWarning(TEMPLATEPARSER_LOG) << "html string is empty for extracting to plainText";
        slotExtractToPlainTextFinished(false);
    }
}

void TemplateParserExtractHtmlInfo::slotExtractToPlainTextFinished(bool success)
{
    if (!success) {
        qCWarning(TEMPLATEPARSER_LOG) << "Impossible to extract plaintext";
    } else {
        mResult.mPlainText = mTemplateWebEngineView->plainText();
    }
    if (!mHtmlForExtractionHeaderAndBody.isEmpty()) {
        mExtractHtmlElementWebEngineView = new TemplateExtractHtmlElementWebEngineView;
        connect(mExtractHtmlElementWebEngineView, &TemplateExtractHtmlElementWebEngineView::loadContentDone, this, &TemplateParserExtractHtmlInfo::slotExtractHtmlElementFinished);
        mExtractHtmlElementWebEngineView->setHtmlContent(mHtmlForExtractionHeaderAndBody);
    } else {
        qCWarning(TEMPLATEPARSER_LOG) << "html string is empty for extracting to header and body";
        slotExtractHtmlElementFinished(false);
    }
}

void TemplateParserExtractHtmlInfo::slotExtractHtmlElementFinished(bool success)
{
    if (!success) {
        qCWarning(TEMPLATEPARSER_LOG) << "Impossible to extract html element";
    } else {
        mResult.mBodyElement = mExtractHtmlElementWebEngineView->bodyElement();
        mResult.mHeaderElement = mExtractHtmlElementWebEngineView->headerElement();
        mResult.mHtmlElement = mExtractHtmlElementWebEngineView->htmlElement();
    }
    Q_EMIT finished(mResult);
    deleteLater();
}
