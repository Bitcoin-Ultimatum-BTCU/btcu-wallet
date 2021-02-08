// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/sendconfirmdialog.h"
#include "qt/btcu/forms/ui_sendconfirmdialog.h"
#include "bitcoinunits.h"
#include "walletmodel.h"
#include "transactiontablemodel.h"
#include "transactionrecord.h"
#include "wallet/wallet.h"
#include "guiutil.h"
#include "qt/btcu/qtutils.h"
#include <QList>
#include <QDateTime>

TxDetailDialog::TxDetailDialog(QWidget *parent, bool isConfirmDialog, QString warningStr) :
    QDialog(parent),
    ui(new Ui::TxDetailDialog)
{
    ui->setupUi(this);

    this->setStyleSheet(parent->styleSheet());

    // Container
    setCssProperty(ui->frame, "container-border");
   //setCssTitleScreen(ui->labelTitle);
    ui->labelTitle->setProperty("cssClass", "text-title-screen");



    setCssProperty(ui->scrollArea, "container");
    setCssProperty(ui->scrollAreaWidgetContents, "container");
    // Labels
    setCssProperty(ui->labelWarning, "text-title2-dialog");
    setCssProperty(ui->labelSend, "text-body1-dialog-uncheck");
    setCssProperty(ui->textSend, "text-body1-dialog-uncheck");
    setCssProperty(ui->labelInputs, "text-body1-dialog-uncheck");
    setCssProperty(ui->textInputs, "text-body1-dialog-uncheck");
    setCssTextBodyDialog({ui->labelAmount, /*ui->labelSend, ui->labelInputs,*/ ui->labelFee, ui->labelChange, ui->labelId, ui->labelSize, ui->labelStatus, ui->labelConfirmations, ui->labelDate});
    setCssProperty({ui->labelDivider, ui->labelDivider1, ui->labelDivider2, ui->labelDivider3, ui->labelDivider4, ui->labelDivider5, ui->labelDivider6, ui->labelDivider7, ui->labelDivider8, ui->labelDivider9}, "container-divider");
    setCssTextBodyDialog({ui->textAmount, /*ui->textSend, ui->textInputs,*/ ui->textFee, ui->textChange, ui->textId, ui->textSize, ui->textStatus, ui->textConfirmations, ui->textDate});

    setCssProperty(ui->pushCopy, "ic-copy-big");
    setCssProperty({ui->pushInputs, ui->pushOutputs}, "ic-arrow-down");
    setCssProperty(ui->btnEsc, "ic-close");

    setCssProperty(ui->outputsScrollArea, "container");
    setCssProperty(ui->container_outputs_base, "container");
    ui->labelWarning->setVisible(false);
    ui->gridInputs->setVisible(false);
    ui->outputsScrollArea->setVisible(false);
    ui->contentChangeAddress->setVisible(false);
    ui->labelDivider4->setVisible(false);

    setCssProperty({ui->labelOutputIndex, ui->labelTitlePrevTx}, "text-body2-dialog");

    if(isConfirmDialog){
        ui->labelTitle->setText(tr("Confirm Your Transaction"));
        setCssProperty(ui->btnCancel, "btn-dialog-cancel");
        ui->btnSave->setText(tr("SEND"));
        setCssBtnPrimary(ui->btnSave);
        if (!warningStr.isEmpty()) {
            ui->labelWarning->setVisible(true);
            ui->labelWarning->setText(warningStr);
        } else {
            ui->labelWarning->setVisible(false);
        }

        // hide change address for now
        ui->contentConfirmations->setVisible(false);
        ui->contentStatus->setVisible(false);
        ui->contentDate->setVisible(false);
        ui->contentSize->setVisible(false);
        ui->contentConfirmations->setVisible(false);
        ui->contentID->setVisible(false);
        ui->labelDivider7->setVisible(false);
        ui->labelDivider5->setVisible(false);
        ui->labelDivider3->setVisible(false);
        ui->labelDivider9->setVisible(false);

        connect(ui->btnCancel, SIGNAL(clicked()), this, SLOT(close()));
        connect(ui->btnSave, &QPushButton::clicked, [this](){acceptTx();});
    }else{
        ui->labelTitle->setText(tr("Transaction Details"));
        ui->containerButtons->setVisible(false);
    }

    connect(ui->btnEsc, SIGNAL(clicked()), this, SLOT(closeDialog()));
    connect(ui->pushInputs, SIGNAL(clicked()), this, SLOT(onInputsClicked()));
    connect(ui->pushOutputs, SIGNAL(clicked()), this, SLOT(onOutputsClicked()));
}

