#ifndef QMATFILEMODEL_H
#define QMATFILEMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QVector>

#include "matio.h"
#include "qmatnavigable.h"

class QMatFileModel : public QAbstractItemModel,
    public QMatNavigable
{
public:
    static QString makeTypeDesc(matvar_t*);
    static size_t linear_index(matvar_t *var,
        const QVector<size_t> &indices);
    static QAbstractItemModel* modelForVar(QSharedPointer<mat_t>, QSharedPointer<matvar_t>);
    static matvar_t* resolveVariableSpec(mat_t *mat, const QString &expr, matvar_t**);
    static bool isFullReal(matvar_t *var);

public:
    QMatFileModel(QSharedPointer<mat_t>);
    ~QMatFileModel();

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    QVariant data(const QModelIndex &index, int role) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QAbstractItemModel* navigateInto(const QModelIndex&);
    bool equals(QMatNavigable *other);
    QString name();
    void setParentName(const QString&);
    QString childName(const QModelIndex &);

private:
    QSharedPointer<mat_t> m_Mat;
    QVector<QSharedPointer<matvar_t> > m_Vars;
};

#endif // QMATFILEMODEL_H
