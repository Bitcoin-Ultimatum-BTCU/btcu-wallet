// Copyright (c) 2019-2020 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/txrow.h"
#include "qt/btcu/forms/ui_txrow.h"

#include "guiutil.h"
#include "qt/btcu/qtutils.h"

TxRow::TxRow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TxRow)
{
    ui->setupUi(this);
   shadowEffect = new QGraphicsDropShadowEffect();
   shadowEffect->setColor(QColor(0, 0, 0, 22));
   shadowEffect->setXOffset(0);
   shadowEffect->setYOffset(2);
   shadowEffect->setBlurRadius(6);
   ui->rowContainer->setProperty("cssClass", "container-border");
}

void TxRow::init(bool isLightTheme) {
    setConfirmStatus(true);
    updateStatus(isLightTheme, false, false);
}

void TxRow::setConfirmStatus(bool isConfirm){
    if(isConfirm){
        /*setCssProperty(ui->lblAddress, "text-list-body1");
        setCssProperty(ui->lblDate, "text-list-caption");*/
       setCssSubtitleScreen(ui->lblAddress);
       setCssSubtitleScreen(ui->lblDate);
    }else{
        setCssProperty(ui->lblAddress, "text-list-body-unconfirmed");
        setCssProperty(ui->lblDate,"text-list-caption-unconfirmed");
    }
}

void TxRow::updateStatus(bool isLightTheme, bool isHover, bool isSelected){
    if(isLightTheme)
        ui->lblDivisory->setStyleSheet("background-color:rgba(0, 0, 0, 0)"/*#bababa"*/);
    else
        ui->lblDivisory->setStyleSheet("background-color:rgba(0, 0, 0, 0)"/*#40ffffff"*/);
   this->setGraphicsEffect(0);
   ui->rowContainer->setGraphicsEffect(shadowEffect);
}

void TxRow::setDate(QDateTime date){
    ui->lblDate->setText(GUIUtil::dateTimeStr(date));
}

void TxRow::setLabel(QString str){
    ui->lblAddress->setText(str);
}

void TxRow::setAmount(QString str){
    ui->lblAmount->setText(str);
}

void TxRow::setType(bool isLightTheme, int type, bool isConfirmed){
    QString path;
    QString css;
    bool sameIcon = false;
    switch (type) {
        case TransactionRecord::ZerocoinMint:
            path = "://ic-transaction-mint";
            css = "text-list-amount-send";
            break;
        case TransactionRecord::Generated:
        case TransactionRecord::StakeZBTCU:
        case TransactionRecord::MNReward:
        case TransactionRecord::StakeMint:
        case TransactionRecord::LeasingReward:
            path = "://ic-transaction-staked";
            css = "text-list-amount-receive";
            break;
        case TransactionRecord::RecvWithObfuscation:
        case TransactionRecord::RecvWithAddress:
        case TransactionRecord::RecvFromOther:
        case TransactionRecord::RecvFromZerocoinSpend:
            path = "://ic-transaction-received";
            css = "text-list-amount-receive";
            break;
        case TransactionRecord::SendToAddress:
        case TransactionRecord::SendToOther:
        case TransactionRecord::ZerocoinSpend:
        case TransactionRecord::ZerocoinSpend_Change_zPiv:
        case TransactionRecord::ZerocoinSpend_FromMe:
            path = "://ic-transaction-sent";
            css = "text-list-amount-send";
            break;
        case TransactionRecord::SendToSelf:
            path = "://ic-transaction-mint";
            css = "text-list-amount-send";
            break;
        case TransactionRecord::StakeDelegated:
            path = "://ic-transaction-stake-delegated";
            css = "text-list-amount-receive";
            break;
        case TransactionRecord::StakeHot:
            path = "://ic-transaction-stake-hot";
            css = "text-list-amount-unconfirmed";
            break;
        case TransactionRecord::P2CSDelegationSent:
        case TransactionRecord::P2CSDelegationSentOwner:
            path = "://ic-transaction-cs-contract";
            css = "text-list-amount-send";
            break;
        case TransactionRecord::P2CSDelegation:
            path = "://ic-transaction-cs-contract";
            css = "text-list-amount-unconfirmed";
            break;
        case TransactionRecord::P2LLeasingSent:
        case TransactionRecord::P2LLeasingSentToSelf:
        case TransactionRecord::P2LLeasingRecv:
        case TransactionRecord::P2LUnlockOwnLeasing:
        case TransactionRecord::P2LUnlockLeasing:
        case TransactionRecord::P2LReturnLeasing:
            path = "://ic-transaction-cs-contract";
            css = "text-list-amount-send";
            break;
        case TransactionRecord::P2CSUnlockOwner:
        case TransactionRecord::P2CSUnlockStaker:
            path = "://ic-transaction-cs-contract";
            css = "text-list-amount-send";
            break;
        default:
            path = "://ic-pending";
            sameIcon = true;
            css = "text-list-amount-unconfirmed";
            break;
    }

    if (!isLightTheme && !sameIcon){
        path += "-dark";
    }

    if (!isConfirmed){
        css = "text-list-amount-unconfirmed";
        path += "-inactive";
        setConfirmStatus(false);
    }else{
        setConfirmStatus(true);
    }
    setCssProperty(ui->lblAmount, css, true);
    ui->icon->setIcon(QIcon(path));
}

TxRow::~TxRow(){
    delete ui;
}
