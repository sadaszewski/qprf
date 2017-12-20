//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "prfpodfileformat.h"

#include <QFile>

/*
 * Example:
 *
 * OFile of("test.pff");
 * OCellArray oca = OCellArrayFactory(of)
 *     .setDimensions(1)
 *     .setName("results")
 *     .build();
 * oca.append(2000);
 * QVector<OStruct> outstructs;
 * QVector<size_t> unflushedCount(2000);
 *
 * OStructFactory osf(of);
 * osf.setDimensions(1)
 *  .addField("x", Float32, 1, 1)
 *  .addField("y", Float32, 1, 1);
 *
 * for (size_t i = 0; i < n_subjects; i++) {
 *     OStruct os = osf.build();
 *     outstructs[i] = os;
 *     ocaf.setCell(i, os);
 * }
 * oca.flush(); // calls os.flush()
 * for (size_t i = 0; i < n_subjects; i++) {
 *     size_t n_samples = samples[i].size();
 *     outstruct[i].append(n_samples, &samples[i]);
 *     samples[i].clear();
 *     unflushedCount[i] += n_samples;
 *     if (unflushedCount[i] > 100) {
 *         outstruct[i].flush();
 *         unflushedCount[i] = 0;
 *     }
 * }
 * of.flush();
 *
 *
 * /

/* PrfPodFileFormat::PrfPodFileFormat()
{

} */

namespace PrfPod {
    static size_t _DataTypeSizes[] = {
        1,
        1,
        2,
        4,
        8,
        1,
        2,
        4,
        8,
        4,
        8
    };

    OBaseImpl::OBaseImpl(const OFactoryBase &fac): m_File(fac.file()) {
        setName(fac.name());
        setDimensions(fac.dimensions().size(), &fac.dimensions()[0]);
        m_IsValid = true;
    }

    void OBaseImpl::setDimensions(size_t rank, const size_t *dims) {
        // m_Rank = rank;
        m_Dimensions.resize(rank);
        for (size_t i = 0; i < rank; i++)
            m_Dimensions[i] = dims[i];
    }

    OFileImpl::OFileImpl(const QString &fname) {
        m_LastId = 0;
        m_LastVarOfs = 0;
        m_LastBlockOfs = 0;
        m_BytesWritten = 0;

        m_IODevice = QSharedPointer<QIODevice>(new QFile(fname));
        m_IsValid = m_IODevice->open(QIODevice::WriteOnly);
        if (!m_IsValid) return;

        Prologue prl;
        prl.m_Type = BlockTypePrologue;
        prl.m_NextBlockOfs = 0;
        prl.m_Magic = Magic;
        prl.m_Version = 1;
        QByteArray meta = QString("{\"comment\": \"Pff - POD File Format Version 1\"}").toUtf8().append((char) 0x00);
        prl.m_MetaLen = meta.length();
        prl.m_DirOfs = 0;
        m_IsValid &= write(QByteArray((char*) &prl, sizeof(Prologue)));
        m_IsValid &= write(meta);
    }

    bool OFileImpl::write(const QByteArray &ba) {
        int sz = ba.size();
        while (sz > 0) {
            int ret = m_IODevice->write(ba.right(sz));
            if (ret == -1) { return false; }
            sz -= ret;
        }
        m_BytesWritten += ba.size();
        return true;
    }

    void OFileImpl::close() {
        return m_IODevice->close();
    }

    uint64_t OFileImpl::newVariable(const QString &name) {
        if (m_NamesToOffsets.contains(name)) {
            return 0;
        }

        uint64_t id = ++m_LastId;

        newBlock();

        m_NamesToOffsets[name] = m_BytesWritten;
        if (m_LastVarOfs != 0) {
            m_IODevice->seek(m_LastVarOfs + offsetof(VariableHeader, m_NextVarOfs));
            m_IODevice->write((char*) &m_BytesWritten, sizeof(uint64_t));
            m_IODevice->seek(m_BytesWritten);
        }
        m_LastVarOfs = m_BytesWritten;

        return id;
    }

    bool OFileImpl::writeDir() {
        return true;
    }

    void OFileImpl::newBlock() {
        if (m_LastBlockOfs != 0) {
            m_IODevice->seek(m_LastBlockOfs + offsetof(BlockHeader, m_NextBlockOfs));
            m_IODevice->write((char*) &m_BytesWritten, sizeof(uint64_t));
            m_IODevice->seek(m_BytesWritten);
        }
        m_LastBlockOfs = m_BytesWritten;
    }

    OFactoryBase::OFactoryBase(OFile &of) {
        m_File = of;
        m_Dimensions.resize(2);
        m_Dimensions[0] = 0;
        m_Dimensions[1] = 1;
    }

