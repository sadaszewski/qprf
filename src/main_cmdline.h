#ifndef _MAIN_CMDLINE_H_
#define _MAIN_CMDLINE_H_

#include <QObject>

class PrfJob;

class MainCmdLine: public QObject {
    Q_OBJECT

public:
    MainCmdLine();
    int process();

public slots:
    void jobStatusChanged(PrfJob*, const QString&);
    void jobProgressChanged(PrfJob*, const QString&);
};

#endif
