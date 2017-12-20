#ifndef QMATSTRUCTMODEL_H
#define QMATSTRUCTMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QVector>

#include "matio.h"
#include "qmatnavigable.h"

class QMatStructModel : public QAbstractItemModel,
    public QMatNavigable
{
public:
    QMatStructModel(QSharedPointer<mat_t>, QSharedPointer<matvar_t>);
    ~QMatStructModel();

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
    QSharedPointer<matvar_t> m_Var;
    QVector<QString> m_FieldNames;
    QString m_ParentName;
};

#endif // QMATFILEMODEL_H
