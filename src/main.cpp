//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "prfmainwindow.h"
#include <QApplication>

#include <QDebug>
#include <QDateTime>
#include "matio.h"

#if 1

int main(int argc, char *argv[])
{
    /* qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    mat_t *f = Mat_Open("F:\\Users\\sadaszew\\Downloads\\prf\\big_reduced_3.mat", MAT_ACC_RDONLY);

    qDebug() << "File opened.";

    matvar_t *var = Mat_VarRead(f, "big_reduced");

    qDebug() << "Elapsed:" << (QDateTime::currentMSecsSinceEpoch() - timestamp) << "ms"; */


    QApplication a(argc, argv);
    PrfMainWindow w;
    w.show();

    return a.exec();
}

#endif
