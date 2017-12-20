//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "prfjobsdialog.h"
#include "ui_prfjobsdialog.h"

// PrfJobsDialog *PrfJobsDialog::m_Singleton;

#include <QTimer>
#include <QDebug>

#include "prfjoblistmodel.h"

PrfJobsDialog::PrfJobsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrfJobsDialog)
{
    ui->setupUi(this);
    // m_Timer = new QTimer(this);
    //m_JobsModel->re
    //connect(m_Timer, SIGNAL(timeout()), m_JobsModel, SLOT());
    //connect(m_Timer, SIGNAL(timeout()), ui->treeView, SLOT(repaint()));
    // connect(m_Timer, SIGNAL(timeout()), this, SLOT(handleQueue()));
    // m_Timer->setInterval(1000);
    // m_Timer->start();

    connect(ui->stopSelected_pushButton, SIGNAL(clicked(bool)), this, SLOT(onStopSelected()));
    connect(ui->removeStopped_pushButton, SIGNAL(clicked(bool)), this, SLOT(onRemoveStopped()));
}

PrfJobsDialog::~PrfJobsDialog()
{
    delete ui;
}

void PrfJobsDialog::setJobsModel(QAbstractItemModel *model) {
    m_JobsModel = model;
    ui->treeView->setModel(model);

    PrfJobListModel *jobs_model = dynamic_cast<PrfJobListModel*>(model);

    connect(jobs_model, SIGNAL(addedJob(PrfJob*)), this, SLOT(handleQueue()));
    connect(jobs_model, SIGNAL(finishedJob(PrfJob*)), this, SLOT(handleQueue()));
}

/* PrfJobsDialog* PrfJobsDialog::singleton() {
    if (m_Singleton == 0) {
        m_Singleton = new PrfJobsDialog();
    }
    return m_Singleton;
} */

void PrfJobsDialog::onStopSelected() {
    QModelIndexList lst = ui->treeView->selectionModel()->selectedRows();
    PrfJobListModel *model = dynamic_cast<PrfJobListModel*>(m_JobsModel);
    for (int i = 0; i < lst.size(); i++) {
        model->requestInterruption(lst.at(i).row());
    }
}

void PrfJobsDialog::onRemoveStopped() {
    PrfJobListModel *model = dynamic_cast<PrfJobListModel*>(m_JobsModel);
    for (int i = 0; i < model->rowCount(QModelIndex()); i++) {
        PrfJob *job = model->job(i);
        if (job->isFinished()) {
            model->removeJob(job);
            delete job;
            i--;
        }
    }
}

void PrfJobsDialog::handleQueue() {
    PrfJobListModel *model = dynamic_cast<PrfJobListModel*>(m_JobsModel);
    int runningCount = 0;
    int pendingCount = 0;
    for (int i = 0; i < model->rowCount(QModelIndex()); i++) {
        PrfJob *job = model->job(i);
        if (job->isRunning())
            runningCount++;
        else if (!job->isFinished())
            pendingCount++;
    }
    if (runningCount == 0 && pendingCount > 0) {
        qDebug() << "runningCount:" << runningCount << "pendingCount:" << pendingCount;
        for (int i = 0; i < model->rowCount(QModelIndex()); i++) {
            PrfJob *job = model->job(i);
            if (!job->isRunning() && !job->isFinished()) {
                qDebug() << QString("Starting job %1").arg(i);
                job->start();
                return; // start just one job at a time
            }
        }
    }
}
