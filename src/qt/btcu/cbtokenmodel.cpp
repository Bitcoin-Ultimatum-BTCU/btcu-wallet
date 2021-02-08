//
// Created by ek-ut on 19.11.2020.
//

#include "cbtokenmodel.h"
CBTokenModel::CBTokenModel() {
   columns << tr("Label") << tr("Name") << tr("Date");
}

CBTokenModel::~CBTokenModel() 
{

}
int CBTokenModel::rowCount(const QModelIndex& parent) const
{
   Q_UNUSED(parent);
   return Tokens.size();
}

int CBTokenModel::rowCount() const
{
   return Tokens.size();
}
int CBTokenModel::columnCount(const QModelIndex& parent) const
{
   Q_UNUSED(parent);
   return columns.length();
}
QVariant CBTokenModel::data(const QModelIndex& index, int role) const
{
   if (!index.isValid())
      return QVariant();
   CBTokenRow * CurentToken = Tokens.at(index.row());
   if (role == Qt::DisplayRole || role == Qt::EditRole) {

      switch (index.column()) {
         case Label:
            return CurentToken->getPixmap() ;
         case Name:
            return CurentToken->getText() + "   " + CurentToken->getAvailable();
         case Date:
            return CurentToken->getText();
      }
   }

   return QVariant();
}
bool CBTokenModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
   if (!index.isValid())
      return false;

   return false;
}
QVariant CBTokenModel::headerData(int section, Qt::Orientation orientation, int role) const
{
   if (orientation == Qt::Horizontal)
   {
      if (role == Qt::DisplayRole && section < columns.size())
      {
         return columns[section];
      }
   }
   return QVariant();
}
/*QModelIndex CBTokenModel::index(int row, int column, const QModelIndex& parent) const
{
   Q_UNUSED(parent);
   //return QModelIndex();
   CBTokenRow * CurentToken = Tokens.at(row);
   QString  s ;
   s = CurentToken->getText();
   if (CurentToken) {
      return createIndex(row, column, &s);
   } else {
      return QModelIndex();
   }
}*/
bool CBTokenModel::removeRows(int row, int count, const QModelIndex& parent)
{
   Q_UNUSED(parent);
   return false;
}

Qt::ItemFlags CBTokenModel::flags(const QModelIndex& index) const
{
   if (!index.isValid())
      return 0;

   return 0;
}

void CBTokenModel::AddRow(CBTokenRow* newTocen)
{
   Tokens.append(newTocen);
}