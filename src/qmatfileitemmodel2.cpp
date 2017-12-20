//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "qmatfileitemmodel2.h"

#include <QDebug>

QMatFileItemModel2::QMatFileItemModel2(const QString &fname) {
    mat_t *mat = Mat_Open(fname.toLatin1(), MAT_ACC_RDONLY);
    m_Mat = QSharedPointer<mat_t>(mat, QMatFileItemModel2::MatDeleter());
    m_FieldIndex = -1;
    init();
}

QMatFileItemModel2::QMatFileItemModel2(QSharedPointer<mat_t> mat) {
    m_Mat = mat;
    m_FieldIndex = -1;
    init();
}

QMatFileItemModel2::QMatFileItemModel2(QSharedPointer<QMatFileItemModel2> parent, QSharedPointer<matvar_t> var) {
    m_Parent = parent;
    m_Mat = parent->m_Mat;
    m_Var = var;
    m_FieldIndex = -1;
    init();
}

QMatFileItemModel2::QMatFileItemModel2(QSharedPointer<QMatFileItemModel2> parent, int field_index) {
    m_Parent = parent;
    m_Mat = parent->m_Mat;
    m_Var = parent->m_Var;
    m_FieldIndex = field_index;
    init();
}

QMatFileItemModel2::QMatFileItemModel2(QSharedPointer<QMatFileItemModel2> parent, const QModelIndex &index) {
    m_Parent = parent;
    m_Mat = parent->m_Mat;
    m_Var = parent->m_Var;
    m_FieldIndex = parent->m_FieldIndex;
    m_Indices = QVector<size_t>(m_Parent->m_Indices);
    m_Indices.append(index.row());
    m_Indices.append(index.column());
    init();
}

QMatFileItemModel2::~QMatFileItemModel2() {
    qDebug() << "Destroying" << name() << (quintptr) this;
}

void QMatFileItemModel2::init() {

    if (m_Var.isNull()) {
        Mat_Rewind(m_Mat.data());
        matvar_t *var;
        m_RowCount = 0;
        while ((var = Mat_VarReadNextInfo(m_Mat.data())) != NULL) {
            m_VarNames.append(var->name);
            m_VarTypes.append(makeTypeDesc(var));
            m_RowCount++;
            Mat_VarFree(var);
        }
        m_ColumnCount = 2;
    } else {
        if (m_Var->class_type == MAT_C_STRUCT) {
            if (m_FieldIndex == -1) {
                m_RowCount = Mat_VarGetNumberOfFields(m_Var.data());
                m_ColumnCount = 1;
                char* const *fieldnames = Mat_VarGetStructFieldnames(m_Var.data());
                for (int i = 0; i < m_RowCount; i++) {
                    m_VarNames.append(fieldnames[i]);
                }
            } else {
                int sz = m_Indices.size();
                m_RowCount = sz < m_Var->rank ? m_Var->dims[sz] : 0;
                m_ColumnCount = sz  + 1 < m_Var->rank ? m_Var->dims[sz + 1] : 1;
            }
        } else if (isArrayLike(m_Var.data())) {
            int sz = m_Indices.size();
            m_RowCount = sz < m_Var->rank ? m_Var->dims[sz] : 0;
            m_ColumnCount = sz  + 1 < m_Var->rank ? m_Var->dims[sz + 1] : 1;
            qDebug() << "Array-Like:" << m_RowCount << m_ColumnCount;
        } else {
            m_RowCount = m_ColumnCount = 0;
        }
    }
}

int QMatFileItemModel2::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_RowCount;
}

int QMatFileItemModel2::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_ColumnCount;
}

QModelIndex QMatFileItemModel2::index(int row, int column, const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return createIndex(row, column);
}

QModelIndex QMatFileItemModel2::parent(const QModelIndex &child) const {
    Q_UNUSED(child);
    return QModelIndex();
}

QVariant QMatFileItemModel2::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    if (m_Var.isNull()) {
        if (index.column() == 0)
            return m_VarNames[index.row()];
        else
            return m_VarTypes[index.row()];
    }


    if (m_Var->class_type == MAT_C_STRUCT) {
        if (m_FieldIndex == -1) {
            return m_VarNames[index.row()];
            //Mat_VarGetS
        }
    }

    if (isArrayLike(m_Var.data())) {

        QVector<size_t> indices(m_Indices);
        indices.append(index.row());
        indices.append(index.column());

        if (indices.size() >= m_Var->rank) {
            // ok ready to retrieve some data
            return "data";
        } else {
            QString tmp = m_Var->name;
            if (m_Var->class_type == MAT_C_CELL)
                tmp += "{";
            else
                tmp += "(";
            for (int i = 0; i < indices.size(); i++) {
                if (i > 0) tmp += ",";
                tmp += QString("%1").arg(indices[i]);
            }
            if (m_Var->class_type == MAT_C_CELL)
                tmp += ", ...}";
            else
                tmp += ", ...)";
            return tmp;
        }




    }

    return QVariant();

}

QVariant QMatFileItemModel2::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Vertical)
        return QString("%1").arg(section + 1);

    if (m_Var.isNull()) {
        if (section == 0) return "Name";
        if (section == 1) return "Type";
    }

    return QVariant();
}

