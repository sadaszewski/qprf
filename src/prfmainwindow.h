//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFMAINWINDOW_H
#define PRFMAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class PrfMainWindow;
}

class PrfJobsDialog;
class PrfJobListModel;

class PrfMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PrfMainWindow(QWidget *parent = 0);
    ~PrfMainWindow();

protected slots:
    void createStage2Task();
    void createSamplingTask();
    void onOpenCLConfig();
    void onAbout();
    void killSplash();
    void on_action_MAT_File_Converted_triggered(bool);

private:
    Ui::PrfMainWindow *ui;
    PrfJobsDialog *m_JobsDialog;
    PrfJobListModel *m_JobsModel;
};

#endif // PRFMAINWINDOW_H
