#ifndef QMATFIELDMODEL_H
#define QMATFIELDMODEL_H

#include <QObject>
#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QVector>

#include "matio.h"
#include "qmatnavigable.h"

class QMatFieldModel : public QAbstractItemModel,
    public QMatNavigable
{
public:
    QMatFieldModel(QSharedPointer<mat_t>, QSharedPointer<matvar_t>, int, const QString&);
    QMatFieldModel(QSharedPointer<mat_t>, QSharedPointer<matvar_t>, int, const QString&, const QVector<size_t>&);
    ~QMatFieldModel();

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
    int m_FieldIndex;
    QVector<size_t> m_Indices;
    QString m_ParentName;
    QString m_FieldName;
};

#endif // QMATFILEMODEL_H
