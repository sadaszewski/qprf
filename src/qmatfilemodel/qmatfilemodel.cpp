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

static void var_deleter(matvar_t *var) {
    qDebug() << "var_deleter(): Deleting variable:" << var->name;
    Mat_VarFree(var);
}

QString QMatFileModel::makeTypeDesc(matvar_t *var) {
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

    return desc;
}

size_t QMatFileModel::linear_index(matvar_t *var,
    const QVector<size_t> &indices) {

    size_t linear = 0;

    for (int i = var->rank - 1; i > 0; i--) {
        linear += indices[i];
        linear *= var->dims[i - 1];
    }
    linear += indices[0];

    return linear;
}

QAbstractItemModel* QMatFileModel::modelForVar(QSharedPointer<mat_t> mat, QSharedPointer<matvar_t> var) {

    if (var->data == NULL) {
        qDebug() << "Loading data for:" << var->name;
        Mat_VarReadDataAll(mat.data(), var.data());
    }

    switch (var->class_type) {
    case MAT_C_STRUCT: return new QMatStructModel(mat, var);
    case MAT_C_CELL: return new QMatCellModel(mat, var);
    case MAT_C_CHAR: return new QMatCharModel(mat, var);
    case MAT_C_DOUBLE:
    case MAT_C_SINGLE:
    case MAT_C_INT8:
    case MAT_C_UINT8:
    case MAT_C_INT16:
    case MAT_C_UINT16:
    case MAT_C_INT32:
    case MAT_C_UINT32:
    case MAT_C_INT64:
    case MAT_C_UINT64: return new QMatArrayModel(mat, var);
    default: return NULL;
    }
}

matvar_t* QMatFileModel::resolveVariableSpec(mat_t *mat, const QString &_expr, matvar_t **topVar) {
    Mat_Rewind(mat);

    QString expr(_expr);

    QRegExp _name("[a-zA-Z][a-zA-Z0-9_]*");
    QRegExp _fieldsubs("\\.(" + _name.pattern() + ")");
    QRegExp _arysubs("\\(([0-9]+)(,[0-9]+)*\\)");
    QRegExp _celsubs("\\{([0-9]+)(,[0-9]+)*\\}");
    //QString _anysubs("(" + _fieldsubs + "|" + _arysubs + "|" + _celsubs + ")");

    //QRegExp re(QString("%1")

    qDebug() << "_celsubs:" << _celsubs.pattern();

    qDebug() << "Processing expression:" << expr;

    if (_name.indexIn(expr) != 0) {
        qDebug() << "Expression must start with a variable name.";
        *topVar = NULL;
        return NULL;
    }
    expr = expr.right(expr.length() - _name.matchedLength());

    matvar_t *var = NULL;
    while ((var = Mat_VarReadNextInfo(mat)) != NULL) {
        if (_name.cap() == var->name)
            break;
        Mat_VarFree(var);
        var = NULL;
    }

    if (var == NULL) {
        qDebug() << "Specified variable not found.";
        *topVar = NULL;
        return NULL;
    }

    qDebug() << "Top level variable found. Remaining expr:" << expr;

    // QSharedPointer<matvar_t> topVar(var, var_deleter);
    // Q_UNUSED(topVar);
    *topVar = var;

    int pendingIndex = -1;

    while (expr.length() > 0) {
        // qDebug() << "expr:" << expr;
        // qDebug() << "_celsubs.indexIn(expr):" << _celsubs.indexIn(expr);

        if (_fieldsubs.indexIn(expr) == 0) {
            if (var->class_type != MAT_C_STRUCT) {
                qDebug() << "Trying to access field of non-struct type.";
                return NULL;
            }
            if (pendingIndex == -1) {
                qDebug() << "Accessing slabs of fields is not supported.";
                return NULL;
            }
            qDebug() << "pendingIndex:" << pendingIndex;
            var = Mat_VarGetStructFieldByName(var, _fieldsubs.cap(1).toLatin1(), pendingIndex);
            if (var == NULL) {
                qDebug() << "Specified index was out of bounds for this struct.";
                return NULL;
            }
            pendingIndex = -1;
            expr = expr.right(expr.length() - _fieldsubs.matchedLength());
            qDebug() << "Field indexing found, remaining expr:" << expr;
        } else if (_arysubs.indexIn(expr) == 0) {
            if (var->class_type != MAT_C_STRUCT) {
                qDebug() << "Indexing of non-struct types is not supported.";
                return NULL;
            }
            QVector<size_t> indices;
            for (int i = 0; i < _arysubs.captureCount(); i++) {
                bool ok;
                indices.append(_arysubs.cap(i + 1).toInt(&ok));
            }
            qDebug() << "indices:" << indices;
            pendingIndex = linear_index(var, indices);
            qDebug() << "Set pendingIndex to:" << pendingIndex;
            expr = expr.right(expr.length() -_arysubs.matchedLength());
            qDebug() << "Array indexing found, remaining expr:" << expr;
        } else if (_celsubs.indexIn(expr) == 0) {
            if (var->class_type != MAT_C_CELL) {
                qDebug() << "Trying to access cell in non-cell type.";
                return NULL;
            }
            QVector<size_t> indices;
            qDebug() << "captureCount():" << _celsubs.captureCount();
            for (int i = 0; i < _celsubs.captureCount(); i++) {
                bool ok;
                indices.append(_celsubs.cap(i + 1).toInt(&ok));
            }
            qDebug() << "indices:" << indices;
            int idx = linear_index(var, indices);
            var = Mat_VarGetCell(var, idx);
            if (var == NULL) {
                qDebug() << "Specified cell is out of bounds.";
                return NULL;
            }
            expr = expr.right(expr.length() -_celsubs.matchedLength());
            qDebug() << "Cell indexing found, remaining expr:" << expr;
        } else if (expr[0] == ' ') {
            expr = expr.right(expr.length() - 1);
        } else {
            qDebug() << "Syntax error in expression.";
            return NULL;
        }
    }

    return var;

}

