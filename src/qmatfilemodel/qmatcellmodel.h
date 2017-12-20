#ifndef QMATCELLMODEL_H
#define QMATCELLMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QSharedPointer>

#include "matio.h"
#include "qmatnavigable.h"

class QMatCellModel : public QAbstractItemModel,
    public QMatNavigable
{
public:
    QMatCellModel(QSharedPointer<mat_t>, QSharedPointer<matvar_t>);
    QMatCellModel(QSharedPointer<mat_t>, QSharedPointer<matvar_t>, const QVector<size_t>&);
    ~QMatCellModel();

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

    QString name(const QVector<size_t>&) const;

private:
    QSharedPointer<mat_t> m_Mat;
    QSharedPointer<matvar_t> m_Var;
    QString m_ParentName;
    QVector<size_t> m_Indices;
};

#endif // QMATFILEMODEL_H
