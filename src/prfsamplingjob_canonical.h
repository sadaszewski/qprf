//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFSAMPLINGJOB_CANONICAL_H
#define PRFSAMPLINGJOB_CANONICAL_H

#include "prfjob.h"

struct _mat_t;

typedef struct _mat_t mat_t;


class PrfSamplingJobCanonical : public PrfJob
{
    Q_OBJECT
public:
    PrfSamplingJobCanonical();

    const QString& error() const;
    QString status() const;
    QString parameters() const;

    void set_pRF_Model(int);
    void setWriteDebug(bool);
    void setDataFileName(const QString&);
    void setDataVariableSpec(const QString&);
    void setStage2FileName(const QString&);
    void setNumberOfSteps(int);
    void setTemperature(float);

    void setXStdev(float);
    void setYStdev(float);
    void setRFSizeStdev(float);
    void setExptStdev(float);
    void setGainStdev(float);

    // void setSubdiv(int);
    // void setTR(float);

    void setMaxNoiseStdev(float);

    void saveConfig(const QString&);
    void loadConfig(const QString&);

    QJsonDocument toJson() const;
    bool fromJson(const QJsonDocument &);

protected:
    void run();

private:
    void setError(const QString&);

    int read_range_var(mat_t*, const char*, int*);
    int read_single(mat_t*, const char*, float*);
    int write_var(mat_t*, const char*, float*, int dim0, int dim1);
    int write_cell_vector(mat_t*, const char*, const QVector<QVector<float> >&);
    int save_samples(const char *, const QVector<QVector<float> >&);

private:
    QString m_Error;
    QString m_DataFileName;
    QString m_DataVariableSpec;
    QString m_Stage2FileName;
    int m_NumberOfSteps;
    float m_Temperature;

    float m_XStdev;
    float m_YStdev;
    float m_RFSizeStdev;
    float m_ExptStdev;
    float m_GainStdev;

    float m_XMean;
    float m_YMean;
    float m_RFSizeMean;

    float m_MaxNoiseStdev;

    // int m_SubDiv;

    // float m_TR;

    int m_pRF_Model;
    bool m_WriteDebug;

};

#endif // PRFSAMPLINGJOB_CANONICAL_H
