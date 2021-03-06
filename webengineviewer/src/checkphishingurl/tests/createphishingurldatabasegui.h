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

#ifndef CREATEPHISHINGURLDATABASEGUI_H
#define CREATEPHISHINGURLDATABASEGUI_H

#include <QWidget>
#include "../createphishingurldatabasejob.h"
class QPlainTextEdit;
class QComboBox;
class CreatePhisingUrlDataBaseGui : public QWidget
{
    Q_OBJECT
public:
    explicit CreatePhisingUrlDataBaseGui(QWidget *parent = nullptr);
    ~CreatePhisingUrlDataBaseGui();
private Q_SLOTS:
    void slotResult(const QByteArray &data);
    void slotDownloadFullDatabase();
    void slotDebugJSon(const QByteArray &data);
    void slotDownloadPartialDatabase();
    void slotSaveResultToDisk();
private:
    WebEngineViewer::CreatePhishingUrlDataBaseJob::ContraintsCompressionType compressionType();
    void clear();
    QPlainTextEdit *mResult;
    QPlainTextEdit *mJson;
    QComboBox *mCompressionType;
};

#endif // CREATEPHISHINGURLDATABASEGUI_H
