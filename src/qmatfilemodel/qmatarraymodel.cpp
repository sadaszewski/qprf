//
// Copyright (C) Stanislaw Adaszewski, 2016
// All Rights Reserved.
//

#include "qmatarraymodel.h"
#include "qmatfilemodel.h"

#include <QDebug>

class TypeHelperBase {
public:
    static QSharedPointer<TypeHelperBase> forClass(matio_classes cls, bool isComplex, void *val);

    virtual ~TypeHelperBase() {}
    virtual QString toString() = 0;
};

template<typename T> class TypeHelper: public TypeHelperBase {
public:
    TypeHelper(bool isComplex, T *val) {
        m_IsComplex = isComplex;
        m_Val = *val;
        if (isComplex)
            m_Imag = *(val + 1);
    }

    QString toString() {
        if (m_IsComplex)
            return QString("%1 + %2i").arg(m_Val).arg(m_Imag);
        else
            return QString("%1").arg(m_Val);
    }

private:
    T m_Val;
    T m_Imag;
    bool m_IsComplex;
};

QSharedPointer<TypeHelperBase> TypeHelperBase::forClass(matio_classes cls, bool isComplex, void *val) {
    switch(cls) {
    case MAT_C_CHAR: return QSharedPointer<TypeHelperBase>(new TypeHelper<quint8>(isComplex, (quint8*) val));
    case MAT_C_INT8: return QSharedPointer<TypeHelperBase>(new TypeHelper<qint8>(isComplex, (qint8*) val));
    case MAT_C_UINT8: return QSharedPointer<TypeHelperBase>(new TypeHelper<quint8>(isComplex, (quint8*) val));
    case MAT_C_INT16: return QSharedPointer<TypeHelperBase>(new TypeHelper<qint16>(isComplex, (qint16*) val));
    case MAT_C_UINT16: return QSharedPointer<TypeHelperBase>(new TypeHelper<quint16>(isComplex, (quint16*) val));
    case MAT_C_INT32: return QSharedPointer<TypeHelperBase>(new TypeHelper<qint32>(isComplex, (qint32*) val));
    case MAT_C_UINT32: return QSharedPointer<TypeHelperBase>(new TypeHelper<quint32>(isComplex, (quint32*) val));
    case MAT_C_INT64: return QSharedPointer<TypeHelperBase>(new TypeHelper<qint64>(isComplex, (qint64*) val));
    case MAT_C_UINT64: return QSharedPointer<TypeHelperBase>(new TypeHelper<quint64>(isComplex, (quint64*) val));
    case MAT_C_SINGLE: return QSharedPointer<TypeHelperBase>(new TypeHelper<float>(isComplex, (float*) val));
    case MAT_C_DOUBLE: return QSharedPointer<TypeHelperBase>(new TypeHelper<double>(isComplex, (double*) val));
    default: return QSharedPointer<TypeHelperBase>();
    }
}

QMatArrayModel::QMatArrayModel(QSharedPointer<mat_t> mat, QSharedPointer<matvar_t> var) {
    m_Mat = mat;
    m_Var = var;
    //m_Var->class_type;
    //init();
}

QMatArrayModel::QMatArrayModel(QSharedPointer<mat_t> mat,
    QSharedPointer<matvar_t> var,
    const QVector<size_t> &indices):
    QMatArrayModel(mat, var) {

    m_Indices = indices;
    //init();
}

QMatArrayModel::~QMatArrayModel() {
    qDebug() << "~QMatArrayModel():" << name();
}

#if 0

void QMatArrayModel::init() {
    // int sz = m_Indices.
    // for (int i = 0; i < 16; i++) {
//        m_RowCache.append(QSharedPointer<char>(new char[m_Var->]));
  //  }
    // int nbytes = 16384;
    // m_Cache = QSharedPointer<char>(new char[nbytes]);
    // m_CacheStart = -1;
    m_ElemSize = Mat_SizeOfClass(m_Var->class_type);
    if (m_Var->isComplex) m_ElemSize *= 2;
    m_CacheStartCol = m_CacheEndCol = -1;
    m_CacheStartRow = m_CacheEndRow = -1;
    // m_CacheSize = nbytes / m_ElemSize;
    // qDebug() << "m_CacheSize:" << m_CacheSize << "m_ElemSize:" << m_ElemSize;
    qDebug() << "m_ElemSize:" << m_ElemSize;
}

#endif

int QMatArrayModel::rowCount(const QModelIndex &parent) const {
    int sz = m_Indices.size();
    return sz < m_Var->rank ? m_Var->dims[sz] : 0;
}

int QMatArrayModel::columnCount(const QModelIndex &parent) const {
    int sz = m_Indices.size();
    return sz + 1 < m_Var->rank ? m_Var->dims[sz + 1] : 1;
}

QModelIndex QMatArrayModel::index(int row, int column, const QModelIndex &parent) const {
    return createIndex(row, column);
}

QModelIndex QMatArrayModel::parent(const QModelIndex &child) const {
    return QModelIndex();
}

QVariant QMatArrayModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    int sz = indices.size();
    if (sz >= m_Var->rank) {
        int elemSize = Mat_SizeOfClass(m_Var->class_type);
        if (m_Var->isComplex) elemSize *= 2;
        int ofs = QMatFileModel::linear_index(m_Var.data(), indices) * elemSize;
        return TypeHelperBase::forClass(m_Var->class_type, m_Var->isComplex,
            ((char*) m_Var->data) + ofs)->toString();
    } else {
        return name(indices);
    }
}

