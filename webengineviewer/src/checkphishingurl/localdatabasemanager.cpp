/*
   Copyright (C) 2016 Laurent Montel <montel@kde.org>

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
#include "localdatabasemanager.h"
#include "webengineviewer_debug.h"
#include "checkphishingurlfromlocaldatabasejob.h"
#include "createphishingurldatabasejob.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QDir>
#include <QTimer>

using namespace WebEngineViewer;

Q_GLOBAL_STATIC(LocalDataBaseManager, s_localDataBaseManager)

namespace {
inline QString tableName() {
    return QStringLiteral("malware");
}
inline QString localDataBasePath() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/phishingurl/");
}
inline QString sqlFileName() {
    return QStringLiteral("/malwaredb.sql");
}
}

LocalDataBaseManager::LocalDataBaseManager(QObject *parent)
    : QObject(parent),
      mRegularCheckDataBaseTimer(Q_NULLPTR),
      mDataBaseOk(false),
      mDownloadProgress(false)
{
}

LocalDataBaseManager::~LocalDataBaseManager()
{
    if (mDataBaseOk) {
        mDataBase.close();
    }
}

void LocalDataBaseManager::closeDataBaseAndDeleteIt()
{
    if (mDataBaseOk) {
        mDataBase.close();
        QFile f(localDataBasePath() + sqlFileName());
        if (!f.remove()) {
            qCWarning(WEBENGINEVIEWER_LOG) << "impossible to remove local database file";
        }
    }
}

void LocalDataBaseManager::readConfig()
{
    //TODO
}

void LocalDataBaseManager::saveConfig()
{
    //TODO
}

void LocalDataBaseManager::downloadPartialDataBase()
{
    mDownloadProgress = true;
    WebEngineViewer::CreatePhishingUrlDataBaseJob *job = new WebEngineViewer::CreatePhishingUrlDataBaseJob(this);
    job->setDataBaseDownloadNeeded(WebEngineViewer::CreatePhishingUrlDataBaseJob::UpdateDataBase);
    job->setDataBaseState(QString()); //TODO
    connect(job, &CreatePhishingUrlDataBaseJob::finished, this, &LocalDataBaseManager::slotDownloadDataBaseFinished);
    job->start();
}

void LocalDataBaseManager::downloadFullDataBase()
{
    mDownloadProgress = true;
    WebEngineViewer::CreatePhishingUrlDataBaseJob *job = new WebEngineViewer::CreatePhishingUrlDataBaseJob(this);
    job->setDataBaseDownloadNeeded(WebEngineViewer::CreatePhishingUrlDataBaseJob::FullDataBase);
    connect(job, &CreatePhishingUrlDataBaseJob::finished, this, &LocalDataBaseManager::slotDownloadDataBaseFinished);
    job->start();
}

void LocalDataBaseManager::initialize()
{
    if (mDownloadProgress) {
        return;
    }
    if (!mDataBaseOk) {
        bool initDatabaseSuccess = initializeDataBase();
        if (initDatabaseSuccess) {
            if (!mDataBase.tables().contains(tableName())) {
                if (createTable()) {
                    downloadFullDataBase();
                } else {
                    qCWarning(WEBENGINEVIEWER_LOG) << "Impossible to create Table";
                }
            } else {
                mDataBaseOk = true;
            }
        }
    } else {
        qCWarning(WEBENGINEVIEWER_LOG) << "Database already initialized.";
    }
    if (mDataBaseOk) {
        if (!mRegularCheckDataBaseTimer) {
            mRegularCheckDataBaseTimer = new QTimer(this);
            mRegularCheckDataBaseTimer->setSingleShot(true);
            mRegularCheckDataBaseTimer->setInterval(60*1000*60*5); //Each 5 hours //Perhaps improve it.
            connect(mRegularCheckDataBaseTimer, &QTimer::timeout, this, &LocalDataBaseManager::slotCheckDataBase);
            mRegularCheckDataBaseTimer->start();
        }
    }
}

void LocalDataBaseManager::slotCheckDataBase()
{
    if (mDataBaseOk && !mDownloadProgress) {
        downloadPartialDataBase();
    }
}

void LocalDataBaseManager::slotDownloadDataBaseFinished(const WebEngineViewer::UpdateDataBaseInfo &infoDataBase,
                                                            WebEngineViewer::CreatePhishingUrlDataBaseJob::DataBaseDownloadResult status)
{

    qDebug() << "LocalDataBaseManager::slotDownloadFullDataBaseFinished "<<status;
    switch(status) {
    case CreatePhishingUrlDataBaseJob::InvalidData:
        qCWarning(WEBENGINEVIEWER_LOG) << "Invalid Data.";
        mDataBaseOk = false;
        break;
    case CreatePhishingUrlDataBaseJob::ValidData:
        qCWarning(WEBENGINEVIEWER_LOG) << "Valid Data.";
        mDataBaseOk = true;
        break;
    case CreatePhishingUrlDataBaseJob::UnknownError:
        qCWarning(WEBENGINEVIEWER_LOG) << "Unknown data.";
        mDataBaseOk = false;
        break;
    case CreatePhishingUrlDataBaseJob::BrokenNetwork:
        qCWarning(WEBENGINEVIEWER_LOG) << "Broken Networks.";
        mDataBaseOk = false;
        break;
    }
    if (mDataBaseOk) {
        //qDebug() << "infoDataBase" << infoDataBase.additionList.count();
        Q_FOREACH(const Addition &add, infoDataBase.additionList) {
            //qDebug() << " add.size" << add.prefixSize;
            //qDebug() << " add.hash" << QByteArray::fromBase64(add.hashString).size();
            const QByteArray uncompressed = QByteArray::fromBase64(add.hashString);
            for (int i = 0; i < uncompressed.size();) {
                QByteArray m = uncompressed.mid(i, add.prefixSize);
                i += add.prefixSize;
                //qDebug() << "m " << m << " m.size" << m.size();
            }
        }
    }
    mDownloadProgress = false;
    //We finish to download restart timer if necessary
    if (mRegularCheckDataBaseTimer && !mRegularCheckDataBaseTimer->isActive()) {
        mRegularCheckDataBaseTimer->start();
    }
}

LocalDataBaseManager *LocalDataBaseManager::self()
{
    return s_localDataBaseManager;
}

bool LocalDataBaseManager::initializeDataBase()
{
    mDataBase = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    QDir().mkpath(localDataBasePath());
    mDataBase.setDatabaseName(localDataBasePath() + sqlFileName());
    if (!mDataBase.open()) {
        qCWarning(WEBENGINEVIEWER_LOG) << "Impossible to open DataBase: " << mDataBase.lastError().text();
        return false;
    }
    return true;
}

bool LocalDataBaseManager::createTable()
{
    QSqlQuery query(mDataBase);
    return query.exec(QStringLiteral("create table %1 (id int primary key, "
                              "hash varchar(32))").arg(tableName()));
}

void LocalDataBaseManager::checkUrl(const QUrl &url)
{
    if (mDataBaseOk) {

    } else {
        Q_EMIT checkUrlFinished(url, WebEngineViewer::LocalDataBaseManager::Unknown);
    }
    //TODO
}
