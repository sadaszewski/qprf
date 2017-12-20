//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "prfmainwindow.h"
#include "ui_prfmainwindow.h"

#include "prfmatnavdialog.h"
#include "prfstage2dialog.h"
#include "prfopencldialog.h"
#include "prfstage2job.h"
#include "prfjobsdialog.h"
#include "prfjoblistmodel.h"
#include "prfmatconvdialog.h"

#include "prfsamplingdialog.h"
#include "prfsamplingdialog_canonical.h"
#include "prfsamplingdialog_3param.h"
#include "prfsamplingjob.h"
#include "prfsamplingjob_canonical.h"
#include "prfsamplingjob_3param.h"

#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QInputDialog>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

PrfMainWindow::PrfMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PrfMainWindow)
{
    ui->setupUi(this);
    ui->statusBar->addWidget(new QLabel("Ready."));

    connect(ui->commandLinkButton, SIGNAL(clicked(bool)), this, SLOT(createStage2Task()));
    connect(ui->commandLinkButton_2, SIGNAL(clicked(bool)), this, SLOT(createSamplingTask()));
    ui->commandLinkButton_3->hide();

    connect(ui->actionOpen_CL_Config, SIGNAL(triggered(bool)), this, SLOT(onOpenCLConfig()));
    connect(ui->action_Quit, SIGNAL(triggered(bool)), qApp, SLOT(quit()));
    connect(ui->action_About, SIGNAL(triggered(bool)), this, SLOT(onAbout()));
    connect(ui->action_MAT_File_Converter, SIGNAL(triggered(bool)), this, SLOT(on_action_MAT_File_Converted_triggered(bool)));

    m_JobsModel = new PrfJobListModel();
    m_JobsModel->setParent(this);

    m_JobsDialog = new PrfJobsDialog(this);
    m_JobsDialog->setJobsModel(m_JobsModel);

    QTimer::singleShot(0, this, SLOT(killSplash()));
}

void PrfMainWindow::killSplash() {
#ifdef Q_OS_WIN
    if (qApp->arguments().size() > 1) {
        bool ok;
        HANDLE h_splash = (HANDLE) qApp->arguments()[1].toLongLong(&ok);
        if (ok) {
            qDebug() << "Trying to terminate splash screen process:" << h_splash;
            if (TerminateProcess(h_splash, 0)) {
                qDebug() << "Success";
            } else {
                qDebug() << "Failed. Nothing to worry about.";
            }
        }
    }
#endif
}

void PrfMainWindow::on_action_MAT_File_Converted_triggered(bool) {
    PrfMatConvDialog dlg(this);
    dlg.exec();
}

PrfMainWindow::~PrfMainWindow()
{
    delete ui;
}

void PrfMainWindow::createStage2Task() {

    PrfStage2Dialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        PrfStage2Job *job = dlg.createJob();
        job->moveToThread(job);
        qDebug() << "main thread:" << (qulonglong) QThread::currentThreadId();
        qDebug() << job->objectName() << job->parameters() << job->status();
        m_JobsModel->addJob(job);
        m_JobsDialog->show();
        // job->start();
    }


    /* QString fname = QFileDialog::getOpenFileName(this, "Select MAT-File...", QString(), "MAT-File (*.mat)");

    if (!fname.isEmpty()) {
        PrfMatNavDialog dlg;
        dlg.openMatFile(fname);
        dlg.exec();
    }; */
}

void PrfMainWindow::createSamplingTask() {
    bool ok;
    QStringList items = QStringList()
            << "5-param Balloon"
            << "3-param Balloon"
            << "Canonical HRF";
    int model = items.indexOf(QInputDialog::getItem(this, "Hemodynamic Model", "Hemodynamic Model", items, 0, false, &ok));
    if (!ok)
        return;
    PrfJob *job = NULL;
    switch (model) {
    case 0: {
        PrfSamplingDialog dlg(this);
        if (dlg.exec() != QDialog::Accepted)
            return;
        job = dlg.createJob();
        break;
    }
    case 1: {
        PrfSamplingDialog_3Param dlg(this);
        if (dlg.exec() != QDialog::Accepted)
            return;
        // PrfSamplingJob_3Param *job;
        job = dlg.createJob();
        break;
    }
    case 2: {
        PrfSamplingDialogCanonical dlg(this);
        if (dlg.exec() != QDialog::Accepted)
            return;
        job = dlg.createJob();
        break;
    }
    }
    job->moveToThread(job);
    m_JobsModel->addJob(job);
    m_JobsDialog->show();
}

void PrfMainWindow::onOpenCLConfig() {
    PrfOpenCLDialog dlg(this);
    dlg.exec();
}

void PrfMainWindow::onAbout() {
    QMessageBox::about(this, "About QPrf", "<h1>QPrf</h1><p>Copyright (C) Stanisław Adaszewski, 2016</p><p><a href=\"http://algoholic.eu\">http://algoholic.eu</a>");
}