void TxDetailDialog::setData(WalletModel *model, const QModelIndex &index){
    this->model = model;
    TransactionRecord *rec = static_cast<TransactionRecord*>(index.internalPointer());
    QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
    QString address = index.data(Qt::DisplayRole).toString();
    qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
    QString amountText = BitcoinUnits::formatWithUnit(nDisplayUnit, amount, true, BitcoinUnits::separatorAlways);
    ui->textAmount->setText(amountText);

    const CWalletTx* tx = model->getTx(rec->hash);
    if(tx) {
        this->txHash = rec->hash;
        QString hash = QString::fromStdString(tx->GetHash().GetHex());
        ui->textId->setText(hash.left(20) + "..." + hash.right(20));
        ui->textId->setTextInteractionFlags(Qt::TextSelectableByMouse);
        if (tx->vout.size() == 1) {
            ui->textSend->setText(address);
        } else {
            ui->textSend->setText(QString::number(tx->vout.size()) + " recipients");
        }

        if (tx->IsLeasingReward()) {
            int cnt = 0;
            for (const CTxOut &out : tx->vout) {
                if (model->isMine(out.scriptPubKey)) ++cnt;
            }
            ui->textInputs->setText(QString::number(cnt));
        } else
            ui->textInputs->setText(QString::number(tx->vin.size()));
        ui->textConfirmations->setText(QString::number(rec->status.depth));
        ui->textDate->setText(GUIUtil::dateTimeStrWithSeconds(date));
        ui->textStatus->setText(QString::fromStdString(rec->statusToString()));
        ui->textSize->setText(QString::number(rec->size) + " bytes");

        connect(ui->pushCopy, &QPushButton::clicked, [this](){
            GUIUtil::setClipboard(QString::fromStdString(this->txHash.GetHex()));
            Q_EMIT messageInfo(tr("ID copied"), CClientUIInterface::MSG_WARNING_SNACK);
            /*if (!snackBar) snackBar = new SnackBar(nullptr, this);
            snackBar->setText(tr("ID copied"));
            openDialogDropRight(this->snackBar, this);*/

        });
    }

}

void TxDetailDialog::setData(WalletModel *model, WalletModelTransaction &tx){
    this->model = model;
    this->tx = &tx;
    CAmount txFee = tx.getTransactionFee();
    CAmount totalAmount = tx.getTotalTransactionAmount() + txFee;

    ui->textAmount->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, totalAmount, false, BitcoinUnits::separatorAlways) + " (Fee included)");
    if(tx.getRecipients().size() == 1){
        ui->textSend->setText(tx.getRecipients().at(0).address);
        ui->pushOutputs->setVisible(false);
    }else{
        ui->textSend->setText(QString::number(tx.getRecipients().size()) + " recipients");
    }
    ui->textInputs->setText(QString::number(tx.getTransaction()->vin.size()));
    ui->textFee->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, txFee, false, BitcoinUnits::separatorAlways));
}

void TxDetailDialog::acceptTx(){
    this->confirm = true;
    this->sendStatus = model->sendCoins(*this->tx);
    accept();
}

void TxDetailDialog::onInputsClicked() {
    if (ui->gridInputs->isVisible()) {
        ui->gridInputs->setVisible(false);
        ui->contentInputs->layout()->setContentsMargins(0,9,12,9);
        setCssProperty(ui->pushInputs, "ic-arrow-down");
        setCssProperty(ui->labelInputs, "text-body1-dialog-uncheck");
        setCssProperty(ui->textInputs, "text-body1-dialog-uncheck");
        updateStyle(ui->pushInputs);
        updateStyle(ui->labelInputs);
        updateStyle(ui->textInputs);
    } else {
        ui->gridInputs->setVisible(true);
        ui->contentInputs->layout()->setContentsMargins(0,9,12,0);
        setCssProperty(ui->pushInputs, "ic-arrow-up");
        setCssProperty(ui->labelInputs, "text-body1-dialog-check");
        setCssProperty(ui->textInputs, "text-body1-dialog-check");
        updateStyle(ui->labelInputs);
        updateStyle(ui->textInputs);
        updateStyle(ui->pushInputs);
        if (!inputsLoaded) {
            inputsLoaded = true;
            const CWalletTx* tx = (this->tx) ? this->tx->getTransaction() : model->getTx(this->txHash);
            if(tx) {
                int i = 1;

                auto fillOut = [&](const uint256& txHash, const uint32_t n) {
                    QString hash = QString::fromStdString(txHash.GetHex());
                    QLabel *label = new QLabel(hash.left(18) + "..." + hash.right(18));
                    QLabel *label1 = new QLabel(QString::number(n));
                    label1->setAlignment(Qt::AlignCenter);
                    setCssProperty({label, label1}, "text-body2-dialog");

                    ui->gridLayoutInput->addWidget(label,i,0);
                    ui->gridLayoutInput->addWidget(label1,i,1, Qt::AlignCenter);
                    i++;
                };

                if (tx->IsLeasingReward()) for (const CTxOut &out : tx->vout) {
                    if (!model->isMine(out.scriptPubKey)) continue;

                    std::vector<valtype> vSolutions;
                    txnouttype whichType;
                    if (Solver(out.scriptPubKey, whichType, vSolutions) && TX_LEASINGREWARD == whichType)
                        fillOut(uint256(vSolutions[0]), CScriptNum(vSolutions[1], true).getint());

                } else for (const CTxIn &in : tx->vin) {
                    fillOut(in.prevout.hash, in.prevout.n);
                }

                ui->gridInputs->setMinimumHeight(50 + (50 * (i - 1)));
            }
        }
    }
}

