#ifndef QMATCHARMODEL_H
#define QMATCHARMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QVector>

#include "matio.h"
#include "qmatnavigable.h"

class QMatCharModel : public QAbstractItemModel,
    public QMatNavigable
{
public:
    QMatCharModel(QSharedPointer<mat_t>, QSharedPointer<matvar_t>);
    QMatCharModel(QSharedPointer<mat_t>, QSharedPointer<matvar_t>, const QVector<size_t>&);
    ~QMatCharModel();

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    QVariant data(const QModelIndex &index, int role) const;

    QAbstractItemModel* navigateInto(const QModelIndex&);
    bool equals(QMatNavigable *other);
    QString name();
    void setParentName(const QString&);
    QString childName(const QModelIndex &);

    QString name(const QVector<size_t> &indices) const;

private:
    QSharedPointer<mat_t> m_Mat;
    QSharedPointer<matvar_t> m_Var;
    QString m_ParentName;
    QVector<size_t> m_Indices;
};

#endif // QMATFILEMODEL_H
