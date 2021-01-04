// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/lrow.h"
#include "qt/btcu/forms/ui_lrow.h"

LRow::LRow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LRow)
{
    ui->setupUi(this);
    ui->labelName->setProperty("cssClass", "text-list-title1");
    ui->labelAddress->setProperty("cssClass", "text-list-body2");
    ui->labelType->setProperty("cssClass", "text-list-caption-medium");
    ui->labelAmount->setProperty("cssClass", "text-list-amount-unconfirmed");
    ui->labelReward->setProperty("cssClass", "text-list-amount-unconfirmed");
}

void LRow::updateView(const QString& address, const QString& label, bool isLeaser, const QString& amount, const QString& reward) {
    ui->labelName->setText(label);
    ui->labelAddress->setText(address);
    ui->labelAmount->setText(tr("Amount: ") + amount);
    ui->labelReward->setText(tr("Reward: ") + reward);

    if (isLeaser) {
        ui->labelType->setText(tr("Leaser"));
    } else {
        ui->labelType->setText(tr("Owner"));
    }
}

void LRow::updateState(bool isLightTheme, bool isHovered, bool isSelected) {
    ui->lblDivisory->setStyleSheet((isLightTheme) ?  "background-color:#bababa" : "background-color:#40ffffff");
    if (fShowMenuButton) {
        ui->pushButtonMenu->setVisible(isHovered);
    }
}

void LRow::showMenuButton(bool show) {
    this->fShowMenuButton = show;
}

void LRow::enterEvent(QEvent *) {
    if (fShowMenuButton) {
        ui->pushButtonMenu->setVisible(true);
        update();
    }
}

void LRow::leaveEvent(QEvent *) {
    if (fShowMenuButton) {
        ui->pushButtonMenu->setVisible(false);
        update();
    }
}

LRow::~LRow() = default;
