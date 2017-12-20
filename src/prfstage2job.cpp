//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#define _USE_MATH_DEFINES

#include "prfstage2job.h"

#include <QSettings>
#include <QDebug>
#include <QFileInfo>
#include <QDir>

#include <OpenCL/opencl.h>

#include "matio.h"

#include "qmatfilemodel/qmatfilemodel.h"
#include "prftypecastarray.h"

#include <math.h>

using namespace Prf;


static const char *OPENCL_HDR = "#define H %1 \n\
        #define W %2 \n\
        #define TOT_DUR %3\n\
        #define DUR 1";

static const char *OPENCL_BODY = "__kernel void make_gaussians_no_limit( \n\
    \n\
    __global float *xs, \n\
    __global float *ys, \n\
    __global float *rfsizes, \n\
    \n\
    __global float *gaussians) { \n\
    \n\
    int id = get_global_id(0); \n\
    \n\
    float tmp; \n\
    float pdf; \n\
    int ofs = id * W * H; \n\
    float u_x = xs[id]; \n\
    float u_y = ys[id]; \n\
    float rfsize = rfsizes[id]; \n\
    float sigma_2 = pow(rfsizes[id], 2);  \n\
    \n\
    \n\
    for (int y = 0; y < H; y++) { \n\
        ofs = (id * H + y) * W; \n\
        for (int x = 0; x < W; x++) { \n\
            tmp = -0.5 * (pow(x - u_x, 2) / sigma_2 + pow(y - u_y, 2) / sigma_2); \n\
            pdf = exp(tmp) / (2 * M_PI * sigma_2); \n\
            gaussians[ofs] = pdf; \n\
            ofs++; \n\
        } \n\
    } \n\
}\n\
        \n\
        \n\
__kernel void stim_times_gaussians(__global float *stim, \n\
    __global float *gaussians, \n\
    __global float *reduced, \n\
    const int t0) { \n\
    \n\
    int id = get_global_id(0); \n\
    \n\
    for (int t = 0; t < DUR; t++) { \n\
        float accum = 0; \n\
        \n\
        for (int y = 0; y < H; y++) { \n\
            int gauss_ofs = (id * H + y) * W; \n\
            int ofs = (t0 * H + y) * W; \n\
            for (int x = 0; x < W; x++) { \n\
                float pdf = gaussians[gauss_ofs]; \n\
                accum += pdf * stim[ofs]; \n\
                ofs++; \n\
                gauss_ofs++; \n\
            } \n\
        } \n\
        \n\
        reduced[id * TOT_DUR + t0 + t] = accum; \n\
    } \n\
}";

PrfStage2Job::PrfStage2Job()
{

}

void PrfStage2Job::setInputFileName(const QString &fname) {
    m_InputFileName = fname;
}

void PrfStage2Job::setVariableSpec(const QString &spec) {
    m_VariableSpec = spec;
}

void PrfStage2Job::setOutputFileName(const QString &outname) {
    m_OutputFileName = outname;
}

void PrfStage2Job::setXStart(int n) {
    m_XStart = n;
}

void PrfStage2Job::setXEnd(int n) {
    m_XEnd = n;
}

void PrfStage2Job::setXStep(int n) {
    m_XStep = n;
}

void PrfStage2Job::setYStart(int n) {
    m_YStart = n;
}

void PrfStage2Job::setYEnd(int n) {
    m_YEnd = n;
}

void PrfStage2Job::setYStep(int n) {
    m_YStep = n;
}

void PrfStage2Job::setRFSizeStart(int n) {
    m_RFSizeStart = n;
}

void PrfStage2Job::setRFSizeEnd(int n) {
    m_RFSizeEnd = n;
}

void PrfStage2Job::setRFSizeStep(int n) {
    m_RFSizeStep = n;
}

