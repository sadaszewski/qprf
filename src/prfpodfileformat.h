//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFPODFILEFORMAT_H
#define PRFPODFILEFORMAT_H

/*
 *
 * Example:
 *
 * {"name": "foo", "type": "struct", "dimensions": [2], "count": 2,
 *  "total_count": "0x0000000000000002", "data_offset": "0x89abcdef89abcdef",
 *  "skip_offset": "0xdeadbeeffeebdaed", "append_offset": "0x0000000000000000",
 *  "fields": [
 *      {"name": "a", "dimensions": [10, 10, 10], "type": "char"},
 *      {"name": "b", "dimensions": [10, 10], "type": "float32"}
 *  ]
 * }
 * {"type": "append_struct", "name": "foo", "count": 5, "data_offset": "0x1234123412341234",
 *  "skip_offset": "0xdeadbeeffeebdaed", "append_offset": "0x0000000000000000"}
 *
 * {"type": "cell_array", "name": "foo",
 *  "dimensions": [2, 2], "cell_offsets": [
 *      "0xdeadbeeffeebdaed", "0xdeadbeeffeebdaed", "0xdeadbeeffeebdaed", "0xdeadbeeffeebdaed"
 *  ]
 * }
 *
 * <struct name="foo" rank="2" dimensions="2 2" data-offset="145" skip-offset="0x131243124122134" append-offset="0x0000000000000000">
 *     <field name="a">
 *         <matrix rank="3" dimensions="10 10 10" type="char" complex="true" />
 *     </field>
 * </struct>
 *
 * <append-struct name="foo" count="5" data-offset="1234" append-offset="0x0000000000000000" />
 * <append-cell-array name="bar" count="5" skip-offset="0x0000000000000000">
 *      <matrix rank="2" dimensions="10 10" type="float32" data-offset="4567" append-offset="0x0000000000000000" />
 *      <matrix rank="2" dimensions="10 10" type="float32" data-offset="5567" append-offset="0x0000000000000000" />
 *      <matrix rank="2" dimensions="10 10" type="float32" data-offset="6567" append-offset="0x0000000000000000" />
 * </append-cell-array">
 *
 * <cell-array name="x_samples" rank="2" dimensions="2000 1">
 *     <matrix rank="2" dimensions="65231 1" type="float32" data-offset="1234" />
 * </cell-array>
 *
 * <flex-struct name="results" rank="2" dimensions="100 1">
 *      <field name="x">
 *          <matrix ... />
 *          <struct ... />
 *      </field>
 *      <field name="y">
 *      </field>
 *      <field name="z">
 *      </field>
 * </flex-struct>
 *
 * <Struct Name="foo" Rank="2" Dimensions="2 2" DataOffset="1234">
 *      <Field Name="a">
 *          <Matrix Rank="3" Dimensions="10 10 10" Type="CHAR" />
 *      </Field>
 *      <Field Name="b">
 *          <Matrix Rank="3" Dimensions="10 10 10" Type="INT32" />
 *      </Field>
 * </Struct>
 * <CellArray Name="bar" Rank="2" Dimensions="10 10">
 *     <Matrix Rank="4" Type="CHAR" Dimensions="10 10 10" DataOffset="1234" />
 * </CellArray>
 * <CellArray Name="baf" Rank="2" Dimensions="10 10">
 *     <Matrix Rank="4" Type="CHAR" Dimensions="10 10 10" DataOffset="1234" />
 * </CellArray>
 *
 * Matrix.Float32(100, 1).data(&x_samples[0]).write(&f);
 * Struct(2,1).Field("samples_x").append(Matrix.Float32(100, 1).data(&x_samples[0])
 *
 */

#include <QVector>
#include <QString>

#include <QDomDocument>
#include <QIODevice>
#include <QSharedPointer>
#include <QSet>
#include <QIODevice>
#include <QMap>

