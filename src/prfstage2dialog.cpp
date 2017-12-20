//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "prfstage2dialog.h"
#include "ui_prfstage2dialog.h"

#include <QFileDialog>
#include "qmatfilemodel/qmatfilemodel.h"
#include <QDebug>
#include <QMessageBox>

#include "prfmatnavdialog.h"
#include "prfstage2job.h"

PrfStage2Dialog::PrfStage2Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrfStage2Dialog)
{
    ui->setupUi(this);

    connect(ui->browseFile_pushButton, SIGNAL(clicked(bool)), this, SLOT(onBrowseFile()));
    connect(ui->browseVar_pushButton, SIGNAL(clicked(bool)), this, SLOT(onBrowseVar()));
    connect(ui->browseOut_pushButton, SIGNAL(clicked(bool)), this, SLOT(onBrowseOut()));
}

PrfStage2Dialog::~PrfStage2Dialog()
{
    delete ui;
}

PrfStage2Job* PrfStage2Dialog::createJob() {
    PrfStage2Job *job = new PrfStage2Job();

    job->setInputFileName(ui->fileName_lineEdit->text());
    job->setVariableSpec(ui->varname_lineEdit->text());
    job->setOutputFileName(ui->outName_lineEdit->text());

    job->setXStart(ui->xStart_lineEdit->text().toInt());
    job->setXEnd(ui->xEnd_lineEdit->text().toInt());
    job->setXStep(ui->xStep_lineEdit->text().toInt());

    job->setYStart(ui->yStart_lineEdit->text().toInt());
    job->setYEnd(ui->yEnd_lineEdit->text().toInt());
    job->setYStep(ui->yStep_lineEdit->text().toInt());

    job->setRFSizeStart(ui->rfsizeStart_lineEdit->text().toInt());
    job->setRFSizeEnd(ui->rfsizeEnd_lineEdit->text().toInt());
    job->setRFSizeStep(ui->rfsizeStep_lineEdit->text().toInt());

    return job;
}

void PrfStage2Dialog::onBrowseFile() {
    QString fname = QFileDialog::getOpenFileName(this, "Select MAT-File containing stimulus...", QString(),
        "MAT-Files (*.mat)");
    if (!fname.isEmpty()) {
        ui->fileName_lineEdit->setText(fname);

        mat_t *mat = Mat_Open(fname.toLatin1(), MAT_ACC_RDONLY);
        if (mat == NULL) {
            qDebug() << "Failed to open file o_O";
            return;
        }
        matvar_t *topvar;
        matvar_t *var;
        var = QMatFileModel::resolveVariableSpec(mat, ui->varname_lineEdit->text(), &topvar);
        if (var == NULL) {
            qDebug() << "Default variable not found in file, need to specify";
            ui->varname_lineEdit->setText("");
        } else {
            handleVar(var);
        }
        if (topvar != NULL) {
            Mat_VarFree(topvar);
        }
        Mat_Close(mat);
    }
}

void PrfStage2Dialog::onBrowseVar() {

    if (!ui->fileName_lineEdit->text().isEmpty()) {
        PrfMatNavDialog dlg(this);
        dlg.openMatFile(ui->fileName_lineEdit->text());
        if (dlg.exec() == QDialog::Accepted) {
            // ui->varname_lineEdit->setText(dlg.variableSpec());
            mat_t *mat;
            mat = Mat_Open(ui->fileName_lineEdit->text().toLatin1(), MAT_ACC_RDONLY);
            if (mat == NULL) {
                qDebug() << "Cannot open file O_o";
                ui->varname_lineEdit->setText("");
                return;
            }
            matvar_t *topvar;
            matvar_t *var;
            var = QMatFileModel::resolveVariableSpec(mat, dlg.variableSpec(), &topvar);
            if (var == NULL) {
                QMessageBox::warning(this, "Error", "Incorrect variable specified. You need to specify a HxWxTOT_DUR numeric array.");
                ui->varname_lineEdit->setText("");
            } else {
                ui->varname_lineEdit->setText(dlg.variableSpec());
                handleVar(var);
            }
            if (topvar != NULL) {
                Mat_VarFree(topvar);
            }
        }
    }

}

void PrfStage2Dialog::onBrowseOut() {
    QString outname = QFileDialog::getSaveFileName(this, "Please specify output stage 2 file...", QString(), "MAT-Files (*.mat)");
    if (!outname.isNull())
        ui->outName_lineEdit->setText(outname);
}

void PrfStage2Dialog::handleVar(matvar_t *var) {
    if (var->class_type != MAT_C_DOUBLE &&
        var->class_type != MAT_C_SINGLE &&
        var->class_type != MAT_C_INT8 &&
        var->class_type != MAT_C_UINT8 &&
        var->class_type != MAT_C_INT16 &&
        var->class_type != MAT_C_UINT16 &&
        var->class_type != MAT_C_INT32 &&
        var->class_type != MAT_C_UINT32 &&
        var->class_type != MAT_C_INT64 &&
        var->class_type != MAT_C_UINT64) {

        QMessageBox::warning(this, "Error", "Specified variable is not of the expected numeric type.");
        ui->varname_lineEdit->setText("");
        return;
    }

    if (var->rank != 3) {
        QMessageBox::warning(this, "Error", QString("Specified variable is not of the expected rank. Variable rank: %1. Expected: %2.").arg(var->rank).arg(3));
        ui->varname_lineEdit->setText("");
        return;
    }

    ui->xStart_lineEdit->setText("0");
    ui->xEnd_lineEdit->setText(QString("%1").arg(var->dims[1] + 1));
    ui->xStep_lineEdit->setText("4");

    ui->yStart_lineEdit->setText("0");
    ui->yEnd_lineEdit->setText(QString("%1").arg(var->dims[0] + 1));
    ui->yStep_lineEdit->setText("4");

    ui->rfsizeStart_lineEdit->setText("0");
    ui->rfsizeEnd_lineEdit->setText(QString("%1").arg(qMax(var->dims[0], var->dims[1]) + 1));
    ui->rfsizeStep_lineEdit->setText("4");

    qDebug() << "H:" << var->dims[0] << "W:" << var->dims[1] << "TOT_DUR:" << var->dims[2];

}
