//
// Copyright (C) Stanislaw Adaszewski, 2016
// All Rights Reserved.
//

#include "qmatstructmodel.h"
#include "qmatfieldmodel.h"

#include <QDebug>

QMatStructModel::QMatStructModel(QSharedPointer<mat_t> mat, QSharedPointer<matvar_t> var)
{
    m_Mat = mat;
    m_Var = var;

    char * const *fieldnames = Mat_VarGetStructFieldnames(var.data());
    int nfields = Mat_VarGetNumberOfFields(var.data());
    for (int i = 0; i < nfields; i++) {
        m_FieldNames.append(fieldnames[i]);
    }
}

QMatStructModel::~QMatStructModel() {
    qDebug() << "~QMatStructModel():" << name();
}

int QMatStructModel::rowCount(const QModelIndex &parent) const {
    return m_FieldNames.count();
}

int QMatStructModel::columnCount(const QModelIndex &parent) const {
    return 1;
}

QModelIndex QMatStructModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex QMatStructModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

QVariant QMatStructModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (index.column()) {
    case 0: return m_FieldNames[index.row()];
    default: return "NotImplemented";
    }
}

QVariant QMatStructModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Vertical)
        return QString("%1").arg(section + 1);

    switch (section) {
    case 0: return "Name";
    default: return "NotImplemented";
    }
}

QAbstractItemModel* QMatStructModel::navigateInto(const QModelIndex &index) {

    QMatFieldModel *model = new QMatFieldModel(m_Mat, m_Var, index.row(), m_FieldNames[index.row()]);
    model->setParentName(name());
    return model;

}

bool QMatStructModel::equals(QMatNavigable *other) {
    return (name() == other->name());
}

QString QMatStructModel::name() {
    return m_ParentName; // QString("%1.%2").arg(m_ParentName, m_Var->name);
}

void QMatStructModel::setParentName(const QString &parentName) {
    m_ParentName = parentName;
}

QString QMatStructModel::childName(const QModelIndex &index) {
    return QString("%1.%2").arg(m_ParentName).arg(m_FieldNames[index.row()]);
}
