//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "prfjoblistmodel.h"

#include <QDebug>

PrfJobListModel::PrfJobListModel()
{

}

QModelIndex PrfJobListModel::index(int row, int column, const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return createIndex(row, column);
}

int PrfJobListModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;

    return 8;
}

int PrfJobListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;

    return m_Jobs.size();
}

QVariant PrfJobListModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    switch(index.column()) {
    case 0: return m_Jobs[index.row()].job->metaObject()->className();
    case 1: return m_Jobs[index.row()].status;
    case 2: return m_Jobs[index.row()].job->parameters();
    case 3: return m_Jobs[index.row()].added;
    case 4: return m_Jobs[index.row()].started;
    case 5: return m_Jobs[index.row()].finished;
    case 6: {
        const Entry &e(m_Jobs[index.row()]);
        if (e.job->isRunning())
            return e.started.secsTo(QDateTime::currentDateTime());
        else if (e.job->isFinished())
            return e.started.secsTo(e.finished);
        else
            return "";
    }
    case 7: return m_Jobs[index.row()].progress;
    default: return "UNKNOWN";
    }
}

QModelIndex PrfJobListModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

QVariant PrfJobListModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Vertical)
        return QString("%1").arg(section + 1);

    switch (section) {
    case 0: return "Name";
    case 1: return "Status";
    case 2: return "Parameters";
    case 3: return "Added Date";
    case 4: return "Started Date";
    case 5: return "Finished Date";
    case 6: return "Time Elapsed (s)";
    case 7: return "Progress";
    default: return "UNKNOWN";
    }
}

void PrfJobListModel::addJob(PrfJob *job) {
    connect(job, SIGNAL(jobDataChanged(PrfJob*)), this, SLOT(jobDataChanged(PrfJob*)));
    connect(job, SIGNAL(jobStarted(PrfJob*)), this, SLOT(jobStarted(PrfJob*)));
    connect(job, SIGNAL(jobFinished(PrfJob*)), this, SLOT(jobFinished(PrfJob*)));
    connect(job, SIGNAL(statusChanged(PrfJob*,QString)), this, SLOT(jobStatusChanged(PrfJob*,QString)));
    connect(job, SIGNAL(progressChanged(PrfJob*,QString)), this, SLOT(jobProgressChanged(PrfJob*,QString)));
    connect(job, SIGNAL(jobInterrupted(PrfJob*)), this, SLOT(jobInterrupted(PrfJob*)));
    connect(job, SIGNAL(finished()), this, SLOT(jobThreadFinished()));
    beginInsertRows(QModelIndex(), m_Jobs.size(), m_Jobs.size());
    Entry e;
    e.job = job;
    e.status = "Queued";
    e.added = QDateTime::currentDateTime();
    m_Jobs.append(e);
    endInsertRows();
    emit addedJob(job);
}

void PrfJobListModel::removeJob(PrfJob *job) {
    disconnect(job, SIGNAL(jobDataChanged(PrfJob*)), this, SLOT(jobDataChanged(PrfJob*)));
    disconnect(job, SIGNAL(jobStarted(PrfJob*)), this, SLOT(jobStarted(PrfJob*)));
    disconnect(job, SIGNAL(jobFinished(PrfJob*)), this, SLOT(jobFinished(PrfJob*)));
    disconnect(job, SIGNAL(statusChanged(PrfJob*,QString)), this, SLOT(jobStatusChanged(PrfJob*,QString)));
    disconnect(job, SIGNAL(progressChanged(PrfJob*,QString)), this, SLOT(jobProgressChanged(PrfJob*,QString)));
    disconnect(job, SIGNAL(jobInterrupted(PrfJob*)), this, SLOT(jobInterrupted(PrfJob*)));
    int idx;
    for (idx = 0; idx < m_Jobs.size(); idx++)
        if (m_Jobs[idx].job == job) break;
    beginRemoveRows(QModelIndex(), idx, idx);
    m_Jobs.removeAt(idx);
    endRemoveRows();
}

void PrfJobListModel::requestInterruption(int row) {
    m_Jobs[row].job->requestInterruption();
    if (m_Jobs[row].job->isRunning()) {
        m_Jobs[row].status = "Cancelling...";
    } else if (m_Jobs[row].job->isFinished()) {
        // nothing
    } else {
        m_Jobs[row].status = "Cancelled";
    }
    emit dataChanged(index(row, 1, QModelIndex()), index(row, 1, QModelIndex()));
}

PrfJob* PrfJobListModel::job(int row) {
    return m_Jobs[row].job;
}

void PrfJobListModel::jobDataChanged(PrfJob *job) {
    int idx;
    for (idx = 0; idx < m_Jobs.size(); idx++)
        if (m_Jobs[idx].job == job) break;
    emit dataChanged(createIndex(idx, 0), createIndex(idx, columnCount(QModelIndex()) - 1));
}

void PrfJobListModel::jobStarted(PrfJob *job) {
    int idx;
    for (idx = 0; idx < m_Jobs.size(); idx++)
        if (m_Jobs[idx].job == job) break;
    m_Jobs[idx].started = QDateTime::currentDateTime();
    m_Jobs[idx].status = "Running...";
    emit dataChanged(createIndex(idx, 0), createIndex(idx, columnCount(QModelIndex()) - 1));
}

void PrfJobListModel::jobFinished(PrfJob *job) {
    int idx;
    for (idx = 0; idx < m_Jobs.size(); idx++)
        if (m_Jobs[idx].job == job) break;
    qDebug() << "jobFinished(), idx:" << idx << "right column:" << (columnCount(QModelIndex()) - 1);
    qDebug() << "thread:" << (qulonglong) QThread::currentThreadId();
    m_Jobs[idx].finished = QDateTime::currentDateTime();
    m_Jobs[idx].status = "Finished";
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, columnCount(QModelIndex()) - 1, QModelIndex()));
}

void PrfJobListModel::jobStatusChanged(PrfJob *job, const QString &status) {
    int idx;
    for (idx = 0; idx < m_Jobs.size(); idx++)
        if (m_Jobs[idx].job == job) break;
    m_Jobs[idx].status = status;
    emit dataChanged(index(idx, 1, QModelIndex()), index(idx, 1, QModelIndex()));
}

void PrfJobListModel::jobProgressChanged(PrfJob *job, const QString &progress) {
    int idx;
    for (idx = 0; idx < m_Jobs.size(); idx++)
        if (m_Jobs[idx].job == job) break;
    m_Jobs[idx].progress = progress;
    emit dataChanged(index(idx, 7, QModelIndex()), index(idx, 7, QModelIndex()));
}

void PrfJobListModel::jobInterrupted(PrfJob *job) {
    int idx;
    for (idx = 0; idx < m_Jobs.size(); idx++)
        if (m_Jobs[idx].job == job) break;
    m_Jobs[idx].status = "Interrupted";
    m_Jobs[idx].finished = QDateTime::currentDateTime();
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, columnCount(QModelIndex()) - 1, QModelIndex()));
}

void PrfJobListModel::jobThreadFinished() {
    PrfJob *job = dynamic_cast<PrfJob*>(sender());
    int idx;
    for (idx = 0; idx < m_Jobs.size(); idx++)
        if (m_Jobs[idx].job == job) break;
    // m_Jobs[idx].status = "Interrupted";
    m_Jobs[idx].finished = QDateTime::currentDateTime();
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, columnCount(QModelIndex()) - 1, QModelIndex()));
    emit finishedJob(job);
}
