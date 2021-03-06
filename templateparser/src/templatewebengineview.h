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

#ifndef TEMPLATEWEBENGINEVIEW_H
#define TEMPLATEWEBENGINEVIEW_H

#include <QWebEngineView>
#include "templateparser_export.h"

namespace TemplateParser
{
class TemplateWebEnginePage;
class TEMPLATEPARSER_EXPORT TemplateWebEngineView : public QWebEngineView
{
    Q_OBJECT
public:
    explicit TemplateWebEngineView(QWidget *parent = nullptr);
    ~TemplateWebEngineView();

    void setHtmlContent(const QString &html);

    QString plainText() const;

Q_SIGNALS:
    void loadContentDone(bool success);

private:
    void slotLoadFinished(bool ok);
    void setPlainText(const QString &plainText);

    QString mExtractedPlainText;
    TemplateWebEnginePage *mPage;
};
}
#endif // TEMPLATEWEBENGINEVIEW_H
