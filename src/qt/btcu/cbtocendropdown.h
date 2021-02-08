//
// Created by ek-ut on 27.11.2020.
//

#include "qt/btcu/pwidget.h"
#include "qt/btcu/cbtokenmodel.h"
#include "qt/btcu/furabstractlistitemdelegate.h"

#include <QListView>
#include <QObject>
#include <QWidget>

#ifndef CBTOCENDROPDOWN_H
#define CBTOCENDROPDOWN_H


class CbTocenDropdown: public PWidget
{
Q_OBJECT
public:
   explicit CbTocenDropdown(int minWidth, int minHeight, BTCUGUI* parent = nullptr);

   void resizeList(int minWidth, int mintHeight);
   void setTokenModel(CBTokenModel* _model, const QString& type);

   virtual void showEvent(QShowEvent *event) override;
   virtual void leaveEvent(QEvent *);
Q_SIGNALS:
   void contactSelected(QString tocen,  QPixmap ico);
   void TimerHide();
private:
   CBTokenModel* model = nullptr;
   FurAbstractListItemDelegate* delegate = nullptr;

   QListView *list;
   QFrame *frameList;
private Q_SLOTS:
   void handleClick(const QModelIndex &index);


};


#endif //CBTOCENDROPDOWN_H