#if 0
QVariant QMatArrayModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole)
        return QVariant();

    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    int sz = indices.size();
    if (sz >= m_Var->rank) {
        // char val[16];

        int stride = 1;
        // int subofs = 0;
        for (int i = 0; i < sz - 2; i++)
            stride *= m_Var->dims[i];

        int subofs = QMatFileModel::linear_index(m_Var.data(), QVector<size_t>(m_Indices) << 0 << 0);

        int nrows = m_Var->dims[sz - 2];
        int ncols = sz - 1 < m_Var->rank ? m_Var->dims[sz - 1] : 1;



        if (index.row() >= m_CacheStartRow && index.row() < m_CacheEndRow &&
            index.column() >= m_CacheStartCol && index.column() < m_CacheEndCol) {

            int cacheofs =
                m_ElemSize
                 * ((index.column() - m_CacheStartCol) * nrows
                 + index.row() - m_CacheStartRow);

            return TypeHelperBase::forClass(m_Var->class_type, m_Var->isComplex,
                m_Cache.data() + cacheofs)->toString();
        } else {

            m_CacheStartRow = index.row() - 2;
            m_CacheStartCol = index.column() - 2;
            m_CacheEndRow = index.row() + 8;
            m_CacheEndCol = index.column() + 8;

            if (m_CacheStartRow < 0) m_CacheStartRow = 0;
            if (m_CacheEndRow > nrows) m_CacheEndRow = nrows;

            if (m_CacheStartCol < 0) m_CacheStartCol = 0;
            if (m_CacheEndCol > ncols) m_CacheEndCol = ncols;

            int ncacherows = m_CacheEndRow - m_CacheStartRow;
            int ncachecols = m_CacheEndCol - m_CacheStartCol;

            qDebug() << "subofs:" << subofs << "stride:" << stride << "nrows:" << nrows << "ncols:" << ncols;

            qDebug() << "m_CacheStartRow:" << m_CacheStartRow << "m_CacheEndRow:" << m_CacheEndRow;
            qDebug() << "m_CacheStartCol:" << m_CacheStartCol << "m_CacheEndCol:" << m_CacheEndCol;
            qDebug() << "ncacherows:" << ncacherows << "ncachecols:" << ncachecols;

            char *buf = new char[ncacherows * ncachecols * m_ElemSize];
            m_Cache = QSharedPointer<char>(buf);

            for (int i = m_CacheStartCol; i < m_CacheEndCol; i++) {
                int ofs = subofs + (i * nrows + m_CacheStartRow) * stride;

                qDebug() << "ofs:" << ofs;

                Mat_VarReadDataLinear(m_Mat.data(), m_Var.data(), buf, ofs, stride,
                    ncacherows);

                buf += m_ElemSize * ncacherows;
            }

            int cacheofs =
                m_ElemSize
                 * ((index.column() - m_CacheStartCol) * nrows
                 + index.row() - m_CacheStartRow);

            return TypeHelperBase::forClass(m_Var->class_type, m_Var->isComplex,
                m_Cache.data() + cacheofs)->toString();

        }

        // int ofs = QMatFileModel::linear_index(m_Var.data(), indices);

        /* if (m_CacheStart != -1 && ofs >= m_CacheStart && ofs < m_CacheStart + m_CacheSize) {
            return TypeHelperBase::forClass(m_Var->class_type, m_Var->isComplex, m_Cache.data() + m_ElemSize * (ofs - m_CacheStart))->toString();
        } else {
            int nelem = 1;
            for (int i = 0; i < m_Var->rank; i++) nelem *= m_Var->dims[i];
            qDebug() << "nelem:" << nelem;
            m_CacheStart = ofs;
            qDebug() << "m_CacheStart:" << ofs;
            Mat_VarReadDataLinear(m_Mat.data(), m_Var.data(), m_Cache.data(), ofs, 1,
                m_CacheStart + m_CacheSize <= nelem ? m_CacheSize : nelem - m_CacheStart);

            return TypeHelperBase::forClass(m_Var->class_type, m_Var->isComplex,
                m_Cache.data() + m_ElemSize * (ofs - m_CacheStart))->toString();
        } */


    } else {
        return name(indices);
    }
}
#endif


QAbstractItemModel* QMatArrayModel::navigateInto(const QModelIndex &index) {
    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    int sz = indices.size();
    if (sz >= m_Var->rank) {
        return NULL;
    } else {
        QAbstractItemModel *model = new QMatArrayModel(m_Mat, m_Var, indices);
        dynamic_cast<QMatNavigable*>(model)->setParentName(name());
        return model;
    }
}

bool QMatArrayModel::equals(QMatNavigable *other) {
    return (name() == other->name());
}

QString QMatArrayModel::name(const QVector<size_t> &indices) const {
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

QString QMatArrayModel::name() {
   return name(m_Indices);
}

void QMatArrayModel::setParentName(const QString &parentName) {
    m_ParentName = parentName;
}

QString QMatArrayModel::childName(const QModelIndex &index) {
    QVector<size_t> indices(m_Indices);
    indices.append(index.row());
    indices.append(index.column());

    return name(indices);
}
