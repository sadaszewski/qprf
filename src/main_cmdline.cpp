#include <iostream>

using namespace std;

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include "prfjob.h"
#include "prfstage2job.h"
#include "prfsamplingjob.h"
#include "prfsamplingjob_3param.h"
#include "prfsamplingjob_canonical.h"
#include <OpenCL/opencl.h>
#include <QSettings>

#include "main_cmdline.h"

void populate_parser(QCommandLineParser &parser) {
    parser.addOption(QCommandLineOption(QStringList() << "r" << "run-job",
        "Runs a job according to provided JSON job specification file",
        "file"));
    parser.addOption(QCommandLineOption(QStringList() << "p" << "print-devices",
        "Print out OpenCL devices"));
    parser.addOption(QCommandLineOption(QStringList() << "d" << "use-device",
        "Specifies desired OpenCL device by index. Use --print-devices to see the list.",
        "device"));
    parser.addOption(QCommandLineOption(QStringList() << "t" << "num-tasks",
        "Specifies desired number of parallel tasks",
        "ntasks"));
    parser.addHelpOption();
}

void print_devices() {
    cl_uint nplatforms;
    clGetPlatformIDs(0, NULL, &nplatforms);

    QVector<cl_platform_id> platforms(nplatforms);
    clGetPlatformIDs(platforms.size(), &platforms[0], &nplatforms);

    int cnt = 0;

    for (int i = 0; i < platforms.size(); i++) {
        size_t sz;
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, NULL, &sz);

        QByteArray plat_name(sz + 1, 0);
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sz, plat_name.data(), &sz);

        // qDebug() << "plat_name:" << QString::fromLatin1(plat_name);
        //qDebug() << (int) name[name.length() - 1];

        cl_uint ndevices;
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &ndevices);

        QVector<cl_device_id> devices(ndevices);
        clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, ndevices, &devices[0], &ndevices);

        for (uint k = 0; k < ndevices; k++) {
            cout << "=== Device " << cnt << " ===" << endl;
            cnt++;

            cout << "plat_name: " << QString::fromLatin1(plat_name).toStdString() << endl;

            clGetDeviceInfo(devices[k], CL_DEVICE_NAME, 0, NULL, &sz);
            QByteArray device_name(sz + 1, 0);
            clGetDeviceInfo(devices[k], CL_DEVICE_NAME, sz, device_name.data(), &sz);

            // qDebug() << (int) device_name[device_name.length() - 1];

            qDebug() << "device_name:" << QString::fromLatin1(device_name) << " ";

            clGetDeviceInfo(devices[k], CL_DEVICE_EXTENSIONS, 0, NULL, &sz);
            QByteArray extensions(sz + 1, 0);
            clGetDeviceInfo(devices[k], CL_DEVICE_EXTENSIONS, sz, extensions.data(), &sz);

            qDebug() << "extensions:" << QString::fromLatin1(extensions);

            cl_ulong local_mem_size;
            clGetDeviceInfo(devices[k], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &local_mem_size, 0);
            qDebug() << "local_mem_size:" << local_mem_size;

            char dev_ver[128];
            sz = 127;
            clGetDeviceInfo(devices[k], CL_DEVICE_VERSION, sz, &dev_ver[0], &sz);
            qDebug() << "dev_ver:" << dev_ver;
        }
    }
}

PrfJob* create_job(const QString& filename) {
    QFile  f(filename);
    if (!f.open(QFile::ReadOnly)) {
        cerr << "Cannot open " << filename.toStdString() << endl;
        return nullptr;
    }
    QByteArray data = f.readAll();
    QJsonDocument doc;
    doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        cerr << "Cannot parse " << filename.toStdString() << endl;
        return nullptr;
    }

    if (!doc.isObject()) {
        cerr << "Unexpected top-level value in " <<
            filename.toStdString() << endl;
        return nullptr;
    }

    QJsonObject obj;
    obj = doc.object();

    if (!obj.contains("qprf_job")) {
        cerr << "No job found in " << filename.toStdString() << endl;
        return nullptr;
    }

    QJsonObject job = obj["qprf_job"].toObject();
    QString job_type = job["job_type"].toString();

    PrfJob *res = nullptr;

    if (job_type == "generate_lookup_table") {
        res = new PrfStage2Job();
    } else if (job_type == "perform_sampling_5param") {
        res = new PrfSamplingJob();
    } else if (job_type == "perform_sampling_3param") {
        res = new PrfSamplingJob_3Param();
    } else if (job_type == "perform_sampling_canonical") {
        res = new PrfSamplingJobCanonical();
    } else {
        cerr << "Unsupported job_type: " << job_type.toStdString() <<
            " in " << filename.toStdString() << endl;
        return nullptr;
    }

    res->fromJson(doc);
    return res;
}

MainCmdLine::MainCmdLine() {

}

int MainCmdLine::process() {
    QCommandLineParser parser;
    populate_parser(parser);
    //parser.parse(app.arguments());
    parser.process(*QCoreApplication::instance());
    // qDebug() << app.arguments();

    QSettings settings("algoholic.eu", "qprf");

    if (parser.isSet("use-device")) {
        int dev_idx = parser.value("use-device").toInt();
        cout << "Setting device to: " << dev_idx << endl;
        settings.setValue("opencl/device", dev_idx);
    }

    if (parser.isSet("num-tasks")) {
        int ntasks = parser.value("num-tasks").toInt();
        cout << "Setting ntasks to: " << ntasks << endl;
        settings.setValue("opencl/tasks", ntasks);
    }

    if (parser.isSet("print-devices")) {
        cout << "Printing OpenCL devices ..." << endl;
        print_devices();
        return 0;
    }

    QString jobFilename = parser.value("run-job");
    cerr << "jobFilename: " << jobFilename.toStdString() << endl;
    if (jobFilename.isEmpty()) {
        cerr << "No job specified. Exiting." << endl;
        return -1;
    }

    PrfJob *job = create_job(jobFilename);
    if (job == nullptr) {
        cerr << "Failed to create job. Exiting." << endl;
        return -2;
    }

    cout << "Job created:" << endl;
    cout << "(" << job->metaObject()->className() << ") " << job->parameters().toStdString() << endl;

    QEventLoop loop;

    connect(job, SIGNAL(statusChanged(PrfJob*,QString)), this, SLOT(jobStatusChanged(PrfJob*,QString)));
    connect(job, SIGNAL(progressChanged(PrfJob*,QString)), this, SLOT(jobProgressChanged(PrfJob*,QString)));
    connect(job, SIGNAL(finished()), &loop, SLOT(quit()));

    job->start();
    loop.exec();

    return 0;
}

void MainCmdLine::jobStatusChanged(PrfJob*, const QString &text) {
    cout << text.toStdString() << endl;
}

void MainCmdLine::jobProgressChanged(PrfJob*, const QString &text) {
    cout << text.toStdString() << endl;
}

int main(int argc, char *argv[]) {
    cout << "Welcome to QPrf!" << endl;

    QCoreApplication app(argc, argv);

    MainCmdLine m;
    return m.process();
}
