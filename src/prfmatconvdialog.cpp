//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "prfmatconvdialog.h"
#include "ui_prfmatconvdialog.h"

#include <QAction>
#include <QFileDialog>
#include <QDebug>
#include <QFileInfo>

#include <matio.h>

PrfMatConvDialog::PrfMatConvDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrfMatConvDialog)
{
    ui->setupUi(this);

    QAction *action_inputFile = ui->lineEdit_inputFile->addAction(qApp->style()->standardIcon(QStyle::SP_DirIcon), QLineEdit::TrailingPosition);
    QAction *action_outputFile = ui->lineEdit_outputFile->addAction(qApp->style()->standardIcon(QStyle::SP_DirIcon), QLineEdit::TrailingPosition);

    connect(action_inputFile, SIGNAL(triggered(bool)), this, SLOT(on_action_inputFile_triggered(bool)));
    connect(action_outputFile, SIGNAL(triggered(bool)), this, SLOT(on_action_outputFile_triggered(bool)));

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(on_buttonBox_accepted()));
}

void PrfMatConvDialog::on_action_inputFile_triggered(bool) {
    QString fname = QFileDialog::getOpenFileName(this, "Select input file", QString(), "MAT Files (*.mat)");
    if (!fname.isEmpty()) ui->lineEdit_inputFile->setText(fname);
}

void PrfMatConvDialog::on_action_outputFile_triggered(bool) {
    QString fname = QFileDialog::getSaveFileName(this, "Select output file", QString(), "MAT Files (*.mat)");
    if (!fname.isEmpty()) ui->lineEdit_outputFile->setText(fname);
}

void PrfMatConvDialog::on_buttonBox_accepted() {
    int fmt_idx = ui->comboBox_outputFormat->currentIndex();
    mat_ft formats[] = {MAT_FT_MAT4, MAT_FT_MAT5, MAT_FT_MAT73};
    mat_ft fmt = formats[fmt_idx];

    qDebug() << "fmt:" << fmt;

    // mat_t *f_out = Mat_CreateVer(ui->lineEdit_outputFile->text().toLatin1(), NULL, fmt);
    // qDebug() << "f_out:" << f_out;
    mat_t *f_in = Mat_Open(ui->lineEdit_inputFile->text().toLatin1(), MAT_ACC_RDONLY);
    qDebug() << "f_in:" << f_in;

    while (1) {
        matvar_t *var_in = Mat_VarReadNext(f_in);
        if (var_in == NULL) {
            qDebug() << "No more variables to read.";
            break;
        }

        qDebug() << "Success reading variable:" << var_in->name;
        qDebug() << "var_in->data:" << var_in->data;
        qDebug() << "var_in->rank:" << var_in->rank;
        qDebug() << "var_in->dims:" << var_in->dims[0] << var_in->dims[1];

        QFileInfo fi(ui->lineEdit_outputFile->text());

        mat_t *f_out = Mat_CreateVer(fi.absoluteDir().absoluteFilePath(QString("%1.mat").arg(var_in->name)).toLatin1(), NULL, fmt);

        // matvar_t *var_out = Mat_VarCreate(var_in->name, var_in->class_type, var_in->data_type, var_in->rank, var_in->dims, var_in->data, MAT_F_DONT_COPY_DATA);
        // qDebug() << "var_out:" << var_out;

        if (Mat_VarWrite(f_out, var_in, MAT_COMPRESSION_NONE) != 0) {
            qDebug() << "Failed to write variable:" << var_in->name;
        } else {
            qDebug() << "Success writing variable:" << var_in->name;
        }

        Mat_VarFree(var_in);

        Mat_Close(f_out);
    }

    // Mat_Close(f_out);
    Mat_Close(f_in);

    qDebug() << "Done.";
}

PrfMatConvDialog::~PrfMatConvDialog()
{
    delete ui;
}