void TxDetailDialog::onOutputsClicked() {
    if (ui->outputsScrollArea->isVisible()) {
        ui->outputsScrollArea->setVisible(false);
        setCssProperty(ui->pushOutputs, "ic-arrow-down");
        setCssProperty(ui->labelSend, "text-body1-dialog-uncheck");
        setCssProperty(ui->textSend, "text-body1-dialog-uncheck");
        updateStyle(ui->pushOutputs);
        updateStyle(ui->labelSend);
        updateStyle(ui->textSend);
    } else {
        ui->outputsScrollArea->setVisible(true);
        setCssProperty(ui->pushOutputs, "ic-arrow-up");
        setCssProperty(ui->labelSend, "text-body1-dialog-check");
        setCssProperty(ui->textSend, "text-body1-dialog-check");
        updateStyle(ui->pushOutputs);
        updateStyle(ui->labelSend);
        updateStyle(ui->textSend);
        if (!outputsLoaded) {
            outputsLoaded = true;
            QVBoxLayout* layoutVertical = new QVBoxLayout();
            layoutVertical->setContentsMargins(10,0,42,0);
            layoutVertical->setSpacing(6);
            ui->container_outputs_base->setLayout(layoutVertical);

            const CWalletTx* tx = (this->tx) ? this->tx->getTransaction() : model->getTx(this->txHash);
            if(tx) {
                for (const CTxOut &out : tx->vout) {
                    QFrame *frame = new QFrame(ui->container_outputs_base);

                    QHBoxLayout *layout = new QHBoxLayout();
                    layout->setContentsMargins(0, 0, 0, 0);
                    layout->setSpacing(12);
                    frame->setLayout(layout);

                    QLabel *label = nullptr;
                    QString labelRes;
                    CTxDestination dest;
                    bool isCsAddress = out.scriptPubKey.IsPayToColdStaking();
                    bool isLAddress = out.scriptPubKey.IsPayToLeasing();
                    if (ExtractDestination(out.scriptPubKey, dest, isCsAddress, isLAddress)) {
                        std::string address = [&]() -> CBTCUAddress {
                            if (isCsAddress) {
                                return CBTCUAddress::newCSInstance(dest);
                            } else if (isLAddress) {
                                return CBTCUAddress::newLInstance(dest);
                            } else {
                                return CBTCUAddress::newInstance(dest);
                            }
                        }().ToString();
                        labelRes = QString::fromStdString(address);
                        labelRes = labelRes.left(16) + "..." + labelRes.right(16);
                    } else {
                        labelRes = tr("Unknown");
                    }
                    label = new QLabel(labelRes);
                    QLabel *label1 = new QLabel(BitcoinUnits::formatWithUnit(nDisplayUnit, out.nValue, false, BitcoinUnits::separatorAlways));
                    label1->setAlignment(Qt::AlignCenter | Qt::AlignRight);
                    setCssProperty({label, label1}, "text-body2-dialog");

                    layout->addWidget(label);
                    layout->addWidget(label1);
                    layoutVertical->addWidget(frame);
                }
            }
        }
    }
}

void TxDetailDialog::closeDialog(){
    if(snackBar && snackBar->isVisible()) snackBar->hide();
    close();
}

TxDetailDialog::~TxDetailDialog(){
    if(snackBar) delete snackBar;
    delete ui;
}