void PrfStage2Job::saveConfig(const QString &fname) {
    QSettings config(fname, QSettings::IniFormat);
    config.setValue("stage2/inputFileName", m_InputFileName);
    config.setValue("stage2/variableSpec", m_VariableSpec);
    config.setValue("stage2/outputFileName", m_OutputFileName);

    config.setValue("stage2/xStart", m_XStart);
    config.setValue("stage2/yStart", m_YStart);
    config.setValue("stage2/rfSizeStart", m_RFSizeStart);

    config.setValue("stage2/xEnd", m_XEnd);
    config.setValue("stage2/yEnd", m_YEnd);
    config.setValue("stage2/rfSizeEnd", m_RFSizeEnd);

    config.setValue("stage2/xStep", m_XStep);
    config.setValue("stage2/yStep", m_YStep);
    config.setValue("stage2/rfSizeStep", m_RFSizeStep);
}

void PrfStage2Job::loadConfig(const QString &fname) {
    QSettings config(fname, QSettings::IniFormat);

    m_InputFileName = config.value("stage2/inputFileName").toString();
    m_VariableSpec = config.value("stage2/variableSpec").toString();
    m_OutputFileName = config.value("stage2/outputFileName").toString();

    m_XStart = config.value("stage2/xStart").toInt();
    m_YStart = config.value("stage2/yStart").toInt();
    m_RFSizeStart = config.value("stage2/rfSizeStart").toInt();

    m_XEnd = config.value("stage2/xEnd").toInt();
    m_YEnd = config.value("stage2/yEnd").toInt();
    m_RFSizeEnd = config.value("stage2/rfSizeEnd").toInt();

    m_XStep = config.value("stage2/xStep").toInt();
    m_YStep = config.value("stage2/yStep").toInt();
    m_RFSizeStep = config.value("stage2/rfSizeStep").toInt();
}

const QString& PrfStage2Job::error() const {
    return m_Error;
}

QString PrfStage2Job::status() const {
    if (!m_Error.isNull()) return m_Error;

    if (isRunning()) return "Running...";

    if (isFinished()) return "Finished.";

    return "Pending...";
}

QString PrfStage2Job::parameters() const {
    return QString("inputFileName: %1, variableSpec: %2, outputFileName: %3, ").arg(m_InputFileName, m_VariableSpec, m_OutputFileName) +
        QString("xStart: %1, xEnd: %2, xStep: %3, ").arg(m_XStart).arg(m_XEnd).arg(m_XStep) +
        QString("yStart: %1, yEnd: %2, yStep: %3, ").arg(m_YStart).arg(m_YEnd).arg(m_YStep) +
        QString("rfsizeStart: %1, rfsizeEnd: %2, rfsizeStep: %3").arg(m_RFSizeStart).arg(m_RFSizeEnd).arg(m_RFSizeStep);
}

static void mat_deleter(mat_t *mat) {
    Mat_Close(mat);
}

static void var_deleter(matvar_t *var) {
    Mat_VarFree(var);
}

static void CL_CALLBACK cl_callback(const char *errinfo, const void *priv_info, size_t, void *user_data) {
    qDebug() << errinfo;
}

static void context_deleter(cl_context ctx) {
    clReleaseContext(ctx);
}

static void program_deleter(cl_program prog) {
    clReleaseProgram(prog);
}

static void buffer_deleter(cl_mem buf) {
    clReleaseMemObject(buf);
}

static void queue_deleter(cl_command_queue queue) {
    clReleaseCommandQueue(queue);
}

static void kernel_deleter(cl_kernel kernel) {
    clReleaseKernel(kernel);
}

