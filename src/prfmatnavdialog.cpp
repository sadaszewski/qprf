//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "prfmatnavdialog.h"
#include "ui_prfmatnavdialog.h"
#include "matio.h"

#include <QTreeWidgetItem>
#include <QDebug>
#include <QBoxLayout>
#include <QFileInfo>

#include "qmatfilemodel/qmatfilemodel.h"

PrfMatNavDialog::PrfMatNavDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrfMatNavDialog)
{
    ui->setupUi(this);

    connect(ui->tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(onCloseTab(int)));
}

PrfMatNavDialog::~PrfMatNavDialog()
{
    delete ui;
}

static void mat_deleter (mat_t *mat) {
    qDebug() << "mat_deleter(): Closing file...";
    Mat_Close(mat);
}

void PrfMatNavDialog::openMatFile(const QString &fname) {
    mat_t *mat = Mat_Open(fname.toLatin1(), MAT_ACC_RDONLY);

    if (mat) {
        QAbstractItemModel *model = new QMatFileModel(QSharedPointer<mat_t> (mat, mat_deleter));
        model->setParent(ui->tableView);
        ui->tableView->setModel(model);
        ui->tabWidget->setTabText(0, QFileInfo(fname).fileName());

        connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onDoubleClick(QModelIndex)));

        //qDebug() << "Mat file open.";
        //matvar_t *var;
        //while ((var = Mat_VarReadNextInfo(mat)) != NULL) {
          //  QTreeWidgetItem *item = new QTreeWidgetItem();

            //ui->treeWidget->addTopLevelItem(item);
//       }
    }
}

QString PrfMatNavDialog::variableSpec() {
    QWidget *w = ui->tabWidget->currentWidget();
    QTableView *v = w->findChild<QTableView*>();
    QAbstractItemModel *model = v->model();
    QModelIndex index = v->currentIndex();
    if (index.isValid()) {
        /* QAbstractItemModel *new_model = dynamic_cast<QMatNavigable*>(model)->navigateInto(index);
        if (new_model) {
            QString tmp = dynamic_cast<QMatNavigable*>(new_model)->name();
            delete new_model;
            return tmp;
        } */
        // return model->data(index).toString();
        return dynamic_cast<QMatNavigable*>(model)->childName(index);
    }
    return dynamic_cast<QMatNavigable*>(model)->name();
}

void PrfMatNavDialog::onDoubleClick(const QModelIndex &index) {
    qDebug() << "Double clicked";

    QWidget *tab = ui->tabWidget->currentWidget();
    QTableView *v = tab->findChild<QTableView*>();
    QMatNavigable *model = dynamic_cast<QMatNavigable*>(v->model());
    QAbstractItemModel *new_model = model->navigateInto(index);

    if (new_model == NULL) {
        qDebug() << "Cannot navigate inside";
        return;
    }

    qDebug() << "New model ready";

    for (int i = 1; i < ui->tabWidget->count(); i++) {
        tab = ui->tabWidget->widget(i);
        v = tab->findChild<QTableView*>();
        model = dynamic_cast<QMatNavigable*>(v->model());
        if (model->equals(dynamic_cast<QMatNavigable*>(new_model))) {
            ui->tabWidget->setCurrentIndex(i);
            delete new_model;
            return;
        }
    }

    qDebug() << "Existing tab not found";

    QWidget *w = new QWidget();
    QBoxLayout *layout = new QBoxLayout(QBoxLayout::Down);
    w->setLayout(layout);
    v = new QTableView(w);
    layout->addWidget(v);
    new_model->setParent(v);
    v->setModel(new_model);

    connect(v, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onDoubleClick(QModelIndex)));

    qDebug() << "Tab created";

    ui->tabWidget->addTab(w, dynamic_cast<QMatNavigable*>(new_model)->name());
    ui->tabWidget->setCurrentWidget(w);


}

void PrfMatNavDialog::onCloseTab(int idx) {
    if (idx > 0) {
        QWidget *w = ui->tabWidget->widget(idx);
        ui->tabWidget->removeTab(idx);
        delete w;
    }
}
