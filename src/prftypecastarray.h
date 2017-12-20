//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFTYPECASTARRAY_H
#define PRFTYPECASTARRAY_H

#include <qglobal.h>

namespace Prf {

class TypeCastArrayBase {
public:
    virtual ~TypeCastArrayBase() {}
    virtual double asDouble(int idx) = 0;
    virtual float asFloat(int idx) = 0;
    virtual qint8 asInt8(int idx) = 0;
    virtual quint8 asUint8(int idx) = 0;
    virtual qint16 asInt16(int idx) = 0;
    virtual quint16 asUint16(int idx) = 0;
    virtual qint32 asInt32(int idx) = 0;
    virtual quint32 asUint32(int idx) = 0;
    virtual qint64 asInt64(int idx) = 0;
    virtual quint64 asUint64(int idx) = 0;
};

template<typename T> class TypeCastArray: public TypeCastArrayBase {
public:
    TypeCastArray(void *data) {
        m_Data = (T*) data;
    }

    double asDouble(int idx) { return (double) m_Data[idx]; }
    float asFloat(int idx) { return (float) m_Data[idx]; }
    qint8 asInt8(int idx) { return (qint8) m_Data[idx]; }
    quint8 asUint8(int idx) { return (quint8) m_Data[idx]; }
    qint16 asInt16(int idx) { return (qint16) m_Data[idx]; }
    quint16 asUint16(int idx) { return (quint16) m_Data[idx]; }
    qint32 asInt32(int idx) { return (qint32) m_Data[idx]; }
    quint32 asUint32(int idx) { return (quint32) m_Data[idx]; }
    qint64 asInt64(int idx) { return (qint64) m_Data[idx]; }
    quint64 asUint64(int idx) { return (quint64) m_Data[idx]; }

private:
    T *m_Data;
};

class TypeCastArrayFactory {
public:
    static TypeCastArrayBase* create(matio_classes cls, void *data) {
        switch (cls) {
        case MAT_C_SINGLE: return new TypeCastArray<float>(data);
        case MAT_C_DOUBLE: return new TypeCastArray<double>(data);
        case MAT_C_INT8: return new TypeCastArray<qint8>(data);
        case MAT_C_UINT8: return new TypeCastArray<quint8>(data);
        case MAT_C_INT16: return new TypeCastArray<qint16>(data);
        case MAT_C_UINT16: return new TypeCastArray<quint16>(data);
        case MAT_C_INT32: return new TypeCastArray<qint32>(data);
        case MAT_C_UINT32: return new TypeCastArray<quint32>(data);
        case MAT_C_INT64: return new TypeCastArray<qint64>(data);
        case MAT_C_UINT64: return new TypeCastArray<quint64>(data);
        default: return NULL;
        }
    }
};

}

#endif // PRFTYPECASTARRAY_H

