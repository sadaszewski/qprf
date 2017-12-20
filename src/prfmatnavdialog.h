//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFMATNAVDIALOG_H
#define PRFMATNAVDIALOG_H

#include <QDialog>

namespace Ui {
class PrfMatNavDialog;
}


class PrfMatNavDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrfMatNavDialog(QWidget *parent = 0);
    ~PrfMatNavDialog();

    void openMatFile(const QString&);
    QString variableSpec();

private slots:
    void onDoubleClick(const QModelIndex&);
    void onCloseTab(int);

private:
    Ui::PrfMatNavDialog *ui;
};


#endif // PRFMATNAVDIALOG_H
