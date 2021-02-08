//
// Created by ek-ut on 17.11.2020.
//

#ifndef CREATEVALIDATORWIDGET_H
#define CREATEVALIDATORWIDGET_H

#include <QWidget>
#include "qt/btcu/pwidget.h"
#include <QDialog>
class BTCUGUI;

namespace Ui {
   class CreateValidatorWidget;
}


class CreateValidatorWidget : public QDialog
{
   Q_OBJECT
public:
   explicit CreateValidatorWidget(QWidget *parent = nullptr);
   ~CreateValidatorWidget();
   bool isOk = false;

public Q_SLOTS:
   void onpbnBackClicked();

private:
   Ui::CreateValidatorWidget *ui;
};


#endif //CREATEVALIDATORWIDGET_H