QString QMatFileItemModel2::name() const {
    QString ret;

    if (!m_Parent.isNull()) ret += m_Parent->name();

    if (m_Var.isNull()) return "";



    if (m_Var->class_type == MAT_C_STRUCT) {
        if (m_FieldIndex == -1)
            ret += m_Var->name;
        else
            ret += QString(".%1").arg(m_Parent->m_VarNames[m_FieldIndex]);
    } else if (isArrayLike(m_Var.data())) {

        if (m_Indices.size() == 0)
            ret += m_Var->name;
        else {

            if (ret.at(ret.size() - 1) == ')')
                ret = ret.left(ret.length() - 1) + ",";
            else
                ret += "(";

            ret += QString("%1,%2")
                .arg(m_Indices[m_Indices.size() - 2])
                .arg(m_Indices[m_Indices.size() - 1]);

            ret += ")";
        }
    }
    return ret;
}

QSharedPointer<QMatFileItemModel2> QMatFileItemModel2::navigateInto(QSharedPointer<QMatFileItemModel2> self,
    const QModelIndex &index) const {

    if (m_Var.isNull()) {
        //Mat_Rewind(m_Mat.data());
        matvar_t *var = Mat_VarReadInfo(m_Mat.data(), m_VarNames[index.row()].toLatin1());
        QSharedPointer<matvar_t> var2 = QSharedPointer<matvar_t>(var, VarDeleter());
        return QSharedPointer<QMatFileItemModel2>(new QMatFileItemModel2(self, var2));
    } else {

        if (m_Var->class_type == MAT_C_STRUCT) {

            return QSharedPointer<QMatFileItemModel2>();

        } else if (isArrayLike(m_Var.data())) {

            return QSharedPointer<QMatFileItemModel2>(new QMatFileItemModel2(self, index));
        }
    }

}

bool QMatFileItemModel2::equals(const QSharedPointer<QMatFileItemModel2> &other) const {
    return false;
}

size_t QMatFileItemModel2::linear_index(QSharedPointer<matvar_t> var,
                                       const QVector<size_t> &indices) const {

    size_t linear = 0;

    for (int i = var->rank - 1; i > 0; i--) {
        linear += indices[i];
        linear *= var->dims[i - 1];
    }
    linear += indices[0];

    return linear;
}

QString QMatFileItemModel2::makeTypeDesc(matvar_t *var) const {
    QString desc;
    for (int i = 0; i < var->rank; i++) {
        if (i > 0) desc += "x";
        desc += QString("%1").arg(var->dims[i]);
    }
    desc += " ";

    if (var->isComplex) desc += "complex ";

    switch (var->class_type) {
    case MAT_C_EMPTY: desc += "empty"; break;
    case MAT_C_CELL: desc += "cell"; break;
    case MAT_C_STRUCT: desc += "struct"; break;
    case MAT_C_OBJECT: desc += "object"; break;
    case MAT_C_CHAR: desc += "char"; break;
    case MAT_C_SPARSE: desc += "sparse"; break;
    case MAT_C_DOUBLE: desc += "double"; break;
    case MAT_C_SINGLE: desc += "single"; break;
    case MAT_C_INT8: desc += "int8"; break;
    case MAT_C_UINT8: desc += "uint8"; break;
    case MAT_C_INT16: desc += "int16"; break;
    case MAT_C_UINT16: desc += "uint16"; break;
    case MAT_C_INT32: desc += "int32"; break;
    case MAT_C_UINT32: desc += "uint32"; break;
    case MAT_C_INT64: desc += "int64"; break;
    case MAT_C_UINT64: desc += "uint64"; break;
    case MAT_C_FUNCTION: desc += "function"; break;
    default: desc += "UNKNOWN"; break;
    }

    /* switch(var->data_type) {

    case MAT_T_UNKNOWN: desc += "unknown"; break;
    case MAT_T_INT8: desc += "int8"; break;
    case MAT_T_UINT8: desc += "uint8"; break;
    case MAT_T_INT16: desc += "int16"; break;
    case MAT_T_UINT16: desc += "uint16"; break;
    case MAT_T_INT32: desc += "int32"; break;
    case MAT_T_UINT32: desc += "uint32"; break;
    case MAT_T_SINGLE: desc += "single"; break;
    case MAT_T_DOUBLE: desc += "double"; break;
    case MAT_T_INT64: desc += "int64"; break;
    case MAT_T_UINT64: desc += "uint64"; break;
    case MAT_T_MATRIX: desc += "matrix"; break;
    case MAT_T_COMPRESSED: desc += "compressed"; break;
    case MAT_T_UTF8: desc += "utf8"; break;
    case MAT_T_UTF16: desc += "utf16"; break;
    case MAT_T_UTF32: desc += "utf32"; break;


    case MAT_T_ARRAY: desc += "array"; break;
    case MAT_T_CELL: desc += "cell"; break;
    case MAT_T_STRUCT: desc += "struct"; break;
    case MAT_T_STRING: desc += "string"; break;

    case MAT_T_FUNCTION: desc += "function"; break;

    default: desc += "UNKNOWN"; break;
    } */

    return desc;
}

bool QMatFileItemModel2::isArrayLike(matvar_t *var) const {
    if (var->class_type != MAT_C_FUNCTION &&
        var->class_type != MAT_C_OBJECT &&
        var->class_type != MAT_C_STRUCT)
        return true;

    return false;
}
