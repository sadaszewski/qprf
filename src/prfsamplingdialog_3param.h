//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFSAMPLINGDIALOG_3PARAM_H
#define PRFSAMPLINGDIALOG_3PARAM_H

#include <QDialog>

class PrfSamplingJob_3Param;

namespace Ui {
class PrfSamplingDialog_3Param;
}

struct _mat_t;

typedef struct _mat_t mat_t;

class PrfSamplingDialog_3Param : public QDialog
{
    Q_OBJECT

public:
    explicit PrfSamplingDialog_3Param(QWidget *parent = 0);
    ~PrfSamplingDialog_3Param();

    PrfSamplingJob_3Param* createJob();

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
    Ui::PrfSamplingDialog_3Param *ui;
};

#endif // PRFSAMPLINGDIALOG_3PARAM_H
