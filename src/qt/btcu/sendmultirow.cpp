// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/sendmultirow.h"
#include "qt/btcu/forms/ui_sendmultirow.h"

#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "guiutil.h"
#include "bitcoinunits.h"
#include "qt/btcu/qtutils.h"

SendMultiRow::SendMultiRow(PWidget *parent) :
    PWidget(parent),
    ui(new Ui::SendMultiRow),
    iconNumber(new QPushButton())
{
    ui->setupUi(this);
    this->setStyleSheet(parent->styleSheet());

    ui->lineEditAddress->setPlaceholderText(tr("Enter address"));
    setCssProperty(ui->lineEditAddress, "edit-primary-multi-book");
    ui->lineEditAddress->setAttribute(Qt::WA_MacShowFocusRect, 0);
    //setShadow(ui->stackedAddress);
    ui->page_3->setVisible(false);

    ui->lineEditBTCU->setPlaceholderText("Amount 0.00");
    initCssEditLine(ui->containerBTCU);
    ui->lineEditBTCU->setProperty("cssClass","edit-primary-BTCU");
   GUIUtil::setupAmountWidget(ui->lineEditBTCU, this);
   setCssSubtitleScreen(ui->labelBTCU);

    /* Description */
   ui->labelSubtitleDescription->setVisible(false);
    ui->labelSubtitleDescription->setText("Address label (optional)");
    setCssProperty(ui->labelSubtitleDescription, "text-title");
    ui->lineEditDescription->setPlaceholderText(tr("Enter label"));
    initCssEditLine(ui->lineEditDescription);

    // Button menu
    setCssProperty(ui->btnMenu, "btn-menu");
    ui->btnMenu->setVisible(false);

    // Button Contact
    btnContact = ui->lineEditAddress->addAction(QIcon("://ic-contact-arrow-down"), QLineEdit::TrailingPosition);

    //btnUpContact = ui->lineEditAddress->addAction(QIcon("://ic-contact-arrow-up"), QLineEdit::TrailingPosition);
    //ui->lineEditAddress->removeAction(btnUpContact);
    // Icon Number
    ui->stackedAddress->addWidget(iconNumber);
    iconNumber->show();
    iconNumber->raise();

    setCssProperty(iconNumber, "ic-multi-number");
    iconNumber->setText("1");
    iconNumber->setVisible(false);
    QSize size = QSize(24, 24);
    iconNumber->setMinimumSize(size);
    iconNumber->setMaximumSize(size);

    int posIconX = 0;
    int posIconY = 14;
    iconNumber->move(posIconX, posIconY);

    connect(ui->lineEditBTCU, SIGNAL(textChanged(const QString&)), this, SLOT(amountChanged(const QString&)));
    connect(ui->lineEditAddress, SIGNAL(textChanged(const QString&)), this, SLOT(addressChanged(const QString&)));
    connect(btnContact, &QAction::triggered, [this](){
       btnContact->setIcon(QIcon("://ic-contact-arrow-up"));
       /*ui->lineEditAddress->removeAction(btnContact);
       ui->lineEditAddress->addAction(btnUpContact, QLineEdit::TrailingPosition);*/
       Q_EMIT onContactsClicked(this);});
    connect(ui->btnMenu, &QPushButton::clicked, [this](){Q_EMIT onMenuClicked(this);});
    /*connect(btnUpContact, &QAction::triggered, [this](){
       btnContact->setIcon(QIcon("://ic-contact-arrow-down"));
       Q_EMIT onContactsClicked(this);});*/

}

void SendMultiRow::amountChanged(const QString& amount){
    if(!amount.isEmpty()) {
        QString amountStr = amount;
        CAmount value = getAmountValue(amountStr);
        if (value > 0) {
            GUIUtil::updateWidgetTextAndCursorPosition(ui->lineEditBTCU, amountStr);
            setCssEditLine(ui->containerBTCU, true, true);
        }
    }
    Q_EMIT onValueChanged();
}

/**
 * Returns -1 if the value is invalid
 */
CAmount SendMultiRow::getAmountValue(QString amount){
    bool isValid = false;
    CAmount value = GUIUtil::parseValue(amount, displayUnit, &isValid);
    return isValid ? value : -1;
}

bool SendMultiRow::addressChanged(const QString& str){
    if(!str.isEmpty()) {
        QString trimmedStr = str.trimmed();

        bool valid = false;

        if (this->onlyStakingAddressAccepted)
            valid = walletModel->validateStakingAddress(trimmedStr);
        else if (this->onlyLeasingAddressAccepted)
            valid = walletModel->validateLeasingAddress(trimmedStr);
        else
            valid = walletModel->validateAddress(trimmedStr);
        if (!valid) {
            // check URI
            SendCoinsRecipient rcp;
            if (GUIUtil::parseBitcoinURI(trimmedStr, &rcp)) {
                ui->lineEditAddress->setText(rcp.address);
                ui->lineEditBTCU->setText(BitcoinUnits::format(displayUnit, rcp.amount, false));

                QString label = walletModel->getAddressTableModel()->labelForAddress(rcp.address);
                if (!label.isNull() && !label.isEmpty()){
                    ui->lineEditDescription->setText(label);
                } else if(!rcp.message.isEmpty())
                    ui->lineEditDescription->setText(rcp.message);

                Q_EMIT onUriParsed(rcp);
            } else {
                setCssProperty(ui->lineEditAddress, "edit-primary-multi-book-error");
            }
        } else {
            setCssProperty(ui->lineEditAddress, "edit-primary-multi-book");
            QString label = walletModel->getAddressTableModel()->labelForAddress(trimmedStr);
            if (!label.isNull()){
                ui->lineEditDescription->setText(label);
            }
        }
        updateStyle(ui->lineEditAddress);
        return valid;
    }
    return false;
}


