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

#ifndef CHECKPHISHINGURLUTILTEST_H
#define CHECKPHISHINGURLUTILTEST_H

#include <QObject>

class CheckPhishingUrlUtilTest : public QObject
{
    Q_OBJECT
public:
    explicit CheckPhishingUrlUtilTest(QObject *parent = nullptr);
    ~CheckPhishingUrlUtilTest();
private Q_SLOTS:
    void shouldConvertToSecond_data();
    void shouldConvertToSecond();

    void shouldCacheIsStillValid_data();
    void shouldCacheIsStillValid();

    void shouldGenerateBackModeDelay_data();
    void shouldGenerateBackModeDelay();
};

#endif // CHECKPHISHINGURLUTILTEST_H
