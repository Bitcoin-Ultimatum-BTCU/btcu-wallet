//
// Created by ek-ut on 17.11.2020.
//

#ifndef LEASINGSTATISTICSWIDGET_H
#define LEASINGSTATISTICSWIDGET_H
#include <QWidget>
#include "qt/btcu/pwidget.h"



class BTCUGUI;

namespace Ui {
   class LeasingStatisticsWidget;
}

class LeasingStatisticsWidget : public PWidget
{
   Q_OBJECT

public:

   explicit LeasingStatisticsWidget(BTCUGUI *parent);
   ~LeasingStatisticsWidget();

public Q_SLOTS:
   void onpbnBackClicked();
private:
   Ui::LeasingStatisticsWidget *ui;
};


#endif //LEASINGSTATISTICSWIDGET_H
