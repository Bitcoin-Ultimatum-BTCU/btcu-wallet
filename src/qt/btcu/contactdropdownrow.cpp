// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/contactdropdownrow.h"
#include "qt/btcu/forms/ui_contactdropdownrow.h"

ContactDropdownRow::ContactDropdownRow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContactDropdownRow)
{
    ui->setupUi(this);
   ui->lblAddress->setProperty("cssClass", "btn-subtitle-grey");
   ui->lblDivisory->setStyleSheet("background: #BDBDC7");
    ui->lblLabel->setProperty("cssClass", "text-subtitle");
}

void ContactDropdownRow::init(bool isLightTheme, bool isHover) {
    update(isLightTheme, isHover, false);
}

void ContactDropdownRow::update(bool isLightTheme, bool isHover, bool isSelected){
    //ui->lblDivisory->setStyleSheet("background-color:#bababa");
}

void ContactDropdownRow::setData(QString address, QString label){
    ui->lblAddress->setText(address);
    ui->lblLabel->setText(label);
}
void ContactDropdownRow::setVisibleDivisory(bool isVisible)
{
   ui->lblDivisory->setVisible(isVisible);
}

ContactDropdownRow::~ContactDropdownRow()
{
    delete ui;
}
