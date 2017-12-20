#ifndef QMATARRAYMODEL_H
#define QMATARRAYMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QVector>

#include "matio.h"
#include "qmatnavigable.h"

class QMatArrayModel : public QAbstractItemModel,
    public QMatNavigable
{
public:
    QMatArrayModel(QSharedPointer<mat_t>, QSharedPointer<matvar_t>);
    QMatArrayModel(QSharedPointer<mat_t>, QSharedPointer<matvar_t>, const QVector<size_t>&);
    ~QMatArrayModel();

    // void init();

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    QVariant data(const QModelIndex &index, int role) const;


    QAbstractItemModel* navigateInto(const QModelIndex&);
    bool equals(QMatNavigable *other);
    QString name();
    void setParentName(const QString&);
    QString childName(const QModelIndex&);

    QString name(const QVector<size_t>&) const;

private:
    QSharedPointer<mat_t> m_Mat;
    QSharedPointer<matvar_t> m_Var;
    QVector<size_t> m_Indices;
    QString m_ParentName;

#if 0
    mutable QSharedPointer<char> m_Cache;
    mutable int m_CacheStartRow;
    mutable int m_CacheEndRow;
    mutable int m_CacheStartCol;
    mutable int m_CacheEndCol;
    // int m_CacheSize;
    int m_ElemSize;
#endif
};

#endif // QMATFILEMODEL_H
