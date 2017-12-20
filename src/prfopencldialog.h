//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFOPENCLDIALOG_H
#define PRFOPENCLDIALOG_H

#include <QDialog>

namespace Ui {
class PrfOpenCLDialog;
}

class PrfOpenCLDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrfOpenCLDialog(QWidget *parent = 0);
    ~PrfOpenCLDialog();

protected slots:
    void onSaveOpenCLSettings();

private:
    Ui::PrfOpenCLDialog *ui;
};

#endif // PRFOPENCLDIALOG_H
