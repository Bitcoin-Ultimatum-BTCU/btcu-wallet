//
// Created by ek-ut on 16.11.2020.
//

//#ifndef BTCU_MULTILEASINGDIALOG_H
//#define BTCU_MULTILEASINGDIALOG_H
#ifndef MULTILEASINGDIALOG_H
#define MULTILEASINGDIALOG_H

#include <QDialog>
#include "qt/btcu/pwidget.h"

namespace Ui {
   class MultiLeasingDialog;
}

class MultiLeasingDialog: public QDialog
{
Q_OBJECT

public:
   explicit MultiLeasingDialog(BTCUGUI *parent = nullptr);
   ~MultiLeasingDialog();

   void showHide(bool show);

private Q_SLOTS:
   void onbtnSave();

private:
   BTCUGUI *pwParent = nullptr;
   bool isSave = false;
   Ui::MultiLeasingDialog *ui;

};


#endif //MULTILEASINGDIALOG_H
