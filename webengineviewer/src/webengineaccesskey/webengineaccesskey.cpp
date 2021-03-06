/*
   Copyright (C) 2016-2017 Laurent Montel <montel@kde.org>

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

#include "webengineaccesskey.h"
#include "webengineaccesskeyanchor.h"
#include "webengineaccesskeyutils.h"
#include "webenginemanagescript.h"


#include <KActionCollection>
#include <QKeyEvent>
#include <QLabel>
#include <QAction>
#include <QWebEngineView>
#include <QToolTip>
#include <QDebug>
using namespace WebEngineViewer;
template<typename Arg, typename R, typename C>
struct InvokeWrapper {
    R *receiver;
    void (C::*memberFunction)(Arg);
    void operator()(Arg result)
    {
        (receiver->*memberFunction)(result);
    }
};

template<typename Arg, typename R, typename C>

InvokeWrapper<Arg, R, C> invoke(R *receiver, void (C::*memberFunction)(Arg))
{
    InvokeWrapper<Arg, R, C> wrapper = {receiver, memberFunction};
    return wrapper;
}

class WebEngineViewer::WebEngineAccessKeyPrivate
{
public:
    enum AccessKeyState {
        NotActivated,
        PreActivated,
        Activated
    };

    WebEngineAccessKeyPrivate(WebEngineAccessKey *qq, QWebEngineView *webEngine)
        : mWebEngine(webEngine),
          mAccessKeyActivated(NotActivated),
          mActionCollection(nullptr),
          q(qq)
    {

    }
    void makeAccessKeyLabel(QChar accessKey, const WebEngineViewer::WebEngineAccessKeyAnchor &element);
    bool checkForAccessKey(QKeyEvent *event);
    QList<QLabel *> mAccessKeyLabels;
    QHash<QChar, WebEngineViewer::WebEngineAccessKeyAnchor> mAccessKeyNodes;
    QHash<QString, QChar> mDuplicateLinkElements;
    QWebEngineView *mWebEngine;
    AccessKeyState mAccessKeyActivated;
    KActionCollection *mActionCollection;
    WebEngineAccessKey *q;
};

static QString linkElementKey(const WebEngineViewer::WebEngineAccessKeyAnchor &element, const QUrl &baseUrl)
{
    //qDebug()<<" element.href()"<<element.href();
    if (!element.href().isEmpty()) {
        const QUrl url = baseUrl.resolved(QUrl(element.href()));
        //qDebug()<< "URL " << url;
        QString linkKey(url.toString());
        if (!element.target().isEmpty()) {
            linkKey += QLatin1Char('+');
            linkKey += element.target();
        }
        return linkKey;
    }
    return QString();
}

static void handleDuplicateLinkElements(const WebEngineViewer::WebEngineAccessKeyAnchor &element, QHash<QString, QChar> *dupLinkList, QChar *accessKey, const QUrl &baseUrl)
{
    if (element.tagName().compare(QLatin1String("A"), Qt::CaseInsensitive) == 0) {
        const QString linkKey(linkElementKey(element, baseUrl));
        //qDebug() << "LINK KEY:" << linkKey;
        if (dupLinkList->contains(linkKey)) {
            //qDebug() << "***** Found duplicate link element:" << linkKey;
            *accessKey = dupLinkList->value(linkKey);
        } else if (!linkKey.isEmpty()) {
            dupLinkList->insert(linkKey, *accessKey);
        }
        if (linkKey.isEmpty()) {
            *accessKey = QChar();
        }
    }
}

static bool isHiddenElement(const WebEngineViewer::WebEngineAccessKeyAnchor &element)
{
    // width or height property set to less than zero
    if (element.boundingRect().width() < 1 || element.boundingRect().height()  < 1) {
        return true;
    }
#if 0

    // visibility set to 'hidden' in the element itself or its parent elements.
    if (element.styleProperty(QStringLiteral("visibility"), QWebElement::ComputedStyle).compare(QLatin1String("hidden"), Qt::CaseInsensitive) == 0) {
        return true;
    }

    // display set to 'none' in the element itself or its parent elements.
    if (element.styleProperty(QStringLiteral("display"), QWebElement::ComputedStyle).compare(QLatin1String("none"), Qt::CaseInsensitive) == 0) {
        return true;
    }
#endif
    return false;
}

bool WebEngineAccessKeyPrivate::checkForAccessKey(QKeyEvent *event)
{
    if (mAccessKeyLabels.isEmpty()) {
        return false;
    }
    QString text = event->text();
    if (text.isEmpty()) {
        return false;
    }
    QChar key = text.at(0).toUpper();
    bool handled = false;
    if (mAccessKeyNodes.contains(key)) {
        WebEngineViewer::WebEngineAccessKeyAnchor element = mAccessKeyNodes[key];
        if (element.tagName().compare(QLatin1String("A"), Qt::CaseInsensitive) == 0) {
            const QString linkKey(linkElementKey(element, mWebEngine->url()));
            if (!linkKey.isEmpty()) {
                //qDebug()<<" WebEngineAccessKey::checkForAccessKey****"<<linkKey;
                Q_EMIT q->openUrl(QUrl(linkKey));
                handled = true;
            }
        }
    }
    return handled;
}

void WebEngineAccessKeyPrivate::makeAccessKeyLabel(QChar accessKey, const WebEngineViewer::WebEngineAccessKeyAnchor &element)
{
    //qDebug()<<" void WebEngineAccessKey::makeAccessKeyLabel(QChar accessKey, const WebEngineViewer::MailWebEngineAccessKeyAnchor &element)";
    QLabel *label = new QLabel(mWebEngine);
    QFont font(label->font());
    font.setBold(true);
    label->setFont(font);
    label->setText(accessKey);
    QFontMetrics metric(label->font());
    label->setFixedWidth(metric.width(QStringLiteral("WW")));
    label->setPalette(QToolTip::palette());
    label->setAutoFillBackground(true);
    label->setFrameStyle(QFrame::Box | QFrame::Plain);
    QPoint point = element.boundingRect().center();
    label->move(point);
    label->show();
    point.setX(point.x() - label->width() / 2);
    label->move(point);
    mAccessKeyLabels.append(label);
    mAccessKeyNodes.insertMulti(accessKey, element);
}

WebEngineAccessKey::WebEngineAccessKey(QWebEngineView *webEngine, QObject *parent)
    : QObject(parent),
      d(new WebEngineViewer::WebEngineAccessKeyPrivate(this, webEngine))
{
    //qDebug() << " WebEngineAccessKey::WebEngineAccessKey(QWebEngineView *webEngine, QObject *parent)";
}

WebEngineAccessKey::~WebEngineAccessKey()
{
    delete d;
}

void WebEngineAccessKey::setActionCollection(KActionCollection *ac)
{
    d->mActionCollection = ac;
}

void WebEngineAccessKey::wheelEvent(QWheelEvent *e)
{
    hideAccessKeys();
    if (d->mAccessKeyActivated == WebEngineAccessKeyPrivate::PreActivated && (e->modifiers() & Qt::ControlModifier)) {
        d->mAccessKeyActivated = WebEngineAccessKeyPrivate::NotActivated;
    }
}

void WebEngineAccessKey::resizeEvent(QResizeEvent *)
{
    if (d->mAccessKeyActivated == WebEngineAccessKeyPrivate::Activated) {
        hideAccessKeys();
    }
}

void WebEngineAccessKey::keyPressEvent(QKeyEvent *e)
{
    if (e && d->mWebEngine->hasFocus()) {
        if (d->mAccessKeyActivated == WebEngineAccessKeyPrivate::Activated) {
            if (d->checkForAccessKey(e)) {
                hideAccessKeys();
                e->accept();
                return;
            }
            hideAccessKeys();
        } else if (e->key() == Qt::Key_Control && e->modifiers() == Qt::ControlModifier
#if 0 //FIXME
                   && !isEditableElement(d->mWebView->page())
#endif
                  ) {
            d->mAccessKeyActivated = WebEngineAccessKeyPrivate::PreActivated; // Only preactive here, it will be actually activated in key release.
        }
    }
}

void WebEngineAccessKey::keyReleaseEvent(QKeyEvent *e)
{
    //qDebug() << " void WebEngineAccessKey::keyReleaseEvent(QKeyEvent *e)";
    if (d->mAccessKeyActivated == WebEngineAccessKeyPrivate::PreActivated) {
        // Activate only when the CTRL key is pressed and released by itself.
        if (e->key() == Qt::Key_Control && e->modifiers() == Qt::NoModifier) {
            showAccessKeys();

        } else {
            d->mAccessKeyActivated = WebEngineAccessKeyPrivate::NotActivated;
        }
    }
}

void WebEngineAccessKey::hideAccessKeys()
{
    if (!d->mAccessKeyLabels.isEmpty()) {
        for (int i = 0, count = d->mAccessKeyLabels.count(); i < count; ++i) {
            QLabel *label = d->mAccessKeyLabels[i];
            label->hide();
            label->deleteLater();
        }
        d->mAccessKeyLabels.clear();
        d->mAccessKeyNodes.clear();
        d->mDuplicateLinkElements.clear();
        d->mAccessKeyActivated = WebEngineAccessKeyPrivate::NotActivated;
        d->mWebEngine->update();
    }
}

void WebEngineAccessKey::handleSearchAccessKey(const QVariant &res)
{
    //qDebug() << " void WebEngineAccessKey::handleSearchAccessKey(const QVariant &res)" << res;
    const QVariantList lst = res.toList();
    QVector<WebEngineViewer::WebEngineAccessKeyAnchor> anchorList;
    anchorList.reserve(lst.count());
    for (const QVariant &var : lst) {
        //qDebug()<<" var"<<var;
        anchorList << WebEngineViewer::WebEngineAccessKeyAnchor(var);
    }

    QList<QChar> unusedKeys;
    unusedKeys.reserve(10 + ('Z' - 'A' + 1));
    for (char c = 'A'; c <= 'Z'; ++c) {
        unusedKeys << QLatin1Char(c);
    }
    for (char c = '0'; c <= '9'; ++c) {
        unusedKeys << QLatin1Char(c);
    }
    if (d->mActionCollection) {
        for (QAction *act : d->mActionCollection->actions()) {
            QAction *a = qobject_cast<QAction *>(act);
            if (a) {
                const QKeySequence shortCut = a->shortcut();
                if (!shortCut.isEmpty()) {
                    Q_FOREACH (QChar c, unusedKeys) { //Don't use for(..:..)
                        if (shortCut.matches(QKeySequence(c)) != QKeySequence::NoMatch) {
                            unusedKeys.removeOne(c);
                        }
                    }
                }
            }
        }
    }
    QVector<WebEngineViewer::WebEngineAccessKeyAnchor> unLabeledElements;
    QRect viewport = d->mWebEngine->rect();
    for (const WebEngineViewer::WebEngineAccessKeyAnchor &element : qAsConst(anchorList)) {
        const QRect geometry = element.boundingRect();
        if (geometry.size().isEmpty() || !viewport.contains(geometry.topLeft())) {
            continue;
        }
        if (isHiddenElement(element)) {
            continue;    // Do not show access key for hidden elements...
        }
        const QString accessKeyAttribute(element.accessKey().toUpper());
        if (accessKeyAttribute.isEmpty()) {
            unLabeledElements.append(element);
            continue;
        }
        QChar accessKey;
        for (int i = 0; i < accessKeyAttribute.count(); i += 2) {
            const QChar &possibleAccessKey = accessKeyAttribute[i];
            if (unusedKeys.contains(possibleAccessKey)) {
                accessKey = possibleAccessKey;
                break;
            }
        }
        if (accessKey.isNull()) {
            unLabeledElements.append(element);
            continue;
        }

        handleDuplicateLinkElements(element, &d->mDuplicateLinkElements, &accessKey, d->mWebEngine->url());
        if (!accessKey.isNull()) {
            unusedKeys.removeOne(accessKey);
            d->makeAccessKeyLabel(accessKey, element);
        }
    }

    // Pick an access key first from the letters in the text and then from the
    // list of unused access keys
    for (const WebEngineViewer::WebEngineAccessKeyAnchor &element : qAsConst(unLabeledElements)) {
        const QRect geometry = element.boundingRect();
        if (unusedKeys.isEmpty()
                || geometry.size().isEmpty()
                || !viewport.contains(geometry.topLeft())) {
            continue;
        }
        QChar accessKey;
        const QString text = element.innerText().toUpper();
        for (int i = 0; i < text.count(); ++i) {
            const QChar &c = text.at(i);
            if (unusedKeys.contains(c)) {
                accessKey = c;
                break;
            }
        }
        if (accessKey.isNull()) {
            accessKey = unusedKeys.takeFirst();
        }

        handleDuplicateLinkElements(element, &d->mDuplicateLinkElements, &accessKey, d->mWebEngine->url());
        if (!accessKey.isNull()) {
            unusedKeys.removeOne(accessKey);
            d->makeAccessKeyLabel(accessKey, element);
        }
    }
    d->mAccessKeyActivated = (!d->mAccessKeyLabels.isEmpty() ? WebEngineAccessKeyPrivate::Activated : WebEngineAccessKeyPrivate::NotActivated);
}

void WebEngineAccessKey::showAccessKeys()
{
    d->mAccessKeyActivated = WebEngineAccessKeyPrivate::Activated;
    d->mWebEngine->page()->runJavaScript(WebEngineViewer::WebEngineAccessKeyUtils::script(),
                                         WebEngineManageScript::scriptWordId(),
                                         invoke(this, &WebEngineAccessKey::handleSearchAccessKey));
}

