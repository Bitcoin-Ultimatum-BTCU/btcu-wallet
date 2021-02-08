// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/defaultdialog.h"
#include "qt/btcu/forms/ui_defaultdialog.h"
#include "guiutil.h"
DefaultDialog::DefaultDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DefaultDialog)
{
    ui->setupUi(this);

    // Stylesheet
    this->setStyleSheet(parent ? parent->styleSheet() : GUIUtil::loadStyleSheet());

    // Container
    ui->frame->setProperty("cssClass", "container-border");

    // Text
    ui->labelTitle->setText("Dialog Title");
    ui->labelTitle->setProperty("cssClass", "text-title-screen");


    ui->labelMessage->setText("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.");
    ui->labelMessage->setProperty("cssClass", "text-subtitle");

   // Buttons
    ui->btnEsc->setText("");
    ui->btnEsc->setProperty("cssClass", "ic-close");

    ui->btnCancel->setProperty("cssClass", "btn-primary");
    ui->btnSave->setText("OK");
    ui->btnSave->setProperty("cssClass", "btn-secundary");
    ui->verticalSpacer_4->changeSize(0,0,QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(ui->btnEsc, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->btnCancel, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->btnSave, &QPushButton::clicked, [this](){this->isOk = true; accept();});

}

void DefaultDialog::setText(QString title, QString message, QString okBtnText, QString cancelBtnText){
    if(!okBtnText.isNull()) ui->btnSave->setText(okBtnText);
    if(!cancelBtnText.isNull() && !cancelBtnText.isEmpty()){
        ui->btnCancel->setVisible(true);
        ui->btnCancel->setText(cancelBtnText);
    }else{
        ui->btnCancel->setVisible(false);
    }
    if(!message.isNull()) ui->labelMessage->setText(message);
    if(!title.isNull() && !title.isEmpty()) {
       ui->labelTitle->setText(title);
       ui->verticalSpacer->changeSize(0,0,QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
    else {
       ui->labelTitle->setText(message);
       ui->labelMessage->setVisible(false);
       ui->labelTitle->setAlignment(Qt::AlignLeft);
       ui->verticalSpacer_3->changeSize(0,0,QSizePolicy::Fixed, QSizePolicy::Fixed);
       ui->verticalSpacer_2->changeSize(0,0,QSizePolicy::Fixed, QSizePolicy::Fixed);
       ui->horizontalSpacer_21->changeSize(10,0,QSizePolicy::Fixed, QSizePolicy::Fixed);
       ui->horizontalSpacer11->changeSize(15,0,QSizePolicy::Fixed, QSizePolicy::Fixed);
       ui->horizontalSpacer_2->changeSize(10,0,QSizePolicy::Fixed, QSizePolicy::Fixed);
       ui->labelTitle->setProperty("cssClass", "text-message-dialog");
    }
}

void DefaultDialog::setSizeButtons(int WidthSave, int WidthCancel)
{
   ui->btnSave->setMinimumWidth(WidthSave);
   ui->btnCancel->setMinimumWidth(WidthCancel);
}
void DefaultDialog::setType(int type)
{
   ui->labelMessage->setProperty("cssClass", "text-title-screen");
   ui->labelTitle->setAlignment(Qt::AlignLeft);
   ui->labelMessage->setAlignment(Qt::AlignLeft);
   ui->verticalSpacer_2->changeSize(0,5,QSizePolicy::Fixed, QSizePolicy::Fixed);
   ui->verticalSpacer_4->changeSize(0,30,QSizePolicy::Fixed, QSizePolicy::Fixed);
   ui->verticalSpacer->changeSize(0,40,QSizePolicy::Fixed, QSizePolicy::Fixed);
}

DefaultDialog::~DefaultDialog()
{
    delete ui;
}