bool QMatFileModel::isFullReal(matvar_t *var) {
    return ((var->class_type == MAT_C_DOUBLE ||
        var->class_type == MAT_C_SINGLE ||
        var->class_type == MAT_C_INT8 ||
        var->class_type == MAT_C_UINT8 ||
        var->class_type == MAT_C_INT16 ||
        var->class_type == MAT_C_UINT16 ||
        var->class_type == MAT_C_INT32 ||
        var->class_type == MAT_C_UINT32 ||
        var->class_type == MAT_C_INT64 ||
        var->class_type == MAT_C_UINT64) &&
        var->isComplex == false);
}

QMatFileModel::QMatFileModel(QSharedPointer<mat_t> mat)
{
    m_Mat = mat;

    Mat_Rewind(mat.data());
    matvar_t *var;
    while ((var = Mat_VarReadNextInfo(mat.data())) != NULL) {
        m_Vars.append(QSharedPointer<matvar_t>(var, var_deleter));
    }
}

QMatFileModel::~QMatFileModel() {
    qDebug() << "~QMatFileModel():" << name();
}

int QMatFileModel::rowCount(const QModelIndex &parent) const {
    return m_Vars.count();
}

int QMatFileModel::columnCount(const QModelIndex &parent) const {
    return 2;
}

QModelIndex QMatFileModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex QMatFileModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

QVariant QMatFileModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (index.column()) {
    case 0: return m_Vars[index.row()]->name;
    case 1: return makeTypeDesc(m_Vars[index.row()].data());
    default: return "NotImplemented";
    }
}

QVariant QMatFileModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Vertical)
        return QString("%1").arg(section + 1);

    switch (section) {
    case 0: return "Name";
    case 1: return "Type";
    default: return "NotImplemented";
    }
}

QAbstractItemModel* QMatFileModel::navigateInto(const QModelIndex &index) {
    QSharedPointer<matvar_t> var = m_Vars[index.row()];

    //Mat_Rewind(m_Mat.data());
    // matvar_t *var = Mat_VarRead(m_Mat.data(), m_Vars[index.row()]->name);


    QAbstractItemModel *model = modelForVar(m_Mat, var);
    dynamic_cast<QMatNavigable*>(model)->setParentName(var->name);
    qDebug() << "Navigating...";
    return model;
}

bool QMatFileModel::equals(QMatNavigable *other) {
    return (name() == other->name());
}

QString QMatFileModel::name() {
    return "";
}

void QMatFileModel::setParentName(const QString&) {

}

QString QMatFileModel::childName(const QModelIndex &index) {
    return m_Vars[index.row()]->name;
}
