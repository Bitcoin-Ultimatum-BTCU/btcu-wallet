//
// Created by ek-ut on 17.11.2020.
//


#include "qt/btcu/createmasternodewidget.h"
#include "qt/btcu/forms/ui_createmasternodewidget.h"
#include "qt/btcu/qtutils.h"
#include "amount.h"
#include "optionsmodel.h"
#include "coincontrol.h"

CreateMasterNodeWidget::CreateMasterNodeWidget(QWidget *parent) :
QDialog(parent),
ui(new Ui::CreateMasterNodeWidget)
{
   ui->setupUi(this);

   this->setStyleSheet(parent->styleSheet());

   // Container
   ui->frame->setProperty("cssClass", "container-border");
   setCssProperty(ui->labelBord, "line-list-menu");
   // Buttons
   ui->pbnBack->setProperty("cssClass", "btn-leasing-dialog-back");
   //connect(ui->pbnBack, SIGNAL(clicked()), this, SLOT(onpbnBackClicked()));

   connect(ui->pbnBack, SIGNAL(clicked()), this, SLOT(close()));
   connect(ui->pbnCreate, &QPushButton::clicked, [this](){this->isOk = true; accept();});
   ui->pbnCreate->setProperty("cssClass", "btn-leasing-dialog-save");

   // Text
   //setCssTitleScreen(ui->labelMasternode);
   //setCssSubtitleScreen(ui->labelName);
   //setCssSubtitleScreen(ui->labelAddress);
   ui->labelMasternode->setProperty("cssClass", "text-title-screen");
   ui->labelName->setProperty("cssClass", "text-leasing-dialog");
   ui->labelAddress->setProperty("cssClass", "text-leasing-dialog");

   ui->lineEditName->setPlaceholderText(tr("Masternode"));
   ui->lineEditName->setProperty("cssClass", "edit-leasing-dialog");
   ui->lineEditAddress->setPlaceholderText("0");
   ui->lineEditAddress->setProperty("cssClass", "edit-leasing-dialog");



}

void CreateMasterNodeWidget::onpbnBackClicked()
{
   /*window->goToMasterNodes();*/
}
CreateMasterNodeWidget::~CreateMasterNodeWidget()
{
   delete ui;
}