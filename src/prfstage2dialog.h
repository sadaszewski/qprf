//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFSTAGE2DIALOG_H
#define PRFSTAGE2DIALOG_H

#include <QDialog>
#include "matio.h"

class PrfStage2Job;

namespace Ui {
class PrfStage2Dialog;
}

class PrfStage2Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrfStage2Dialog(QWidget *parent = 0);
    ~PrfStage2Dialog();

    PrfStage2Job *createJob();

private slots:
    void onBrowseFile();
    void onBrowseVar();
    void onBrowseOut();

private:
    void handleVar(matvar_t*);


private:
    Ui::PrfStage2Dialog *ui;
};

#endif // PRFSTAGE2DIALOG_H
