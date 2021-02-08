//
// Created by ek-ut on 19.11.2020.
//

#ifndef CBTOKENMODEL_H
#define CBTOKENMODEL_H
#include <QAbstractTableModel>
#include "qt/btcu/cbtokenrow.h"


Q_DECLARE_METATYPE(CBTokenRow*)

class CBTokenModel: public QAbstractTableModel
{
Q_OBJECT

public:
   explicit CBTokenModel();
   ~CBTokenModel();

   enum ColumnIndex {
      Label = 0,
      Name = 1,
      Date = 2
   };

   enum RoleIndex {
      TypeRole = Qt::UserRole
   };
   int rowCount(const QModelIndex& parent) const;
   int columnCount(const QModelIndex& parent) const;
   QVariant data(const QModelIndex& index, int role) const;
   bool setData(const QModelIndex& index, const QVariant& value, int role);
   QVariant headerData(int section, Qt::Orientation orientation, int role) const;
   //QModelIndex index(int row, int column, const QModelIndex& parent) const;
   bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
   Qt::ItemFlags flags(const QModelIndex& index) const;
   void AddRow(CBTokenRow* newTocen);
   int rowCount() const;

private:
   QStringList columns;
   QList<CBTokenRow* > Tokens;
    //typedef QList< QPair > Tokens;
};


#endif //CBTOKENMODEL_H
