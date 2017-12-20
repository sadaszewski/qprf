//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFJOBSDIALOG_H
#define PRFJOBSDIALOG_H

#include <QDialog>

namespace Ui {
class PrfJobsDialog;
}

class QAbstractItemModel;

class PrfJobsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrfJobsDialog(QWidget *parent = 0);
    ~PrfJobsDialog();

    void setJobsModel(QAbstractItemModel*);

private slots:
    void onStopSelected();
    void onRemoveStopped();
    void handleQueue();

private:
    Ui::PrfJobsDialog *ui;
    QAbstractItemModel *m_JobsModel;
    QTimer *m_Timer;
};

#endif // PRFJOBSDIALOG_H
