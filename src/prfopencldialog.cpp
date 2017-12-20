//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "prfopencldialog.h"
#include "ui_prfopencldialog.h"

#include <QVector>
#include <QDebug>
#include <QSettings>

#include <OpenCL/opencl.h>


PrfOpenCLDialog::PrfOpenCLDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrfOpenCLDialog)
{
    ui->setupUi(this);

    cl_uint nplatforms;
    clGetPlatformIDs(0, NULL, &nplatforms);

    QVector<cl_platform_id> platforms(nplatforms);
    clGetPlatformIDs(platforms.size(), &platforms[0], &nplatforms);

    for (int i = 0; i < platforms.size(); i++) {
        size_t sz;
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, NULL, &sz);

        QByteArray name(sz + 1, 0);
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sz, name.data(), &sz);

        qDebug() << "name:" << QString::fromLatin1(name);
        //qDebug() << (int) name[name.length() - 1];

        cl_uint ndevices;
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &ndevices);

        QVector<cl_device_id> devices(ndevices);
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, ndevices, &devices[0], &ndevices);

        for (int k = 0; k < ndevices; k++) {
            clGetDeviceInfo(devices[k], CL_DEVICE_NAME, 0, NULL, &sz);
            QByteArray device_name(sz + 1, 0);
            clGetDeviceInfo(devices[k], CL_DEVICE_NAME, sz, device_name.data(), &sz);

            // qDebug() << (int) device_name[device_name.length() - 1];

            qDebug() << "device_name:" << QString::fromLatin1(device_name) << " ";

            clGetDeviceInfo(devices[k], CL_DEVICE_EXTENSIONS, 0, NULL, &sz);
            QByteArray extensions(sz + 1, 0);
            clGetDeviceInfo(devices[k], CL_DEVICE_EXTENSIONS, sz, extensions.data(), &sz);

            qDebug() << "extensions:" << QString::fromLatin1(extensions);

            ui->device_comboBox->addItem(QString("%1 %2")
                .arg(QString::fromLatin1(name))
                .arg(QString::fromLatin1(device_name)));

            cl_ulong local_mem_size;
            clGetDeviceInfo(devices[k], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &local_mem_size, 0);
            qDebug() << "local_mem_size:" << local_mem_size;

            char dev_ver[128];
            sz = 127;
            clGetDeviceInfo(devices[k], CL_DEVICE_VERSION, sz, &dev_ver[0], &sz);
            qDebug() << "dev_ver:" << dev_ver;
        }
    }

    QSettings settings("algoholic.eu", "qprf");
    bool ok;
    ui->device_comboBox->setCurrentIndex(settings.value("opencl/device", 0).toInt(&ok));

    ui->tasks_lineEdit->setText(QString("%1").arg(settings.value("opencl/tasks", 512).toInt(&ok)));

    connect(this, SIGNAL(accepted()), this, SLOT(onSaveOpenCLSettings()));

}

PrfOpenCLDialog::~PrfOpenCLDialog()
{
    delete ui;
}

void PrfOpenCLDialog::onSaveOpenCLSettings() {
    QSettings settings("algoholic.eu", "qprf");
    settings.setValue("opencl/device", ui->device_comboBox->currentIndex());
    bool ok;
    settings.setValue("opencl/tasks", ui->tasks_lineEdit->text().toInt(&ok));
}
