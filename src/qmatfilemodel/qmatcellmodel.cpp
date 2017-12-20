//
// Copyright (C) Stanislaw Adaszewski, 2016
// All Rights Reserved.
//

#include "qmatcellmodel.h"
#include "qmatfilemodel.h"

#include <QDebug>

static void var_deleter(matvar_t *var) {
    qDebug() << "var_deleter(): NOT deleting variable:" << var->name;
    // Mat_VarFree(var);
}

struct VarDeleter {
    VarDeleter(QSharedPointer<matvar_t> dep): m_Dep(dep) {}

    void operator()(matvar_t *var) {
        qDebug() << "VarDeleter(): NOT deleting variable:" << var->name;
    }

    QSharedPointer<matvar_t> m_Dep;
};

QMatCellModel::QMatCellModel(QSharedPointer<mat_t> mat, QSharedPointer<matvar_t> var) {
    m_Mat = mat;
    m_Var = var;
}

QMatCellModel::QMatCellModel(QSharedPointer<mat_t> mat, QSharedPointer<matvar_t> var,
    const QVector<size_t> &indices):
    QMatCellModel(mat, var) {
    m_Indices = indices;
}

QMatCellModel::~QMatCellModel() {
    qDebug() << "~QMatCellModel():" << name();
}


int QMatCellModel::rowCount(const QModelIndex &parent) const {
    int sz = m_Indices.size();
    return sz < m_Var->rank ? m_Var->dims[sz] : 0;
}

int QMatCellModel::columnCount(const QModelIndex &parent) const {
    int sz = m_Indices.size();
    return sz + 1 < m_Var->rank ? m_Var->dims[sz + 1] : 1;
}

QModelIndex QMatCellModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex QMatCellModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

QVariant QMatCellModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    int sz = indices.size();
    if (sz >= m_Var->rank) {
        int ofs = QMatFileModel::linear_index(m_Var.data(), indices);
        matvar_t *var = Mat_VarGetCell(m_Var.data(), ofs);
        QString tmp = QMatFileModel::makeTypeDesc(var);
        //Mat_VarFree(var);
        return tmp;
    } else {
        return name(indices);
    }
}


QAbstractItemModel* QMatCellModel::navigateInto(const QModelIndex &index) {
    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    int sz = indices.size();
    if (sz >= m_Var->rank) {
        int ofs = QMatFileModel::linear_index(m_Var.data(), indices);
        matvar_t *var = Mat_VarGetCell(m_Var.data(), ofs);
        QAbstractItemModel *model = QMatFileModel::modelForVar(m_Mat, QSharedPointer<matvar_t>(var, VarDeleter(m_Var)));
        dynamic_cast<QMatNavigable*>(model)->setParentName(name(indices));
        return model;
    } else {
        QAbstractItemModel *model = new QMatCellModel(m_Mat, m_Var, indices);
        dynamic_cast<QMatNavigable*>(model)->setParentName(name(indices));
        return model;
    }
}

bool QMatCellModel::equals(QMatNavigable *other) {
    return (name() == other->name());
}

QString QMatCellModel::name(const QVector<size_t> &indices) const {
    QString tmp = m_ParentName;
    if (tmp.endsWith("}")) {
        int idx = tmp.lastIndexOf("{");
        tmp = tmp.left(idx);
    }
    int sz = indices.size();
    if (sz > 0) {
        tmp += "{";
        for (int i = 0; i < m_Var->rank; i++) {
            if (i > 0) tmp += ",";
            if (i < sz)
                tmp += QString("%1").arg(indices[i]);
            else
                tmp += ":";
        }
        tmp += "}";
    }
    return tmp;
}

QString QMatCellModel::name() {
    return name(m_Indices);
}

void QMatCellModel::setParentName(const QString &parentName) {
    m_ParentName = parentName;
}

QString QMatCellModel::childName(const QModelIndex &index) {
    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    return name(indices);
}
