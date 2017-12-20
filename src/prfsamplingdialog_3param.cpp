//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "prfsamplingdialog_3param.h"
#include "ui_prfsamplingdialog_3param.h"

#include "prfsamplingjob_3param.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

#include "prfmatnavdialog.h"
#include "qmatfilemodel/qmatfilemodel.h"
#include "prftypecastarray.h"

#include <math.h>


static void mat_deleter(mat_t *mat) {
    qDebug() << "Closing MAT File...";
    Mat_Close(mat);
}

static void var_deleter(matvar_t *var) {
    qDebug() << "Deleting variable:" << var->name;
    Mat_VarFree(var);
}

PrfSamplingDialog_3Param::PrfSamplingDialog_3Param(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrfSamplingDialog_3Param)
{
    ui->setupUi(this);

    connect(ui->dataFileBrowse_pushButton, SIGNAL(clicked(bool)), this, SLOT(onDataFileBrowse()));
    connect(ui->dataVarSpecBrowse_pushButton, SIGNAL(clicked(bool)), this, SLOT(onDataVarSpecBrowse()));
    connect(ui->stage2FileBrowse_pushButton, SIGNAL(clicked(bool)), this, SLOT(onStage2FileBrowse()));
    connect(ui->pRF_model_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(pRF_model_changed(int)));
}

PrfSamplingDialog_3Param::~PrfSamplingDialog_3Param()
{
    delete ui;
}

PrfSamplingJob_3Param* PrfSamplingDialog_3Param::createJob() {
    PrfSamplingJob_3Param *job = new PrfSamplingJob_3Param();

    job->set_pRF_Model(ui->pRF_model_comboBox->currentIndex());
    job->setWriteDebug(ui->writeDebug_checkBox->isChecked());
    job->setDataFileName(ui->dataFileName_lineEdit->text());
    job->setDataVariableSpec(ui->dataVarSpec_lineEdit->text());
    job->setStage2FileName(ui->stage2FileName_lineEdit->text());
    job->setNumberOfSteps(ui->numOfSteps_lineEdit->text().toInt());
    job->setTemperature(ui->temperature_lineEdit->text().toFloat());

    job->setXStdev(ui->xStep_lineEdit->text().toFloat());
    job->setYStdev(ui->yStep_lineEdit->text().toFloat());
    job->setRFSizeStdev(ui->rfsizeStep_lineEdit->text().toFloat());
    job->setExptStdev(ui->exptStep_lineEdit->text().toFloat());
    job->setGainStdev(ui->gainStep_lineEdit->text().toFloat());

    job->setKappaStdev(ui->kappaStep_lineEdit->text().toFloat());
    job->setTauStdev(ui->tauStep_lineEdit->text().toFloat());
    job->setEpsilonStdev(ui->epsilonStep_lineEdit->text().toFloat());

    job->setSubdiv(ui->subdiv_lineEdit->text().toInt());
    job->setTE(ui->TE_lineEdit->text().toFloat());
    job->setTR(ui->TR_lineEdit->text().toFloat());

    job->setMaxNoiseStdev(ui->maxNoiseStdev_lineEdit->text().toFloat());

    return job;
}

void PrfSamplingDialog_3Param::onDataFileBrowse() {
    QString fname = QFileDialog::getOpenFileName(this, "Select Data MAT-File...", QString(), "MAT-Files (*.mat)");
    if (!fname.isEmpty()) {
        ui->dataFileName_lineEdit->setText(fname);
        if (ui->dataVarSpec_lineEdit->text().isEmpty())
            ui->dataVarSpec_lineEdit->setText("data{0,0}");
        verifyDataVariable();
    }
}

void PrfSamplingDialog_3Param::onDataVarSpecBrowse() {

    if (QFileInfo(ui->dataFileName_lineEdit->text()).exists()) {
        PrfMatNavDialog dlg(this);
        dlg.openMatFile(ui->dataFileName_lineEdit->text());
        if (dlg.exec() == QDialog::Accepted) {
            ui->dataVarSpec_lineEdit->setText(dlg.variableSpec());
            verifyDataVariable();
        }
    } else {
        QMessageBox::warning(this, "Error", "Please select an existing data file first.");
    }

}

void PrfSamplingDialog_3Param::onStage2FileBrowse() {
    QString fname = QFileDialog::getOpenFileName(this, "Select Stage 2 MAT-File...", QString(), "MAT-Files (*.mat)");
    if (!fname.isEmpty()) {
        ui->stage2FileName_lineEdit->setText(fname);
        verifyStage2File();
    }
}