    OFactoryBase& OFactoryBase::setName(const QString &name) {
        m_Name = name;
        return *this;
    }

    OFactoryBase& OFactoryBase::setDimensions(size_t dim_1, size_t dim_2, size_t dim_3,
        size_t dim_4, size_t dim_5, size_t dim_6,
        size_t dim_7, size_t dim_8, size_t dim_9) {

        m_Dimensions.resize(10);
        size_t dims[] = {0, dim_1, dim_2, dim_3, dim_4, dim_5, dim_6, dim_7, dim_8, dim_9};
        for (size_t i = 0; i < 10; i++) {
            if (dims[i] == -1) {
                m_Dimensions.resize(i);
                return *this;
            }
            m_Dimensions[i] = dims[i];
        }

        return *this;
    }

    OFactoryBase& OFactoryBase::setDimensions(size_t rank, size_t *dims) {
        m_Dimensions.resize(rank);
        for (size_t i = 0; i < rank; i++) {
            m_Dimensions[i] = dims[i];
        }
        return *this;
    }

    OFactoryBase& OFactoryBase::setDimensions(const QVector<size_t> &dims) {
        m_Dimensions = dims;
        return *this;
    }

    OMatrixFactory::OMatrixFactory(OFile &of): OFactoryBase(of) {
        m_IsComplex = false;
        m_DataType = Float64;
    }

    OMatrixFactory& OMatrixFactory::setType(DataType dtype) {
        m_DataType = dtype;
        return *this;
    }

    OMatrixFactory& OMatrixFactory::setComplex(bool is_complex) {
        m_IsComplex = is_complex;
        return *this;
    }

    OMatrixFactory& OMatrixFactory::setDimensions(size_t dim_1, size_t dim_2, size_t dim_3,
        size_t dim_4, size_t dim_5, size_t dim_6,
        size_t dim_7, size_t dim_8, size_t dim_9) {

        OFactoryBase::setDimensions(dim_1, dim_2, dim_3, dim_4, dim_5, dim_6, dim_7, dim_8, dim_9);
        return *this;
    }

    OMatrixFactory& OMatrixFactory::setDimensions(size_t rank, size_t *dims) {
        OFactoryBase::setDimensions(rank, dims);
        return *this;
    }

    OMatrixFactory& OMatrixFactory::setDimensions(const QVector<size_t> dims) {
        OFactoryBase::setDimensions(dims);
        return *this;
    }

    OMatrixFactory& OMatrixFactory::setName(const QString &name) {
        OFactoryBase::setName(name);
        return *this;
    }


    OMatrix OMatrixFactory::build() {
        return OMatrix(*this);
    }

    OMatrixImpl::OMatrixImpl(const OMatrixFactory &fac): OBaseImpl(fac) {
        m_DeclOfs = fac.file().bytesWritten();

        m_Decl.m_Type = BlockTypeMatrixDecl;
        m_Decl.m_NextBlockOfs = m_File.bytesWritten() + sizeof(MatrixDecl);

        uint64_t id = m_File.newVariable(fac.name());
        if (id == 0) {
            m_IsValid = false;
            return;
        }
        m_IsValid = (id > 0);
        m_Decl.m_Id = id;
        QByteArray name(fac.name().toUtf8().append((char) 0x00));
        m_Decl.m_NameLen = name.length();
        m_Decl.m_NextVarOfs = 0;
        m_Decl.m_NextBlockOfs += m_Decl.m_NameLen;

        m_Decl.m_Rank = fac.dimensions().size();
        m_Decl.m_DataType = fac.dataType();
        m_Decl.m_IsComplex = fac.isComplex();
        m_Decl.m_DataOfs = 0;
        m_Decl.m_NextBlockOfs += sizeof(uint64_t) * m_Decl.m_Rank;

        m_IsValid &= m_File.write(QByteArray((char*) &m_Decl, sizeof(VariableHeader)));
        m_IsValid &= m_File.write(name);
        m_IsValid &= m_File.write(QByteArray(((char*) &m_Decl) + offsetof(MatrixDecl, m_Rank),
            sizeof(MatrixDecl) - sizeof(VariableHeader)));
        m_IsValid &= m_File.write(QByteArray((char*) &fac.dimensions()[0], sizeof(uint64_t) * m_Decl.m_Rank));


        m_Dim_1_to_N_Prod = 1;
        for (uint64_t i = 1; i < m_Dimensions.size(); i++) {
            m_Dim_1_to_N_Prod *= m_Dimensions[i];
        }

        m_DataTypeSize = _DataTypeSizes[m_Decl.m_DataType];
        if (fac.isComplex()) m_DataTypeSize *= 2;
    }

