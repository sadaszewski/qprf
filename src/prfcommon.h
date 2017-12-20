//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFCOMMON_H
#define PRFCOMMON_H

#include <OpenCL/opencl.h>
#include <QSharedPointer>

class PrfCommon
{
public:
    static QSharedPointer<struct _cl_context> create_cl_context(cl_device_id&, QString&);
    static int get_ntasks();

    static QSharedPointer<struct _cl_program> wrap_program(cl_program);
    static QSharedPointer<struct _cl_mem> wrap_buffer(cl_mem);
    static QSharedPointer<struct _cl_command_queue> wrap_queue(cl_command_queue);
    static QSharedPointer<struct _cl_kernel> wrap_kernel(cl_kernel);

    PrfCommon();
};


#endif // PRFCOMMON_H