namespace PrfPod {

const int PffTypeMatrix = 1;
const int PffTypeCellArray = 2;
const int PffTypeStruct = 3;

const int BlockTypePrologue = 1;
const int BlockTypeMatrixDecl = 2;
const int BlockTypeMatrixData = 3;
const int BlockTypeDirectory = 4;
const int BlockTypeCellArrayDecl = 5;
const int BlockTypeCellArrayData = 6;
const int BlockTypeStructDecl = 7;
// const int BlockTypeStructData = 8;
const uint64_t Magic = 0x0066667020666650ULL;

#define BLOCK_HDR \
    uint64_t m_Type; \
    uint64_t m_NextBlockOfs;

#pragma pack(push)

#pragma pack(1)
struct BlockHeader {
    BLOCK_HDR
};

#pragma pack(1)
struct Prologue {
    BLOCK_HDR

    uint64_t m_Magic; // "Pff pff\x00"
    uint64_t m_Version;
    uint64_t m_MetaLen;
    uint64_t m_DirOfs;

    // char m_Meta[m_MetaLen];
};

#define VARIABLE_HDR \
    uint64_t m_Id; \
    uint64_t m_NameLen; \
    uint64_t m_NextVarOfs;
    // char m_Name[m_NameLen];


#pragma pack(1)
struct VariableHeader {
    BLOCK_HDR

    VARIABLE_HDR
};

#pragma pack(1)
struct MatrixDecl {
    BLOCK_HDR

    VARIABLE_HDR

    uint64_t m_Rank;
    uint64_t m_DataType;
    uint64_t m_IsComplex;
    uint64_t m_DataOfs;
    // uint64_t m_Dimensions[m_Rank];
};

#define DATA_HDR \
    uint64_t m_Id; \
    uint64_t m_NextDataOfs;

#pragma pack(1)
struct MatrixData {
    BLOCK_HDR

    DATA_HDR

    uint64_t m_Count;

    // char data[];
};

#pragma pack(1)
struct Directory {
    BLOCK_HDR

    uint64_t m_NextDirOfs;
    uint64_t m_Count;

    // uint64_t m_NameLengths[m_Count];
    // char m_Names[m_Count][];
    // uint64_t m_Offsets[m_Count];
};

#pragma pack(pop)

enum DataType {
    Char,
    Int8,
    Int16,
    Int32,
    Int64,
    Uint8,
    Uint16,
    Uint32,
    Uint64,
    Float32,
    Float64
};

const int ComplexFlag = (1 << 31);

/* class Field {

public:
    size_t sizeInBytes();

protected:
    QString m_Name;
    DataType m_DataType;
    size_t m_Rank;
    QVector<size_t> m_Dimensions;
}; */


class OFileImpl {
public:
    OFileImpl(const QString &fname);
    bool write(const QByteArray&);
    void close();
    uint64_t newVariable(const QString &name);
    bool writeDir();
    uint64_t bytesWritten() { return m_BytesWritten; }
    bool isValid() { return m_IsValid; }
    void newBlock();

protected:

    QMap<QString, uint64_t> m_NamesToOffsets;
    size_t m_BytesWritten;
    QSharedPointer<QIODevice> m_IODevice;
    uint64_t m_LastId;
    uint64_t m_LastVarOfs;
    uint64_t m_LastBlockOfs;
    bool m_IsValid;

};

class OFile {
public:
    OFile() {}
    OFile(const QString &fname) { m_Impl = QSharedPointer<OFileImpl>(new OFileImpl(fname)); }
    bool write(const QByteArray &ba) const { return m_Impl->write(ba); }
    void close() { m_Impl->close(); }
    uint64_t newVariable(const QString &name) const { return m_Impl->newVariable(name); }
    void newBlock() const { return m_Impl->newBlock(); }
    uint64_t bytesWritten() const { return m_Impl->bytesWritten(); }
    bool isValid() const { return m_Impl->isValid(); }

protected:
    QSharedPointer<OFileImpl> m_Impl;

};

class OFactoryBase;

class OBaseImpl {
public:
    OBaseImpl(const OFactoryBase &fac);
    virtual ~OBaseImpl() {}

