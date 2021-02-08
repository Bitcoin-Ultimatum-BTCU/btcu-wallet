//
// Created by ek-ut on 17.11.2020.
//

#include "createvalidatorwidget.h"
#include "qt/btcu/forms/ui_createvalidatorwidget.h"
#include "qt/btcu/qtutils.h"
#include "amount.h"
#include "optionsmodel.h"
#include "coincontrol.h"
CreateValidatorWidget::CreateValidatorWidget(QWidget *parent) :
QDialog(parent),
ui(new Ui::CreateValidatorWidget)
{
   ui->setupUi(this);

   this->setStyleSheet(parent->styleSheet());

   // Container
   ui->frame->setProperty("cssClass", "container-border");

   setCssProperty(ui->labelBord, "line-list-menu");
   // Buttons
   ui->pbnBack->setProperty("cssClass", "btn-leasing-dialog-back");
   ui->pbnCreate->setProperty("cssClass", "btn-leasing-dialog-save");
   //connect(ui->pbnBack, SIGNAL(clicked()), this, SLOT(onpbnBackClicked()));
   connect(ui->pbnBack, SIGNAL(clicked()), this, SLOT(close()));
   connect(ui->pbnCreate, &QPushButton::clicked, [this](){this->isOk = true; accept();});

   // Text

   setCssTitleScreen(ui->labelValidator);
   setCssSubtitleScreen(ui->labelName);
   setCssSubtitleScreen(ui->labelAddress);
   ui->labelValidator->setProperty("cssClass", "text-title-screen");
   ui->labelName->setProperty("cssClass", "text-leasing-dialog");
   ui->labelAddress->setProperty("cssClass", "text-leasing-dialog");

   ui->lineEditName->setPlaceholderText(tr("Validator"));
   ui->lineEditName->setProperty("cssClass", "edit-leasing-dialog");
   ui->lineEditAddress->setPlaceholderText("0");
   ui->lineEditAddress->setProperty("cssClass", "edit-leasing-dialog");



}
void CreateValidatorWidget::onpbnBackClicked()
{
   //window->goToMasterNodes();
}

CreateValidatorWidget::~CreateValidatorWidget()
{
   delete ui;
}