void SendMultiRow::loadWalletModel() {
    if (walletModel && walletModel->getOptionsModel()) {
        displayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        connect(walletModel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }
    clear();
}

void SendMultiRow::updateDisplayUnit(){
    // Update edit text..
    displayUnit = walletModel->getOptionsModel()->getDisplayUnit();
}

void SendMultiRow::deleteClicked() {
    Q_EMIT removeEntry(this);
}

void SendMultiRow::clear() {
    ui->lineEditAddress->clear();
    ui->lineEditBTCU->clear();
    ui->lineEditDescription->clear();
    setCssProperty(ui->lineEditAddress, "edit-primary-multi-book", true);
}

bool SendMultiRow::validate()
{
    if (!walletModel)
        return false;

    // Check input validity
    bool retval = true;

    // Skip checks for payment request
    if (recipient.paymentRequest.IsInitialized())
        return retval;

    // Check address validity, returns false if it's invalid
    QString address = ui->lineEditAddress->text();
    if (address.isEmpty()){
        retval = false;
        setCssProperty(ui->lineEditAddress, "edit-primary-multi-book-error", true);
    } else
        retval = addressChanged(address);

    CAmount value = getAmountValue(ui->lineEditBTCU->text());

    // Sending a zero amount is invalid
    if (value <= 0) {
        setCssEditLine(ui->containerBTCU, false, true);
        retval = false;
    }

    // Reject dust outputs:
    if (retval && GUIUtil::isDust(address, value)) {
        setCssEditLine(ui->containerBTCU, false, true);
        retval = false;
    }

    return retval;
}

SendCoinsRecipient SendMultiRow::getValue() {
    // Payment request
    if (recipient.paymentRequest.IsInitialized())
        return recipient;

    // Normal payment
    recipient.address = getAddress();
    recipient.label = ui->lineEditDescription->text();
    recipient.amount = getAmountValue();;
    return recipient;
}

QString SendMultiRow::getAddress() {
    return ui->lineEditAddress->text().trimmed();
}

CAmount SendMultiRow::getAmountValue() {
    return getAmountValue(ui->lineEditBTCU->text());
}

QRect SendMultiRow::getEditLineRect(){
    return ui->lineEditAddress->rect();
}

int SendMultiRow::getEditHeight(){
    return ui->stackedAddress->height();
}

int SendMultiRow::getEditWidth(){
    return ui->lineEditAddress->width();
}

int SendMultiRow::getNumber(){
    return number;
}

void SendMultiRow::setAddress(const QString& address) {
    ui->lineEditAddress->setText(address);
    ui->lineEditBTCU->setFocus();
   btnContact->setIcon(QIcon("://ic-contact-arrow-down"));
}

void SendMultiRow::updateAction()
{
   btnContact->setIcon(QIcon("://ic-contact-arrow-down"));
}

void SendMultiRow::setAmount(const QString& amount){
    ui->lineEditBTCU->setText(amount);
}

void SendMultiRow::setAddressAndLabelOrDescription(const QString& address, const QString& message){
    QString label = walletModel->getAddressTableModel()->labelForAddress(address);
    if (!label.isNull() && !label.isEmpty()){
        ui->lineEditDescription->setText(label);
    } else if(!message.isEmpty())
        ui->lineEditDescription->setText(message);
    setAddress(address);
}

void SendMultiRow::setLabel(const QString& label){
    ui->lineEditDescription->setText(label);
}

bool SendMultiRow::isClear(){
    return ui->lineEditAddress->text().isEmpty();
}

void SendMultiRow::setFocus(){
    ui->lineEditAddress->setFocus();
}

void SendMultiRow::setOnlyStakingAddressAccepted(bool onlyStakingAddress) {
    this->onlyStakingAddressAccepted = onlyStakingAddress;
}

void SendMultiRow::setOnlyLeasingAddressAccepted(bool onlyLeasingAddress) {
    this->onlyLeasingAddressAccepted = onlyLeasingAddress;
}

void SendMultiRow::setNumber(int _number){
    number = _number;
    iconNumber->setText(QString::number(_number));
}

void SendMultiRow::hideLabels(){
    ui->layoutLabel->setVisible(false);
    iconNumber->setVisible(true);
}

void SendMultiRow::showLabels(){
    ui->layoutLabel->setVisible(true);
    iconNumber->setVisible(false);
}

void SendMultiRow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}

void SendMultiRow::enterEvent(QEvent *) {
    if(!this->isExpanded && iconNumber->isVisible()){
        isExpanded = true;
        ui->btnMenu->setVisible(isExpanded);
    }
}

void SendMultiRow::leaveEvent(QEvent *) {
    if(isExpanded){
        isExpanded = false;
        ui->btnMenu->setVisible(isExpanded);
    }
}

int SendMultiRow::getMenuBtnWidth(){
    return ui->btnMenu->width();
}

SendMultiRow::~SendMultiRow(){
    delete ui;
}
