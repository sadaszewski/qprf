#ifndef _QMATNAVIGABLE_H_
#define _QMATNAVIGABLE_H_

class QAbstractItemModel;
class QModelIndex;
class QString;

class QMatNavigable {
public:
    virtual ~QMatNavigable() {}
    virtual QAbstractItemModel* navigateInto(const QModelIndex&) = 0;
    virtual bool equals(QMatNavigable *other) = 0;
    virtual QString name() = 0;
    virtual void setParentName(const QString&) = 0;
    virtual QString childName(const QModelIndex&) = 0;
};

#endif
