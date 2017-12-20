//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFSAMPLINGDIALOG_H
#define PRFSAMPLINGDIALOG_H

#include <QDialog>

class PrfSamplingJob;

namespace Ui {
class PrfSamplingDialog;
}

struct _mat_t;

typedef struct _mat_t mat_t;

class PrfSamplingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrfSamplingDialog(QWidget *parent = 0);
    ~PrfSamplingDialog();

    PrfSamplingJob* createJob();

private slots:
    void onDataFileBrowse();
    void onDataVarSpecBrowse();
    void onStage2FileBrowse();
    void pRF_model_changed(int);

private:
    void verifyDataVariable();
    void verifyStage2File();
    int verifyRangeVar(mat_t*, const char*);

private:
    Ui::PrfSamplingDialog *ui;
};

#endif // PRFSAMPLINGDIALOG_H