    void setName(const QString &name) { m_Name = name; }
    const QString& name() const { return m_Name; }
    void setDimensions(size_t rank, const size_t *dims);
    size_t rank() const { return m_Dimensions.size(); }
    const size_t* dimensions() const { return &m_Dimensions[0]; }
    bool isValid() const { return m_IsValid; }

    // virtual void write(QIODevice*) = 0;
    // virtual void read(QIODevice*) = 0;
    virtual void flush() = 0;
    virtual size_t sizeInBytes() = 0;

protected:
    const OFile &m_File;
    QString m_Name;
    // size_t m_Rank;
    QVector<size_t> m_Dimensions;
    bool m_IsValid;
};


class OBase {
public:
    virtual ~OBase() {}

    void setName(const QString &name) { m_Impl->setName(name); }
    const QString& name() const { return m_Impl->name(); }
    void setDimensions(size_t rank, size_t *dims) { m_Impl->setDimensions(rank, dims); }
    size_t rank() const { m_Impl->rank(); }
    const size_t* dimensions() const { return m_Impl->dimensions(); }
    void flush() { m_Impl->flush(); }
    size_t sizeInBytes() { return m_Impl->sizeInBytes(); }
    bool isValid() const { return m_Impl->isValid(); }


protected:
    QSharedPointer<OBaseImpl> m_Impl;
};

class OFactoryBase {
public:
    OFactoryBase(OFile &of);
    virtual ~OFactoryBase() {}
    OFactoryBase& setName(const QString&);

    OFactoryBase& setDimensions(size_t dim_1, size_t dim_2 = -1, size_t dim_3 = -1,
        size_t dim_4 = -1, size_t dim_5 = -1, size_t dim_6 = -1,
        size_t dim_7 = -1, size_t dim_8 = -1, size_t dim_9 = -1);
    OFactoryBase& setDimensions(size_t rank, size_t *dims);
    OFactoryBase& setDimensions(const QVector<size_t> &dims);
    const QVector<size_t> dimensions() const { return m_Dimensions; }
    const OFile& file() const { return m_File; }
    const QString& name() const { return m_Name; }

protected:
    OFile m_File;
    QVector<size_t> m_Dimensions;
    QString m_Name;
};


class OMatrixFactory;
// class OStructFactory;
class OMatrix;

class OMatrixImpl: public OBaseImpl {
protected:
    OMatrixImpl(const OMatrixFactory &fac);

public:
    bool append(size_t count, void *data);
    void flush();
    size_t sizeInBytes();

protected:
    MatrixDecl m_Decl;
    uint64_t m_DeclOfs;
    uint64_t m_Dim_1_to_N_Prod;
    uint64_t m_DataTypeSize;

    friend class OMatrix;
};

class OMatrix: public OBase {
protected:
    OMatrix(const OMatrixFactory &fac) { m_Impl = QSharedPointer<OBaseImpl>(new OMatrixImpl(fac)); }

public:

    bool append(size_t count, void *data) { return dynamic_cast<OMatrixImpl*>(m_Impl.data())->append(count, data); }