void PrfStage2Job::run() {
    emit jobStarted(this);

    qDebug() << "Loading MAT-File...";

    QSharedPointer<mat_t> mat(Mat_Open(m_InputFileName.toLatin1(), MAT_ACC_RDONLY), mat_deleter);
    if (mat.isNull()) {
        setError("Failed to open MAT-File");
        return;
    }

    matvar_t *var;
    matvar_t *_topvar = NULL;
    var = QMatFileModel::resolveVariableSpec(mat.data(), m_VariableSpec, &_topvar);
    QSharedPointer<matvar_t> topvar(_topvar, var_deleter);
    Q_UNUSED(topvar);
    if (var == NULL) {
        setError("Failed to resolve specified variable");
        return;
    }

    if (!QMatFileModel::isFullReal(var)) {
        setError("Variable is not of the expected numeric type");
        return;
    }

    if (var->rank != 3) {
        setError(QString("Variable rank is different than expacted. Is: %1. Expected: %2.").arg(var->rank).arg(3));
        return;
    }

    int H = var->dims[0];
    int W = var->dims[1];
    int TOT_DUR = var->dims[2];

    qDebug() << "H:" << H << "W:" << W << "TOT_DUR:" << TOT_DUR;

    qDebug() << "Reading data...";
    Mat_VarReadDataAll(mat.data(), var);

    qDebug() << "Converting data to row-major float32 ...";
    // int nelem = H * W * TOT_DUR;
    QVector<float> stimulus(H * W * TOT_DUR);
    QSharedPointer<TypeCastArrayBase> castAry(TypeCastArrayFactory::create(var->class_type, var->data));

    int ofs = 0;
    for (int t = 0; t < TOT_DUR; t++) {
        for (int x = 0; x < W; x++) {
            for (int y = 0; y < H; y++) {
                stimulus[(t * H + y) * W + x] = castAry->asFloat(ofs);
                ofs++;
            }
        }
    }
    /* for (int i = 0; i < nelem; i++) {
        stimulus[i] = castAry->asFloat(i);
    } */


    qDebug() << "Enumerating devices...";

    cl_uint nplatforms;
    clGetPlatformIDs(0, NULL, &nplatforms);

    QVector<cl_platform_id> platforms(nplatforms);
    clGetPlatformIDs(platforms.size(), &platforms[0], &nplatforms);

    QVector<cl_device_id> all_devs;

    for (int i = 0; i < platforms.size(); i++) {
        cl_uint ndevices;
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &ndevices);

        QVector<cl_device_id> devices(ndevices);
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, ndevices, &devices[0], &ndevices);

        all_devs += devices;
    }

    QSettings settings("algoholic.eu", "qprf");
    int device_idx = settings.value("opencl/device", 0).toInt();
    if (device_idx >= all_devs.size()) {
        setError("Specified OpenCL device couldn't be found.");
        return;
    }

    cl_device_id dev_id = all_devs[device_idx];

    int ntasks = settings.value("opencl/tasks", 512).toInt();
    qDebug() << "ntasks:" << ntasks;

    cl_int err;
    QSharedPointer<struct _cl_context> ctx(clCreateContext(NULL, 1, &dev_id, cl_callback, NULL, &err), context_deleter);
    if (err != CL_SUCCESS) {
        setError("Error creating OpenCL context");
        return;
    }

    qDebug() << "OpenCL context created";
    // QSharedPointer<struct _cl_context> ctx(_ctx, ctx_deleter);

    QVector<float> xs;
    QVector<float> ys;
    QVector<float> rfsizes;

    for (int x = m_XStart; x < m_XEnd; x += m_XStep) {
        for (int y = m_YStart; y < m_YEnd; y += m_YStep) {
            for (int rfsize = m_RFSizeStart; rfsize < m_RFSizeEnd; rfsize += m_RFSizeStep) {
                xs.append(x);
                ys.append(y);
                rfsizes.append(rfsize);
            }
        }
    }

    qDebug() << "Work list size:" << xs.size();

    QString hdr = QString(OPENCL_HDR).arg(H).arg(W).arg(TOT_DUR);
    qDebug() << "hdr:" << hdr;
    QString prog = hdr + "\n\n" + OPENCL_BODY;
    QStringList lines = prog.split('\n');
    QVector<QByteArray> text1;
    QVector<char*> text2;
    for (int i = 0; i < lines.size(); i++) {
        text1.append(lines.at(i).toLatin1());
        text2.append(text1.last().data());
        // qDebug() << text2.last();
    }

    //qDebug() << "prog:" << prog;
    qDebug() << "text2.size():" << text2.size();

    QByteArray ba = prog.toLatin1();
    char *ba2 = ba.data();

    QSharedPointer<struct _cl_program> program(clCreateProgramWithSource(ctx.data(), 1, (const char**) &ba2, NULL, &err), program_deleter);
    if (err != CL_SUCCESS) {
        setError("Failed to create program from source");
        return;
    }

    if (program.isNull()) qDebug() << "Shouldn't happen!";

    /*err = clCompileProgram(program, 1, &dev_id, "", 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        setError("Failed to compile program");
        return;
    }

    qDebug() << "Program compiled successfully."; */

    err = clBuildProgram(program.data(), 1, &dev_id, "-Werror -cl-std=CL1.1", NULL, NULL);
    if (err != CL_SUCCESS) {
        setError("Failed to build program.");
        size_t sz;
        clGetProgramBuildInfo(program.data(), dev_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &sz);
        QByteArray log(sz + 1, 0);
        clGetProgramBuildInfo(program.data(), dev_id, CL_PROGRAM_BUILD_LOG, sz, log.data(), &sz);
        qDebug() << QString::fromLatin1(log);
        return;
    }

    qDebug() << "Program built successfully.";

    QSharedPointer<struct _cl_mem> g_gaussians(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * W * H * ntasks, NULL, &err), buffer_deleter);
    if (err != CL_SUCCESS) {
        setError("Failed to allocate buffer for gaussians.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_xs(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, &err), buffer_deleter);
    if (err != CL_SUCCESS) {
        setError("Failed to allocate buffer for xs.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_ys(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, &err), buffer_deleter);
    if (err != CL_SUCCESS) {
        setError("Failed to allocate buffer for ys.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_rfsizes(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * ntasks, NULL, &err), buffer_deleter);
    if (err != CL_SUCCESS) {
        setError("Failed to allocate buffer for rfsizes.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_reduced(clCreateBuffer(ctx.data(), CL_MEM_WRITE_ONLY, sizeof(float) * TOT_DUR * ntasks, NULL, &err), buffer_deleter);
    if (err != CL_SUCCESS) {
        setError("Failed to allocate buffer for reduced.");
        return;
    }

    QSharedPointer<struct _cl_mem> g_stim(clCreateBuffer(ctx.data(), CL_MEM_READ_ONLY, sizeof(float) * W * H * TOT_DUR, NULL, &err), buffer_deleter);
    if (err != CL_SUCCESS) {
        setError("Failed to allocate buffer for stim.");
        return;
    }

    QSharedPointer<struct _cl_command_queue> queue(clCreateCommandQueue(ctx.data(), dev_id, 0, &err), queue_deleter);
    if (err != CL_SUCCESS) {
        setError("Failed to create command queue.");
        return;
    }

    QSharedPointer<struct _cl_kernel> make_gaussians_no_limit(clCreateKernel(program.data(), "make_gaussians_no_limit", &err), kernel_deleter);
    if (err != CL_SUCCESS) {
        setError("Failed to create make_gaussians_no_limit() kernel.");
        return;
    }

    cl_mem tmp;
    if ((err = clSetKernelArg(make_gaussians_no_limit.data(), 0, sizeof(cl_mem), &(tmp = g_xs.data()))) != CL_SUCCESS) {
        setError(QString("clSetKernelArg() failed for g_xs: %1").arg(err));
        return;
    }
    clSetKernelArg(make_gaussians_no_limit.data(), 1, sizeof(cl_mem), &(tmp = g_ys.data()));
    clSetKernelArg(make_gaussians_no_limit.data(), 2, sizeof(cl_mem), &(tmp = g_rfsizes.data()));
    clSetKernelArg(make_gaussians_no_limit.data(), 3, sizeof(cl_mem), &(tmp = g_gaussians.data()));

    QSharedPointer<struct _cl_kernel> stim_times_gaussians(clCreateKernel(program.data(), "stim_times_gaussians", &err), kernel_deleter);
    if (err != CL_SUCCESS) {
        setError(QString("Failed to create stim_times_gaussians() kernel: %1.").arg(err));
        return;
    }

    clSetKernelArg(stim_times_gaussians.data(), 0, sizeof(cl_mem), &(tmp = g_stim.data()));
    clSetKernelArg(stim_times_gaussians.data(), 1, sizeof(cl_mem), &(tmp = g_gaussians.data()));
    clSetKernelArg(stim_times_gaussians.data(), 2, sizeof(cl_mem), &(tmp = g_reduced.data()));
    // clSetKernelArg(stim_times_gaussians.data(), 3, sizeof(int), i);

    QVector<float> reduced(TOT_DUR * xs.size());

    clEnqueueWriteBuffer(queue.data(), g_stim.data(), false, 0, sizeof(float) * W * H * TOT_DUR, &stimulus[0], 0, NULL, NULL);

    // QVector<float> gaussians(xs.size() * W * H);

    for (int i = 0; i < xs.size(); i += ntasks) {
        emit progressChanged(this, QString("i: %1").arg(i));

        int n = qMin(ntasks, xs.size() - i);
        qDebug() << "i:" << i << "n:" << n;
        clEnqueueWriteBuffer(queue.data(), g_xs.data(), false, 0, sizeof(float) * n, &xs[i], 0, NULL, NULL);
        clEnqueueWriteBuffer(queue.data(), g_ys.data(), false, 0, sizeof(float) * n, &ys[i], 0, NULL, NULL);
        clEnqueueWriteBuffer(queue.data(), g_rfsizes.data(), false, 0, sizeof(float) * n, &rfsizes[i], 0, NULL, NULL);
        clEnqueueBarrier(queue.data());
        size_t global_work_size = n;
        size_t local_work_size = 1;

        if (clEnqueueNDRangeKernel(queue.data(), make_gaussians_no_limit.data(), 1, NULL, &global_work_size, NULL, 0, NULL, NULL) != CL_SUCCESS) {
            setError("clEnqueueNDRangeKernel() failed for make_gaussians_no_limit()");
            return;
        }
        clEnqueueBarrier(queue.data());

        for (int t = 0; t < TOT_DUR; t++) {
            if (isInterruptionRequested()) {
                qDebug() << "Interrupted!";
                //emit statusChanged(this, "Interrupted");
                emit jobInterrupted(this);
                return;
            }

            // qDebug() << "t:" << t;
            clSetKernelArg(stim_times_gaussians.data(), 3, sizeof(int), &t);

            if (clEnqueueNDRangeKernel(queue.data(), stim_times_gaussians.data(), 1, NULL, &global_work_size, NULL, 0, NULL, NULL) != CL_SUCCESS) {
                setError("clEnqueueNDRangeKernel() failed for stim_times_gaussians()");
                return;
            }
        }
        clEnqueueBarrier(queue.data());

        cl_event ev;
        /* clEnqueueReadBuffer(queue.data(), g_gaussians.data(), false, 0, sizeof(float) * W * H * n, &gaussians[W * H * i], 0, NULL, &ev);
        clWaitForEvents(1, &ev);
        clReleaseEvent(ev); */


        clEnqueueReadBuffer(queue.data(), g_reduced.data(), false, 0, sizeof(float) * TOT_DUR * n, &reduced[i * TOT_DUR], 0, NULL, &ev);
        clWaitForEvents(1, &ev);
        clReleaseEvent(ev);

        qDebug() << "i:" << i << "done.";
    }

    qDebug() << "All done.";

    QFileInfo fi(m_OutputFileName);
    if (!fi.isAbsolute()) {
        fi = QFileInfo(m_InputFileName).absoluteDir().filePath(m_OutputFileName);
    }
    QString outname = fi.absoluteFilePath();

    qDebug() << "outname:" << outname;

    QSharedPointer<mat_t> outmat(Mat_CreateVer(outname.toLatin1(), NULL, MAT_FT_MAT5), mat_deleter);
    if (outmat.isNull()) {
        setError("Failed to create output MAT file.");
        return;
    }

    {
        size_t dims[2] = {TOT_DUR, xs.size()};

        for (int i = 0; i < reduced.size(); i++)
            if (std::isnan(reduced[i])) reduced[i] = 0;

        QSharedPointer<matvar_t> mat_reduced(Mat_VarCreate("reduced", MAT_C_SINGLE, MAT_T_SINGLE, 2, &dims[0], &reduced[0], MAT_F_GLOBAL), var_deleter);
        if (mat_reduced.isNull()) {
            setError("Failed to create reduced variable.");
            return;
        }

        if (Mat_VarWrite(outmat.data(), mat_reduced.data(), MAT_COMPRESSION_ZLIB) != 0) {
            setError("Failed to write output data.");
            return;
        }
    }

    {
        size_t dims[2] = {1, 3};
        float x_range[3] = {m_XStart, m_XEnd, m_XStep};
        QSharedPointer<matvar_t> mat_x_range(Mat_VarCreate("x_range", MAT_C_SINGLE, MAT_T_SINGLE, 2, &dims[0], &x_range[0], MAT_F_GLOBAL), var_deleter);
        if (mat_x_range.isNull()) {
            setError("Failed to create x_range variable.");
            return;
        }

        if (Mat_VarWrite(outmat.data(), mat_x_range.data(), MAT_COMPRESSION_ZLIB) != 0) {
            setError("Failed to write output data.");
            return;
        }
    }

    {
        size_t dims[2] = {1, 3};
        float y_range[3] = {m_YStart, m_YEnd, m_YStep};
        QSharedPointer<matvar_t> mat_y_range(Mat_VarCreate("y_range", MAT_C_SINGLE, MAT_T_SINGLE, 2, &dims[0], &y_range[0], MAT_F_GLOBAL), var_deleter);
        if (mat_y_range.isNull()) {
            setError("Failed to create y_range variable.");
            return;
        }

        if (Mat_VarWrite(outmat.data(), mat_y_range.data(), MAT_COMPRESSION_ZLIB) != 0) {
            setError("Failed to write output data.");
            return;
        }
    }

    {
        size_t dims[2] = {1, 3};
        float rfsize_range[3] = {m_RFSizeStart, m_RFSizeEnd, m_RFSizeStep};
        QSharedPointer<matvar_t> mat_rfsize_range(Mat_VarCreate("rfsize_range", MAT_C_SINGLE, MAT_T_SINGLE, 2, &dims[0], &rfsize_range[0], MAT_F_GLOBAL), var_deleter);
        if (mat_rfsize_range.isNull()) {
            setError("Failed to create rfsize_range variable.");
            return;
        }

        if (Mat_VarWrite(outmat.data(), mat_rfsize_range.data(), MAT_COMPRESSION_ZLIB) != 0) {
            setError("Failed to write output data.");
            return;
        }
    }

    {
        size_t dims[2] = {1, 1};
        QSharedPointer<matvar_t> mat_W(Mat_VarCreate("W", MAT_C_SINGLE, MAT_T_SINGLE, 2, &dims[0], &W, MAT_F_GLOBAL), var_deleter);
        if (mat_W.isNull()) {
            setError("Failed to create W variable.");
            return;
        }

        if (Mat_VarWrite(outmat.data(), mat_W.data(), MAT_COMPRESSION_ZLIB) != 0) {
            setError("Failed to write output data.");
            return;
        }
    }

    {
        size_t dims[2] = {1, 1};
        QSharedPointer<matvar_t> mat_H(Mat_VarCreate("H", MAT_C_SINGLE, MAT_T_SINGLE, 2, &dims[0], &W, MAT_F_GLOBAL), var_deleter);
        if (mat_H.isNull()) {
            setError("Failed to create H variable.");
            return;
        }

        if (Mat_VarWrite(outmat.data(), mat_H.data(), MAT_COMPRESSION_ZLIB) != 0) {
            setError("Failed to write output data.");
            return;
        }
    }

    {
        size_t dims[2] = {xs.size(), 1};
        QSharedPointer<matvar_t> mat_xs(Mat_VarCreate("xs", MAT_C_SINGLE, MAT_T_SINGLE, 2, &dims[0], &xs[0], MAT_F_GLOBAL));
        if (mat_xs.isNull()) {
            setError("Failed to create mat_xs variable.");
            return;
        }
        if (Mat_VarWrite(outmat.data(), mat_xs.data(), MAT_COMPRESSION_ZLIB) != 0) {
            setError("Failed to write mat_xs variable.");
            return;
        }
    }

    {
        size_t dims[2] = {ys.size(), 1};
        QSharedPointer<matvar_t> mat_ys(Mat_VarCreate("ys", MAT_C_SINGLE, MAT_T_SINGLE, 2, &dims[0], &ys[0], MAT_F_GLOBAL));
        if (mat_ys.isNull()) {
            setError("Failed to create mat_ys variable.");
            return;
        }
        if (Mat_VarWrite(outmat.data(), mat_ys.data(), MAT_COMPRESSION_ZLIB) != 0) {
            setError("Failed to write mat_ys variable.");
            return;
        }
    }

    {
        size_t dims[2] = {rfsizes.size(), 1};
        QSharedPointer<matvar_t> mat_rfsizes(Mat_VarCreate("rfsizes", MAT_C_SINGLE, MAT_T_SINGLE, 2, &dims[0], &rfsizes[0], MAT_F_GLOBAL));
        if (mat_rfsizes.isNull()) {
            setError("Failed to create mat_rfsizes variable.");
            return;
        }
        if (Mat_VarWrite(outmat.data(), mat_rfsizes.data(), MAT_COMPRESSION_ZLIB) != 0) {
            setError("Failed to write mat_rfsizes variable.");
            return;
        }
    }

    {
        QVector<float> gaussians(H * W * ntasks);

        cl_event ev;
        clEnqueueReadBuffer(queue.data(), g_gaussians.data(), false, 0, sizeof(float) * H * W * ntasks, &gaussians[0], 0, NULL, &ev);
        clWaitForEvents(1, &ev);
        clReleaseEvent(ev);

        size_t dims[3] = {W, H, ntasks}; // xs.size()};
        matvar_t *var = Mat_VarCreate("gaussians", MAT_C_SINGLE, MAT_T_SINGLE, 3, dims, &gaussians[0], MAT_F_GLOBAL);
        Mat_VarWrite(outmat.data(), var, MAT_COMPRESSION_ZLIB);
        Mat_VarFree(var);
    }

    {
        size_t dims[3] = {W, H, TOT_DUR};
        matvar_t *var = Mat_VarCreate("stimulus", MAT_C_SINGLE, MAT_T_SINGLE, 3, dims, &stimulus[0], MAT_F_GLOBAL);
        Mat_VarWrite(outmat.data(), var, MAT_COMPRESSION_ZLIB);
        Mat_VarFree(var);
    }


    qDebug() << "Output saved.";

    emit jobFinished(this);
}

void PrfStage2Job::setError(const QString &err) {
    qDebug() << err;
    m_Error = err;
    // emit jobDataChanged(this);
    emit statusChanged(this, err);
}
