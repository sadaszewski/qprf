//
// Copyright (C) Stanislaw Adaszewski, 2016
// All Rights Reserved.
//

#include "qmatfilemodel.h"
#include "qmatcellmodel.h"
#include "qmatstructmodel.h"
#include "qmatarraymodel.h"
#include "qmatcharmodel.h"

#include <QDebug>

QMatCharModel::QMatCharModel(QSharedPointer<mat_t> mat, QSharedPointer<matvar_t> var) {
    m_Mat = mat;
    m_Var = var;

}

QMatCharModel::QMatCharModel(QSharedPointer<mat_t> mat, QSharedPointer<matvar_t> var,
    const QVector<size_t> &indices):
    QMatCharModel(mat, var) {

    m_Indices = indices;
}

QMatCharModel::~QMatCharModel() {
    qDebug() << "~QMatCharModel():" << name();
}

int QMatCharModel::rowCount(const QModelIndex &parent) const {
    int sz = m_Indices.size();
    return sz < m_Var->rank ? m_Var->dims[sz] : 0;
}

int QMatCharModel::columnCount(const QModelIndex &parent) const {
    return 1;
}

QModelIndex QMatCharModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex QMatCharModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

QVariant QMatCharModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    QVector<size_t> indices;
    indices.append(index.row());
    indices.append(index.column());

    int sz = indices.size();
    if (sz >= m_Var->rank) {

        int ofs = QMatFileModel::linear_index(m_Var.data(), indices);
        //QSharedPointer<char> val(new char[m_Var->dims[m_Var->rank-1]]);
        qDebug() << "start:" << ofs << "stride:" << 1 << "edge:" << m_Var->dims[m_Var->rank - 1];
        //Mat_VarReadDataLinear
        // Mat_VarRead
        // m_Var->class_type = MAT_C_UINT8;
        // Mat_VarReadDataLinear(m_Mat.data(), m_Var.data(), val.data(), ofs, 1, m_Var->dims[m_Var->rank - 1]);
        // m_Var->class_type = MAT_C_CHAR;
        qDebug() << "m_Var->data:" << m_Var->data;
        return QString::fromLatin1(((char*) m_Var->data) + ofs, m_Var->dims[m_Var->rank-1]);

    } else {
        return name(indices);
    }
}


QAbstractItemModel* QMatCharModel::navigateInto(const QModelIndex &index) {
    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    int sz = indices.size();
    if (sz >= m_Var->rank) {
        return NULL;
    } else {
        QAbstractItemModel *model = new QMatCharModel(m_Mat, m_Var, indices);
        dynamic_cast<QMatNavigable*>(model)->setParentName(name());
        return model;
    }
}

bool QMatCharModel::equals(QMatNavigable *other) {
    return (name() == other->name());
}

QString QMatCharModel::name() {
    return name(m_Indices);
}

void QMatCharModel::setParentName(const QString &parentName) {
    m_ParentName = parentName;
}

QString QMatCharModel::name(const QVector<size_t> &indices) const {
    QString tmp = m_ParentName;
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
    return tmp;
}

QString QMatCharModel::childName(const QModelIndex &index) {
    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    return name(indices);
}
