//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef QMATFILEITEMMODEL_H
#define QMATFILEITEMMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QVector>
#include <QSharedPointer>

#include "matio.h"

struct MatIndex {
    MatIndex *parent;
    matvar_t *var;
    QString field_name;
    int rank;
    QVector<size_t> indices;

    MatIndex() {
        rank = 0;
    }

    ~MatIndex() {
        Mat_VarFree(var);
    }
};

class QMatFileItemModel : public QAbstractItemModel
{
public:
    QMatFileItemModel(mat_t*);

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    QVariant data(const QModelIndex &index, int role) const;

private:
    size_t linear_index(MatIndex *user_data) const;

private:
    mat_t *m_Mat;
    mutable QVector<QSharedPointer<MatIndex> > indexes;
    QSharedPointer<MatIndex> root;
};

#endif // QMATFILEITEMMODEL_H
