//
// Created by ek-ut on 17.11.2020.
//

#include "qt/btcu/leasingstatisticswidget.h"
#include "qt/btcu/forms/ui_leasingstatisticswidget.h"


#include "qt/btcu/qtutils.h"
#include "amount.h"
#include "optionsmodel.h"
#include "coincontrol.h"

LeasingStatisticsWidget::LeasingStatisticsWidget(BTCUGUI *parent) :
PWidget(parent),
ui(new Ui::LeasingStatisticsWidget)
{
   ui->setupUi(this);

   this->setStyleSheet(parent->styleSheet());

   // Container
   //ui->frame->setProperty("cssClass", "container-border");
   ui->containerLeasing_1->setProperty("cssClass", "container-border");
   ui->containerLeasing_2->setProperty("cssClass", "container-border");
   ui->containerLeasing_3->setProperty("cssClass", "container-border");
   ui->containerLeasing_4->setProperty("cssClass", "container-border");
   ui->containerNode_1->setProperty("cssClass", "container-border");
   ui->containerNode_2->setProperty("cssClass", "container-border");
   ui->containerNode_3->setProperty("cssClass", "container-border");
   ui->containerNode_4->setProperty("cssClass", "container-border");

   // Buttons
   ui->pbnBack->setProperty("cssClass", "btn-primary");
   ui->pbnUnleasing->setProperty("cssClass", "btn-secundary");
   connect(ui->pbnBack, SIGNAL(clicked()), this, SLOT(onpbnBackClicked()));
   // Text
   ui->labelLeasing->setProperty("cssClass", "text-title-dialog");
   ui->labelNode->setProperty("cssClass", "text-title-dialog");

   ui->labelMasternode->setProperty("cssClass", "text-list-title1");
   ui->labelName->setProperty("cssClass", "text-list-title1");
   ui->labelLeasing_1_data->setProperty("cssClass", "text-list-title1");
   ui->labelLeasing_2_data->setProperty("cssClass", "text-list-title1");
   ui->labelLeasing_3_data->setProperty("cssClass", "text-list-title1");
   ui->labelLeasing_4_data->setProperty("cssClass", "text-list-title1");
   ui->labelNode_1_data->setProperty("cssClass", "text-list-title1");
   ui->labelNode_2_data->setProperty("cssClass", "text-list-title1");
   ui->labelNode_3_data->setProperty("cssClass", "text-list-title1");
   ui->labelNode_4_data->setProperty("cssClass", "text-list-title1");
   ui->labelLeasing_1_txt->setProperty("cssClass", "text-body2-grey");
   ui->labelLeasing_2_txt->setProperty("cssClass", "text-body2-grey");
   ui->labelLeasing_3_txt->setProperty("cssClass", "text-body2-grey");
   ui->labelLeasing_4_txt->setProperty("cssClass", "text-body2-grey");
   ui->labelNode_1_txt->setProperty("cssClass", "text-body2-grey");
   ui->labelNode_2_txt->setProperty("cssClass", "text-body2-grey");
   ui->labelNode_3_txt->setProperty("cssClass", "text-body2-grey");
   ui->labelNode_4_txt->setProperty("cssClass", "text-body2-grey");

   ui->labelAddress->setProperty("cssClass", "text-list-amount-send");


}

void LeasingStatisticsWidget::onpbnBackClicked()
{
   window->goToLeasing();
}

LeasingStatisticsWidget::~LeasingStatisticsWidget()
{
   delete ui;
}
