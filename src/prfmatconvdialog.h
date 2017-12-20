//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFMATCONVDIALOG_H
#define PRFMATCONVDIALOG_H

#include <QDialog>

namespace Ui {
class PrfMatConvDialog;
}

class PrfMatConvDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrfMatConvDialog(QWidget *parent = 0);
    ~PrfMatConvDialog();

private slots:
    void on_action_inputFile_triggered(bool);
    void on_action_outputFile_triggered(bool);
    void on_buttonBox_accepted();

private:
    Ui::PrfMatConvDialog *ui;
};

#endif // PRFMATCONVDIALOG_H
