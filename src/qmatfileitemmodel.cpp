//
// Copyright (C) Stanislaw Adaszewski, 2016-2017
// All Rights Reserved.
// Website: http://adared.ch
// Email: info@adared.ch
//

#include "qmatfileitemmodel.h"

#include <QVector>
#include <QSharedPointer>




QMatFileItemModel::QMatFileItemModel(mat_t *mat)
{
    m_Mat = mat;
}

int QMatFileItemModel::rowCount(const QModelIndex &parent) const {
    if (!parent.isValid()) {
        Mat_Rewind(m_Mat);
        matvar_t *var;
        int count = 0;
        while ((var = Mat_VarReadNextInfo(m_Mat)) != NULL) {
            count++;
            Mat_VarFree(var);
        }
        return count;
    } else {
        MatIndex *user_data = (MatIndex*) parent.internalPointer();
        matvar_t *var = user_data->var;

        if (var->data_type & MAT_T_STRUCT) {

            if (user_data->field_name.isNull()) {

                int nfields = Mat_VarGetNumberOfFields(var);
                return nfields;

            } else {

                if (user_data->rank < var->rank) {
                    return var->dims[user_data->rank];
                } else {
                    return 0;
                }

            }

        } else if (var->data_type & MAT_T_CELL) {

            if (user_data->rank < var->rank) {
                return var->dims[user_data->rank];
            } else {
                return 0;
            }

        } else if (var->data_type & MAT_T_MATRIX) {

            return 0;

            /* if (user_data->rank < var->rank) {
                return var->dims[user_data->rank];
            } else {
                return 0;
            } */

        } else if (var->data_type & MAT_T_STRING) {
            return 0;
        } else {
            return 0;
        }
    }
}

int QMatFileItemModel::columnCount(const QModelIndex &parent) const {
    if (!parent.isValid()) {
       return 1;
    } else {
        MatIndex *user_data = (MatIndex*) parent.internalPointer();
        matvar_t *var = user_data->var;

        if (var->data_type & MAT_T_STRUCT) {

            if (user_data->field_name.isNull()) {

                return 1;

            } else {

                if (user_data->rank + 1 < var->rank) {
                    return var->dims[user_data->rank + 1];
                } else {
                    return 1;
                }

            }

        } else if (var->data_type & MAT_T_CELL) {

            if (user_data->rank + 1 < var->rank) {
                return var->dims[user_data->rank + 1];
            } else {
                return 1;
            }

        } else if (var->data_type & MAT_T_MATRIX) {

            if (user_data->rank + 1 < var->rank) {
                return var->dims[user_data->rank + 1];
            } else {
                return 1;
            }

        } else {
            return 0;
        }
    }
}

size_t QMatFileItemModel::linear_index(MatIndex *user_data) const {
    matvar_t *var = user_data->var;
    size_t linear = 0;

    for (int i = var->rank - 1; i > 0; i--) {
        linear += user_data->indices[i];
        linear *= var->dims[i - 1];
    }
    linear += user_data->indices[0];

    return linear;
}

QModelIndex QMatFileItemModel::index(int row, int column, const QModelIndex &parent) const {
    if (!parent.isValid()) {
        Mat_Rewind(m_Mat);
        matvar_t *var;
        for (int i = 0; i < row - 1; i++) {
            var = Mat_VarReadNextInfo(m_Mat);
            Mat_VarFree(var);
        }
        var = Mat_VarReadNextInfo(m_Mat);
        QSharedPointer<MatIndex> user_data = QSharedPointer<MatIndex>::create();
        user_data->var = var;
        indexes.append(user_data);
        return createIndex(0, 0, user_data.data());
    } else {
        MatIndex *user_data = (MatIndex*) parent.internalPointer();
        matvar_t *var = user_data->var;

        QSharedPointer<MatIndex> new_data = QSharedPointer<MatIndex>::create();
        new_data->parent = user_data;
        indexes.append(new_data);

        if (var->data_type & MAT_T_STRUCT) {

            if (user_data->field_name.isNull()) {

                char* const *fieldnames = Mat_VarGetStructFieldnames(var);
                new_data->field_name = fieldnames[row];
                return createIndex(0, 0, new_data.data());

            } else {

                if (user_data->rank < var->rank) {

                    new_data->var = var;
                    new_data->rank = user_data->rank + 2;
                    new_data->indices = QVector<size_t>(user_data->indices);
                    new_data->indices.append(row);
                    new_data->indices.append(column);
                    return createIndex(0, 0, new_data.data());


                } else {

                    size_t linear = linear_index(user_data);
                    new_data->var = Mat_VarGetStructFieldByName(var, user_data->field_name.toLatin1(), linear);
                    return createIndex(0, 0, new_data.data());


                }

            }

        } else if (var->data_type & MAT_T_CELL) {

            if (user_data->rank < var->rank) {

                new_data->var = var;
                new_data->rank = user_data->rank + 2;
                new_data->indices = QVector<size_t>(user_data->indices);
                new_data->indices.append(row);
                new_data->indices.append(column);
                return createIndex(0, 0, new_data.data());


            } else {

                size_t linear = linear_index(user_data);
                new_data->var = Mat_VarGetCell(var, linear);
                return createIndex(0, 0, new_data.data());


            }

        } else if (var->data_type & MAT_T_MATRIX) {



        } else {

        }
    }
}

QModelIndex QMatFileItemModel::parent(const QModelIndex &child) const {
    MatIndex *user_data = (MatIndex*) child.internalPointer();
    if (user_data->parent == NULL) {
        return QModelIndex();
    } else {
        return createIndex(0, 0, user_data->parent);
    }
}

QVariant QMatFileItemModel::data(const QModelIndex &index, int role) const {
    MatIndex *user_data = (MatIndex*) index.internalPointer();
    matvar_t *var = user_data->var;

    if (role != Qt::DisplayRole) return QVariant();

    if (var->data_type & MAT_T_STRUCT) {

        if (user_data->field_name.isNull()) {

            return QVariant(QString("%1").arg(QString::fromLatin1(var->name)));

        } else if (user_data->indices.isEmpty()) {

            return user_data->field_name;

        } else {

            return QString("[%1, %2]")
                .arg(user_data->indices[user_data->rank - 2])
                .arg(user_data->indices[user_data->rank - 1]);

        }

    } else if (var->data_type & MAT_T_CELL) {

        if (user_data->indices.isEmpty()) {

            return QVariant(QString::fromLatin1(var->name));

        } else {

            return QString("[%1, %2]")
                .arg(user_data->indices[user_data->rank - 2])
                .arg(user_data->indices[user_data->rank - 1]);

        }

    } else {
        return QVariant();
    }
}