    friend class OMatrixFactory;
};

class OMatrixFactory: public OFactoryBase {
public:
    OMatrixFactory(OFile &of);
    OMatrixFactory& setType(DataType);
    OMatrixFactory& setComplex(bool);
    OMatrixFactory& setDimensions(size_t dim_1, size_t dim_2 = -1, size_t dim_3 = -1,
        size_t dim_4 = -1, size_t dim_5 = -1, size_t dim_6 = -1,
        size_t dim_7 = -1, size_t dim_8 = -1, size_t dim_9 = -1);
    OMatrixFactory& setDimensions(size_t rank, size_t *dims);
    OMatrixFactory& setDimensions(const QVector<size_t> dims);
    OMatrixFactory& setName(const QString &name);
    OMatrix build();
    uint64_t dataType() const { return m_DataType; }
    bool isComplex() const { return m_IsComplex; }

protected:
    DataType m_DataType;
    bool m_IsComplex;
};

/* class OStructImpl: public OBaseImpl {
protected:
    OStructImpl(OFile &of): m_File(of) {}

public:
    void append(size_t count, void *data);
    void flush();
    size_t sizeInBytes();

protected:
    QVector<Field> m_Fields;

    QVector<void*> m_DataToFlush;
    QVector<void*> m_CountsToFlush;
    size_t m_FlushedCount;
    OFile m_File;
    size_t m_AppendAttrOfs; // whenever we flush we update this

    friend class OStructFactory;
    friend class OStruct;
}; */

/* class OStruct: public OBase {
protected:
    OStruct(OFile &of) { m_Impl = QSharedPointer<OBaseImpl>(new OStructImpl(of)); }

public:
    void append(size_t count, void *data) { dynamic_cast<OStructImpl*>(m_Impl.data())->append(count, data); }


protected:

    friend class OStructFactory;
}; */



/* class OStructFactory {
public:
    OStructFactory(OFile&, size_t dim_1, size_t dim_2 = -1, size_t dim_3 = -1,
        size_t dim_4 = -1, size_t dim_5 = -1, size_t dim_6 = -1, size_t dim_7 = -1,
        size_t dim_8 = -1, size_t dim_9 = -1);
    void setName(const QString&);
    void addField(const QString& name, DataType dtype, size_t rank, size_t *dims);
    void addField(const QString& name, DataType dtype, size_t dim_0, size_t dim_1,
        size_t dim_2 = -1, size_t dim_3 = -1, size_t dim_4 = -1,
        size_t dim_5 = -1, size_t dim_6 = -1, size_t dim_7 = -1,
        size_t dim_8 = -1, size_t dim_9 = -1);
    OStruct build();
}; */

/* class OCellArray {
public:
    void append(size_t count);
    void setCell(size_t index, const OBase &value);
    void flush();
}; */


/* class IBase {

}; */


/* class IStruct {
public:
    IStruct();

}; */


class IVariableImpl {

public:
    virtual ~IVariableImpl() {}
    const QString& name() const { return m_Name; }
    int varType() const { return m_VarType; }
    bool isNull() const { return m_IsNull; }
    bool isValid() const { return m_IsValid; }

protected:
    // QVector<uint64_t> m_Dimensions;
    QString m_Name;
    int m_VarType;
    bool m_IsNull;
    bool m_IsValid;
};

class IMatrix;

class IVariable {
public:
    // const QVector<uint64_t>& dimensions() const { return m_Impl->dimensions(); }
    int varType() const { return m_Impl->varType(); }
    bool isNull() const { return m_Impl->isNull(); }
    bool isValid() const { return m_Impl->isValid(); }

protected:
    QSharedPointer<IVariableImpl> m_Impl;
};

class IFile;



class IMatrixImpl: public IVariableImpl {
public:
    uint64_t maxBatchCount() { return m_MaxBatchCount; }

protected:
    uint64_t m_MaxBatchCount;
};

class IMatrix: public IVariable {
public:
    // IMatrix()
    IMatrix(const IVariable&);
    uint64_t maxBatchCount() { return m_Impl->maxBatchCount(); }
    bool readBatch(void *data, uint64_t *count);

