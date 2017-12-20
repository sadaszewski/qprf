//
// Copyright (C) Stanislaw Adaszewski, 2016
// All Rights Reserved.
//

#include "qmatfieldmodel.h"
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

QMatFieldModel::QMatFieldModel(QSharedPointer<mat_t> mat,
    QSharedPointer<matvar_t> var, int fieldIndex,
    const QString &fieldName,
    const QVector<size_t> &indices)
{
    m_Mat = mat;
    m_Var = var;
    m_FieldIndex = fieldIndex;
    m_FieldName = fieldName;
    m_Indices = indices;
}

QMatFieldModel::QMatFieldModel(QSharedPointer<mat_t> mat, QSharedPointer<matvar_t> var,
    int fieldIndex, const QString &fieldName)
{
    m_Mat = mat;
    m_Var = var;
    m_FieldIndex = fieldIndex;
    m_FieldName = fieldName;
}

QMatFieldModel::~QMatFieldModel() {
    qDebug() << "~QMatFieldModel():" << name();
}

int QMatFieldModel::rowCount(const QModelIndex &parent) const {
    int sz = m_Indices.size();
    return sz < m_Var->rank ? m_Var->dims[sz] : 0;
}

int QMatFieldModel::columnCount(const QModelIndex &parent) const {
    int sz = m_Indices.size();
    return sz + 1 < m_Var->rank ? m_Var->dims[sz + 1] : 1;
}

QModelIndex QMatFieldModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex QMatFieldModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

QVariant QMatFieldModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    int sz = indices.size();
    if (sz >= m_Var->rank) {
        int ofs = QMatFileModel::linear_index(m_Var.data(), indices);
        matvar_t *var = Mat_VarGetStructFieldByIndex(m_Var.data(), m_FieldIndex, ofs);
        QString tmp = QMatFileModel::makeTypeDesc(var);
        // Mat_VarFree(var);
        return tmp;
    } else {
        /* QString tmp = m_Var->name;
        tmp += "(";
        for (int i = 0; i < m_Var->rank; i++) {
            if (i>0) tmp += ",";
            if (i < sz)
                tmp += QString("%1").arg(indices[i]);
            else
                tmp += ":";
        }
        tmp += ")";
        return tmp; */
        return name(indices);
    }
}

QAbstractItemModel* QMatFieldModel::navigateInto(const QModelIndex &index) {

    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    int sz = indices.size();
    if (sz >= m_Var->rank) {
        int ofs = QMatFileModel::linear_index(m_Var.data(), indices);
        matvar_t *var = Mat_VarGetStructFieldByIndex(m_Var.data(), m_FieldIndex, ofs);
        qDebug() << "m_FieldIndex:" << m_FieldIndex << "ofs:" << ofs;
        QAbstractItemModel *model = QMatFileModel::modelForVar(m_Mat, QSharedPointer<matvar_t>(var, VarDeleter(m_Var)));
        dynamic_cast<QMatNavigable*>(model)->setParentName(name(indices));
        return model;
    } else {
        QAbstractItemModel *model = new QMatFieldModel(m_Mat, m_Var, m_FieldIndex, m_FieldName, indices);
        dynamic_cast<QMatNavigable*>(model)->setParentName(name(indices));
        return model;
    }
}


bool QMatFieldModel::equals(QMatNavigable *other) {
    return (name() == other->name());
}

QString QMatFieldModel::name(const QVector<size_t> &indices) const {
    QString tmp = m_ParentName;
    // int idx = tmp.lastIndexOf(".");

    if (tmp.endsWith(")")) {
        int idx = tmp.lastIndexOf("(");
        tmp = tmp.left(idx);
    }
    int sz = indices.size();
    if (sz > 0) {
        tmp += "(";
        for (int i = 0; i < m_Var->rank; i++) {
            if (i > 0) tmp += ",";
            if (i < sz)
                tmp += QString("%1").arg(indices[i]);
            else
                tmp += ":";
        }
        tmp += ")";
    }

    tmp += "." + m_FieldName;
    // tmp += "]";
    return tmp;
}

QString QMatFieldModel::name() {
   return name(m_Indices);
}

void QMatFieldModel::setParentName(const QString &parentName) {
    m_ParentName = parentName;
}

QString QMatFieldModel::childName(const QModelIndex &index) {
    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    return name(indices);
}
