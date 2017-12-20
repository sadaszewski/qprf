//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "prfcommon.h"

#include <QSettings>
#include <QDebug>

PrfCommon::PrfCommon()
{

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

static void CL_CALLBACK cl_callback(const char *errinfo, const void *priv_info, size_t, void *user_data) {
    qDebug() << errinfo;
}

QSharedPointer<struct _cl_context> PrfCommon::create_cl_context(cl_device_id &dev_id, QString &error) {

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
        error = "Specified OpenCL device couldn't be found.";
        return QSharedPointer<struct _cl_context>();
    }

    // cl_device_id
    dev_id = all_devs[device_idx];

    // int ntasks = settings.value("opencl/tasks", 512).toInt();
    // qDebug() << "ntasks:" << ntasks;

    cl_int err;
    QSharedPointer<struct _cl_context> ctx(clCreateContext(NULL, 1, &dev_id, cl_callback, NULL, &err), context_deleter);
    if (err != CL_SUCCESS) {
        error = "Error creating OpenCL context";
        return QSharedPointer<struct _cl_context>();
    }

    qDebug() << "OpenCL context created";

    return ctx;
}

int PrfCommon::get_ntasks() {
    QSettings settings("algoholic.eu", "qprf");

    int ntasks = settings.value("opencl/tasks", 512).toInt();
    qDebug() << "ntasks:" << ntasks;
    return ntasks;
}

QSharedPointer<struct _cl_program> PrfCommon::wrap_program(cl_program prog) {
    return QSharedPointer<struct _cl_program>(prog, program_deleter);
}

QSharedPointer<struct _cl_mem> PrfCommon::wrap_buffer(cl_mem buf) {
    return QSharedPointer<struct _cl_mem>(buf, buffer_deleter);
}

QSharedPointer<struct _cl_command_queue> PrfCommon::wrap_queue(cl_command_queue queue) {
    return QSharedPointer<struct _cl_command_queue>(queue, queue_deleter);
}

QSharedPointer<struct _cl_kernel> wrap_kernel(cl_kernel kernel) {
    return QSharedPointer<struct _cl_kernel>(kernel, kernel_deleter);
}
