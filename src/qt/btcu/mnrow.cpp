// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/mnrow.h"
#include "qt/btcu/forms/ui_mnrow.h"
#include "qt/btcu/qtutils.h"

MNRow::MNRow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MNRow)
{
    ui->setupUi(this);

   setCssProperty(ui->Contener, "container-border");
   setCssSubtitleScreen(ui->label);
   ui->label_2->setProperty("cssClass","text-list-amount-send");
   setCssSubtitleScreen(ui->label_3);
   setCssSubtitleScreen(ui->label_4);
   setCssSubtitleScreen(ui->label_5);
   setCssSubtitleScreen(ui->label_6);
   connect(ui->pushButtonMenu, SIGNAL(clicked()), this, SLOT(onPbnMenuClicked()));

}

void MNRow::updateView(QString address, QString label, QString status, bool wasCollateralAccepted){
    /*ui->labelName->setText(label);
    ui->labelAddress->setText(address);
    ui->labelDate->setText("Status: " + status);
    if (!wasCollateralAccepted){
        ui->labelDate->setText("Status: Collateral tx not found");
    } else {Q_EMIT
        ui->labelDate->setText("Status: " + status);
    }*/
}
void MNRow::onPbnMenuClicked()
{
   Q_EMIT onMenuClicked();
}

MNRow::~MNRow(){
    delete ui;
}
