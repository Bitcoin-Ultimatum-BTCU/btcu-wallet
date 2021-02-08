//
// Created by ek-ut on 17.11.2020.
//

#ifndef CREATEMASTERNODEWIDGET_H
#define CREATEMASTERNODEWIDGET_H

#include <QWidget>
#include "qt/btcu/pwidget.h"
#include <QDialog>

class BTCUGUI;

namespace Ui {
   class CreateMasterNodeWidget;
}

class CreateMasterNodeWidget : public QDialog
{
   Q_OBJECT

public:

   explicit CreateMasterNodeWidget(QWidget *parent = nullptr);
   ~CreateMasterNodeWidget();
   bool isOk = false;

public Q_SLOTS:
   void onpbnBackClicked();

private:
   Ui::CreateMasterNodeWidget *ui;
};


#endif //CREATEMASTERNODEWIDGET_H
