// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FURLISTROW_H
#define FURLISTROW_H

#include <QWidget>
#include <QColor>

template <class T=QWidget*>
class FurListRow{

public:
    FurListRow(){}
    virtual inline ~FurListRow(){}

    virtual inline T createHolder(int){
       return nullptr;
    }

    virtual inline void init(T, const QModelIndex&, bool isHovered, bool isSelected) const{

    }

    virtual inline QColor rectColor(bool isHovered, bool isSelected){
        return QColor();
    }

   virtual inline T getWidget(){
      return nullptr;
   }
};

#endif // FURLISTROW_H
