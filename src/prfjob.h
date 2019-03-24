//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef PRFJOB_H
#define PRFJOB_H

#include <QThread>
#include <QJsonDocument>

class QString;

class PrfJob: public QThread {
    Q_OBJECT
public:
    virtual ~PrfJob() {}
    virtual const QString& error() const = 0;
    virtual QString status() const = 0;
    virtual QString parameters() const = 0;
    virtual QJsonDocument toJson() const = 0;
    virtual bool fromJson(const QJsonDocument&) = 0;

signals:
    void jobStarted(PrfJob*);
    void jobFinished(PrfJob*);
    void jobDataChanged(PrfJob*);
    void statusChanged(PrfJob*, const QString&);
    void progressChanged(PrfJob*, const QString&);
    void jobInterrupted(PrfJob*);
};

#endif // PRFJOB_H