    bool OMatrixImpl::append(size_t count, void *data) {
        uint64_t sz = m_Dim_1_to_N_Prod * count * m_DataTypeSize;
        return m_File.write(QByteArray((char*) data, sz));
    }

    void OMatrixImpl::flush() {

    }

    size_t OMatrixImpl::sizeInBytes() {
        return 0;
    }

    IFileImpl::IFileImpl(const QString &fname) {
        m_IODevice = QSharedPointer<QIODevice>(new QFile(fname));
        m_IsValid = m_IODevice->open(QIODevice::ReadOnly);
        if (!m_IsValid) return;

        Prologue prl;
        m_IsValid &= read(&prl, sizeof(Prologue));
        m_IsValid &= (prl.m_Magic == Magic);

        QByteArray ba(prl.m_MetaLen, 0);
        m_IsValid &= read(ba.data(), ba.length());

        m_Meta = QString::fromUtf8(ba.data(), ba.length() - 1);
    }

    bool IFileImpl::read(void *data, uint64_t sz) {
        uint64_t bytes_left = sz;
        uint64_t bytes_read = 0;
        while (bytes_left > 0) {
            int ret = m_IODevice->read((char*) data + bytes_left, bytes_left);
            if (ret == -1) {
                return false;
            }
            bytes_left -= ret;
            bytes_read += ret;
        }
        return true;
    }

}

#if 1

using namespace PrfPod;

int main(int argc, char *argv[]) {
    OFile of("test.pff");
    OMatrixFactory fac(of);
    fac.setType(Uint64)
        .setComplex(true)
        .setDimensions(2);
    uint64_t dummy[400];
    for (int i = 0; i < 400; i++) {
        dummy[i] = i;
    }
    for (int i = 0; i < 100; i++) {
        OMatrix mat = fac.setName(QString("x{%1}").arg(i)).build();
        mat.append(100, &dummy[0]);
    }
    of.close();

    IFile if_("test.pff");
    for (int i = 0; i < 100; i++) {
        IMatrix mat = if_.readVariable(QString("x{%1}").arg(i));
        if (!mat.isValid()) break; // x{..} is not a matrix shouldn't happen
        QVector<uint64_t> tmp(2 * 2 * mat.maxBatchCount());
        uint64_t count;
        while (mat.readBatch(&tmp[0], &count)) {
            for (int k = 0; k < count; k++) {
                // print out content
            }
        }
    }
}

#endif

#if 0

int main(int argc, char *argv[]) {
    PrfPod::OFile of("test.pff");
    PrfPod::OMatrix mat = PrfPod::OMatrixFactory(of)
        .setType(PrfPod::Float64)
        .setComplex(true)
        .setDimensions(2)
        .setName("results{1}.x")
        .build();
    PrfPod::OCellArray oca = PrfPod::OCellArrayFactory(of)
        .setDimensions(2000);
        .setNames("results").build();
    PrfPod::OMatrix mat = mat_fac.setName(oca.cellName(0, 0)).build();
    double mat_data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    mat.append(2, &mat_data[0]);
    //OStructChunk chunk = rec.newChunk(2);
    //chunk.setField(1, 0, val);
    //rec.append(chunk);
    mat.flush();

    of.close();
    PrfPod::IFile if("test.pff");
    PrfPod::ICellArray results = if.readCellArray("results");
    int n_voxels = results.dimensions()[0];
    PrfPod::IContainer co = if.read(PrfPod::VarSpec("results{%1}.%2")
        .range(0, 10)
        .names(QStringList() << "x" << "y" << "rfsize" << "expt" << "gain"));
    IMatrix x = co.matrix(k, 0); // matrix(QString("results{%1}.%2").arg(i).arg("x"));
    IMatrix y = co.matrix(k, 1);
    IMatrix rfsize = co.matrix(k, 2);
    IMatrix expt = co.matrix(k, 3);
    IMatrix gain = co.matrix(k, 4);
    for (int i = 0; i < n_voxels; i++) {
        PrfPod::IMatrix x = if.readMatrix(QString("results{%1}.x").arg(i));
        PrfPod::IMatrix y = if.readMatrix(QString("results{%1}.y").arg(i));
        PrfPod::IMatrix rfsize = if.readMatrix(QString("results{%1}.rfsize").arg(i));
        PrfPod::IMatrix expt = if.readMatrix(QString("results{%1}.expt").arg(i));
        PrfPod::IMatrix gain = if.readMatrix(QString("results{%1}.gain").arg(i));
        // QVector<double> data(imat.numElements());
        // imat.readAll(&data[0]);
    }
}

#endif
