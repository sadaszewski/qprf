//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFSTAGE2JOB_H
#define PRFSTAGE2JOB_H

#include <QObject>
#include <QThread>
#include <QJsonDocument>

#include "prfjob.h"

class PrfStage2Job : public PrfJob
{
    Q_OBJECT
public:
    PrfStage2Job();

    void setInputFileName(const QString&);
    void setVariableSpec(const QString&);
    void setOutputFileName(const QString&);

    void setXStart(int);
    void setXEnd(int);
    void setXStep(int);

    void setYStart(int);
    void setYEnd(int);
    void setYStep(int);

    void setRFSizeStart(int);
    void setRFSizeEnd(int);
    void setRFSizeStep(int);

    void saveConfig(const QString&);
    void loadConfig(const QString&);

    QJsonDocument toJson() const;
    bool fromJson(const QJsonDocument&);

    const QString& error() const;
    QString status() const;
    QString parameters() const;

protected:
    void run();

private:
    void setError(const QString&);


private:
    QString m_InputFileName;
    QString m_VariableSpec;
    QString m_OutputFileName;

    int m_XStart;
    int m_XEnd;
    int m_XStep;

    int m_YStart;
    int m_YEnd;
    int m_YStep;

    int m_RFSizeStart;
    int m_RFSizeEnd;
    int m_RFSizeStep;

    QString m_Error;
};

#endif // PRFSTAGE2JOB_H