void PrfSamplingDialog_3Param::pRF_model_changed(int idx) {
    if (idx == 0) { // CSS-pRF
        // ui->gainStep_lineEdit->setEnabled(true);
        ui->exptStep_lineEdit->setEnabled(true);
    } else { //  pRF
        // ui->gainStep_lineEdit->setEnabled(false);
        ui->exptStep_lineEdit->setEnabled(false);
    }
}

void PrfSamplingDialog_3Param::verifyDataVariable() {

    QSharedPointer<mat_t> mat(Mat_Open(ui->dataFileName_lineEdit->text().toLatin1(), MAT_ACC_RDONLY), mat_deleter);
    if (mat.isNull()) {
        QMessageBox::warning(this, "Error", "Failed to open specified file.");
        return;
    }

    matvar_t *topvar = 0;
    matvar_t *var = QMatFileModel::resolveVariableSpec(mat.data(), ui->dataVarSpec_lineEdit->text(), &topvar);
    QSharedPointer<matvar_t> _topvar(topvar, var_deleter);
    Q_UNUSED(_topvar);
    //if (var.data() != topvar)
        //Mat_VarFree(topvar);

    if (var == NULL) {
        QMessageBox::warning(this, "Warning",
            QString("Variable %1 is incorrect, you need to specify an existing numeric variable of size [N, TOT_DUR].")
            .arg(ui->dataVarSpec_lineEdit->text()));
        ui->dataVarSpec_lineEdit->setText("");
        return;
    }

    if (var->rank != 2) {
        QMessageBox::warning(this, "Error",
            QString("Specified variable is not of the expected rank. Variable rank: %1. Expected: %2.")
            .arg(var->rank).arg(2));
        ui->dataVarSpec_lineEdit->setText("");
        return;
    }


}

void PrfSamplingDialog_3Param::verifyStage2File() {
    QSharedPointer<mat_t> mat(Mat_Open(ui->stage2FileName_lineEdit->text().toLatin1(), MAT_ACC_RDONLY), mat_deleter);
    if (mat.isNull()) {
        QMessageBox::warning(this, "Error", "Failed to open specified file.");
        return;
    }

    QSharedPointer<matvar_t> mat_reduced(Mat_VarRead(mat.data(), "reduced"), var_deleter);
    if (mat_reduced.isNull()) {
        QMessageBox::warning(this, "Error", "Variable reduced not found in stage 2 file.");
        ui->stage2FileName_lineEdit->setText("");
        return;
    }
    if (mat_reduced->rank != 2) {
        QMessageBox::warning(this, "Error", "Variable reduced has wrong rank.");
        ui->stage2FileName_lineEdit->setText("");
        return;
    }

    int x_range_len = verifyRangeVar(mat.data(), "x_range");
    if (x_range_len == -1)
        return;

    int y_range_len = verifyRangeVar(mat.data(), "y_range");
    if (y_range_len == -1)
        return;

    int rfsize_range_len = verifyRangeVar(mat.data(), "rfsize_range");
    if (rfsize_range_len == -1)
        return;

    int nelem = x_range_len * y_range_len * rfsize_range_len;
    qDebug() << "nelem:" << nelem;

    if (mat_reduced->dims[1] != nelem) {
        QMessageBox::warning(this, "Error", "Specified ranges are inconsistent with reduced variable dimensions.");
        ui->stage2FileName_lineEdit->setText("");
        return;
    }
}

int PrfSamplingDialog_3Param::verifyRangeVar(mat_t *mat, const char *varname) {
    QSharedPointer<matvar_t> var(Mat_VarRead(mat, varname), var_deleter);
    if (var.isNull()) {
        QMessageBox::warning(this, "Error", QString("Variable %1 not found in stage 2 file.").arg(varname));
        ui->stage2FileName_lineEdit->setText("");
        return -1;
    }
    if (var->rank != 2) {
        QMessageBox::warning(this, "Error", QString("Variable %1 has wrong rank.").arg(varname));
        ui->stage2FileName_lineEdit->setText("");
        return -1;
    }
    if (var->dims[0] != 1 || var->dims[1] != 3) {
        QMessageBox::warning(this, "Error", QString("Variable %1 has wrong dimension.").arg(varname));
        ui->stage2FileName_lineEdit->setText("");
        return -1;
    }
    QSharedPointer<Prf::TypeCastArrayBase> range(Prf::TypeCastArrayFactory::create(var->class_type, var->data));
    //int range_len = (int) ceilf((range->asFloat(1) - range->asFloat(0)) / range->asFloat(2));
    int start = range->asInt32(0);
    int end = range->asInt32(1);
    int step = range->asInt32(2);
    qDebug() << varname << "start:" << start << "end:" << end << "step:" << step;
    int range_len = 0;
    for (int i = start; i < end; i += step) range_len++;
    qDebug() << varname << "range length:" << range_len;
    return range_len;
}
