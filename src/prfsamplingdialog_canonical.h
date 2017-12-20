//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFSAMPLINGDIALOGCANONICAL_H
#define PRFSAMPLINGDIALOGCANONICAL_H

#include <QDialog>

class PrfSamplingJobCanonical;

namespace Ui {
class PrfSamplingDialogCanonical;
}

struct _mat_t;

typedef struct _mat_t mat_t;

class PrfSamplingDialogCanonical : public QDialog
{
    Q_OBJECT

public:
    explicit PrfSamplingDialogCanonical(QWidget *parent = 0);
    ~PrfSamplingDialogCanonical();

    PrfSamplingJobCanonical* createJob();

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
    Ui::PrfSamplingDialogCanonical *ui;
};

#endif // PRFSAMPLINGDIALOGCANONICAL_H
