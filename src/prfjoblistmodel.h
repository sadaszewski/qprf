//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFJOBLISTMODEL_H
#define PRFJOBLISTMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QDateTime>

#include "prfjob.h"

class PrfJobListModel : public QAbstractItemModel
{
    Q_OBJECT

    struct Entry {
        QDateTime added;
        QDateTime started;
        QDateTime finished;
        PrfJob *job;
        QString status;
        QString progress;
    };

public:
    PrfJobListModel();


    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex parent(const QModelIndex &child) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void addJob(PrfJob*);
    void removeJob(PrfJob*);
    void requestInterruption(int);
    PrfJob* job(int);

public slots:
    void jobDataChanged(PrfJob*);
    void jobStarted(PrfJob*);
    void jobFinished(PrfJob*);
    void jobStatusChanged(PrfJob*, const QString&);
    void jobProgressChanged(PrfJob*, const QString&);
    void jobInterrupted(PrfJob*);
    void jobThreadFinished();

signals:
    void addedJob(PrfJob*);
    void finishedJob(PrfJob*);

private:
    QList<Entry> m_Jobs;
};

#endif // PRFJOBLISTMODEL_H
