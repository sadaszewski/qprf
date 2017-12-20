//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#ifndef QMATFILEITEMMODEL2_H
#define QMATFILEITEMMODEL2_H

#include <QObject>
#include <QAbstractItemModel>
#include <QSharedPointer>
#include <QVector>
#include <QStringList>

#include "matio.h"





class QMatFileItemModel2 : public QAbstractItemModel
{
    Q_OBJECT

public:
    struct MatDeleter {
        void operator()(mat_t *mat) {
            Mat_Close(mat);
        }
    };

    struct VarDeleter {
        void operator()(matvar_t *var) {
            Mat_VarFree(var);
        }
    };


public:
    explicit QMatFileItemModel2(const QString &fname);
    explicit QMatFileItemModel2(QSharedPointer<mat_t>);
    explicit QMatFileItemModel2(QSharedPointer<QMatFileItemModel2>,
        QSharedPointer<matvar_t>);
    explicit QMatFileItemModel2(QSharedPointer<QMatFileItemModel2>,
        int);
    explicit QMatFileItemModel2(QSharedPointer<QMatFileItemModel2>,
        const QModelIndex&);

    ~QMatFileItemModel2();

    void init();

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    QVariant data(const QModelIndex &index, int role) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QString name() const;
    QSharedPointer<QMatFileItemModel2> navigateInto(QSharedPointer<QMatFileItemModel2> self, const QModelIndex&) const;
    bool equals(const QSharedPointer<QMatFileItemModel2> &other) const;

private:
    size_t linear_index(QSharedPointer<matvar_t> var,
        const QVector<size_t> &indices) const;

    QString makeTypeDesc(matvar_t*) const;

    bool isArrayLike(matvar_t *var) const;


private:
    QSharedPointer<QMatFileItemModel2> m_Parent;
    QSharedPointer<mat_t> m_Mat;
    QSharedPointer<matvar_t> m_Var;
    QVector<size_t> m_Indices;
    int m_FieldIndex;
    int m_RowCount;
    int m_ColumnCount;
    QStringList m_VarNames;
    QStringList m_VarTypes;


};

#endif // QMATFILEITEMMODEL2_H