    // bool readAll(void*);
    // bool readChunk(void*);

protected:
    QSharedPointer<IMatrixImpl> m_Impl;
};


class IFileImpl {
public:
    IFileImpl(const QString &fname);
    bool isValid() const { return m_IsValid; }
    bool read(void *data, uint64_t sz);
    const QString& meta() const { return m_Meta; }
    IVariable readVariable(const QString &name);

protected:
    QSharedPointer<QIODevice> m_IODevice;
    bool m_IsValid;
    QString m_Meta;
};

class IFile {
public:
    IFile(const QString& fname) { m_Impl = QSharedPointer<IFileImpl>(new IFileImpl(fname)); }
    bool isValid() const { return m_Impl->isValid(); }
    bool read(char *data, uint64_t sz) const { return m_Impl->read(data, sz); }
    const QString& meta() const { return m_Impl->meta(); }
    IVariable readVariable(const QString &name = QString()) const { return m_Impl->readVariable(name); }
    // IMetaData readDeclarations();
    // void readAll();
    // IMatrix readMatrix(const QString &name);

protected:
    QSharedPointer<IFileImpl> m_Impl;
};


class IMatrixFactory {
public:
    IMatrixFactory(const IFile &f): m_File(f) {}


protected:
    IFile m_File;

};


#if 0
class Struct: public Base {
private:
    Struct(const std::vector<Field> &fields);

public:
    void resize(size_t count);
    void data(void *data);
    void append(void *data);
    void set(size_t index, void *data);
    void* at(size_t index);

private:
    std::vector<Field> m_Fields;
    size_t m_Count;
};


#define MATRIX_CTOR_DECL(typ) static Matrix typ(size_t dim_0 = 1, size_t dim_1 = -1, size_t dim_2 = -1, \
    size_t dim_3 = -1, size_t dim_4 = -1, size_t dim_5 = -1, size_t dim_6 = -1, \
    size_t dim_7 = -1, size_t dim_8 = -1, size_t dim_9 = -1);


class MatrixImpl: public Base {

public:
    MatrixImpl(DataType type, size_t rank, size_t *dimensions);
    size_t rank() const { return 0; }
    const size_t* dimensions() const { return 0; }
    DataType dataType() { return (DataType) 0; }
    void setData(void*) {  }
    void* data() { return 0; }

private:
    size_t m_Rank;
    std::vector<size_t> m_Dimensions;
    DataType m_DataType;
};

class Matrix: public Base {

public:

    MATRIX_CTOR_DECL(Char)
    MATRIX_CTOR_DECL(Int8)
    MATRIX_CTOR_DECL(Int16)
    MATRIX_CTOR_DECL(Int32)
    MATRIX_CTOR_DECL(Int64)
    MATRIX_CTOR_DECL(Uint8)
    MATRIX_CTOR_DECL(Uint16)
    MATRIX_CTOR_DECL(Uint32)
    MATRIX_CTOR_DECL(Uint64)
    MATRIX_CTOR_DECL(Float32)
    MATRIX_CTOR_DECL(Float64)


    size_t rank() const { return m_Impl->rank(); }
    const size_t* dimensions() const { return m_Impl->dimensions(); }
    DataType dataType() { return m_Impl->dataType(); }
    void setData(void *data) { m_Impl->setData(data); }
    void* data() { return m_Impl->data(); }

private:
    QSharedPointer<MatrixImpl> m_Impl;
};

class CellArray: public Base {
public:
    size_t rank() const { return m_Rank; }
    const size_t* dimensions() const { return &m_Dimensions[0]; }
    void setCell(size_t index, Base *value);
    Base* cell(size_t index);

private:
    size_t m_Rank;
    std::vector<size_t> m_Dimensions;
    DataType m_DataType;
};

class StructFactory {
public:
    void addField(const char *name, size_t sz);
    Struct* create();
};

class MatrixFactory {
public:
    void setRank(size_t rank);
    void setDimensions(size_t *dims);
    void setDataType(DataType);
    Matrix* create();
};

class CellArrayFactory {
public:
    void setRank(size_t rank);
    void setDimensions(size_t *dims);
    CellArray* create();
};


class File
{
public:
    File();

    Matrix newMatrix(const MatrixFactory&);
    Struct newStruct(const StructFactory&);

};
#endif


}

#endif // PRFPODFILEFORMAT_H
