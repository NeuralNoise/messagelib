/*
  Copyright (c) 2016 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "testwebengine.h"
#include <QHBoxLayout>
#include <QWebEngineView>
#include <MessageViewer/WebHitTestResult>
#include <MessageViewer/WebEnginePage>
#include <QDebug>
#include <QContextMenuEvent>
TestWebEngine::TestWebEngine(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *hboxLayout = new QHBoxLayout(this);
    QWebEngineView *pageView = new QWebEngineView(this);
    hboxLayout->addWidget(pageView);
    mEnginePage = new MessageViewer::WebEnginePage(this);
    pageView->setPage(mEnginePage);
    pageView->setContextMenuPolicy(Qt::DefaultContextMenu);
    setContextMenuPolicy(Qt::CustomContextMenu);

    pageView->load(QUrl(QStringLiteral("http://www.kde.org")));
    //connect(pageView, &QWebEngineView::showContextMenu, this, &TestWebEngine::slotShowContextMenu);
}

TestWebEngine::~TestWebEngine()
{

}

#if 0
void TestWebEngine::slotShowContextMenu()
{

}
#endif
void TestWebEngine::contextMenuEvent(QContextMenuEvent *e)
{
    qDebug()<<" void TestWebEngine::contextMenuEvent(QContextMenuEvent *e)";
    const MessageViewer::WebHitTestResult webHit = mEnginePage->hitTestContent(e->pos());

    qDebug() << " alternateText"<<webHit.alternateText();
    qDebug() << " boundingRect"<<webHit.boundingRect();
    qDebug() << " imageUrl"<<webHit.imageUrl();
    qDebug() << " isContentEditable"<<webHit.isContentEditable();
    qDebug() << " isContentSelected"<<webHit.isContentSelected();
    qDebug() << " isNull"<<webHit.isNull();
    qDebug() << " linkTitle"<<webHit.linkTitle();
    qDebug() << " linkUrl"<<webHit.linkUrl();
    qDebug() << " mediaUrl"<<webHit.mediaUrl();
    qDebug() << " mediaPaused"<<webHit.mediaPaused();
    qDebug() << " mediaMuted"<<webHit.mediaMuted();
    qDebug() << " pos"<<webHit.pos();
    qDebug() << " tagName"<<webHit.tagName();
}
