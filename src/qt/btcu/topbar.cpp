// Copyright (c) 2019-2020 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/topbar.h"
#include "qt/btcu/forms/ui_topbar.h"
#include <QPixmap>
#include "qt/btcu/lockunlock.h"
#include "qt/btcu/qtutils.h"
#include "qt/btcu/receivedialog.h"
#include "askpassphrasedialog.h"

#include "bitcoinunits.h"
#include "clientmodel.h"
#include "qt/guiconstants.h"
#include "qt/guiutil.h"
#include "optionsmodel.h"
#include "qt/platformstyle.h"
#include "wallet/wallet.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "guiinterface.h"
#include "qt/btcu/cbtokenrow.h"
#include "qt/btcu/cbtokenmodel.h"


#include "addresstablemodel.h"
#include <QStandardItem>

TopBar::TopBar(BTCUGUI* _mainWindow, QWidget *parent) :
    PWidget(_mainWindow, parent),
    ui(new Ui::TopBar)
{
    ui->setupUi(this);
    showBottom();
    init();
    // Set parent stylesheet
    this->setStyleSheet(_mainWindow->styleSheet());
    /* Containers */
    this->setContentsMargins(2, 0, 2, 3);
    //setShadow(this);
#ifdef Q_OS_MAC
    //ui->containerTop->load("://bg-dashboard-banner");
    //setCssProperty(ui->containerTop,"container-topbar-no-image");
#else
    //ui->containerTop->setProperty("cssClass", "container-border-topbar");

    //this->setProperty("cssClass", "container-border");
    //ui->containerTop->setVisible(false);
    ui->top_container_2->setProperty("cssClass", "container-top");
#endif

    std::initializer_list<QWidget*> lblTitles = {ui->labelTitle1,  ui->labelTitle3, ui->labelTitle4, ui->labelTitle5, ui->labelTitle6,
                                                 ui->labelPendingPiv, ui->labelLeasing, ui->labelImmaturePiv, ui->labelImmaturezPiv};
    setCssProperty(lblTitles, "text-title-topbar");
    QFont font;
    font.setWeight(QFont::Light);
    Q_FOREACH (QWidget* w, lblTitles) { w->setFont(font); }

    // Amount information top
    //ui->widgetTopAmount->setVisible(false);
    //setCssProperty({ui->labelAmountTopPiv, ui->labelAmountTopzPiv}, "amount-small-topbar");
    //setCssProperty({ui->labelAmountPiv/*, ui->labelAmountzPiv*/}, "amount-topbar");
    setCssProperty({ui->labelAmountPiv}, "amount-small-topbar");




   //ui->lineEditTocens->setPlaceholderText(tr("My tocens"));
   ui->lineEditTocens->setGraphicsEffect(nullptr);
   ui->lineEditTocens->setText(tr("My Tokens"));
   btnOwnerTocen = ui->lineEditTocens->addAction(QIcon("://ic-contact-arrow-down"), QLineEdit::TrailingPosition);
   ui->lineEditTocens->installEventFilter(this);

   setCssProperty(ui->lineEditTocens, "edit-primary-multi-book");
   //ui->lineEditOwnerAddress->setAttribute(Qt::WA_MacShowFocusRect, 0);
   setShadow(ui->lineEditTocens);
   connect(btnOwnerTocen, &QAction::triggered, [this](){ onTocensClicked(); });
   ui->lineEditTocens->setReadOnly(true);
   // Progress Sync
    progressBar = new QProgressBar(ui->layoutSync);
    progressBar->setRange(1, 10);
    progressBar->setValue(4);
    progressBar->setTextVisible(false);
    progressBar->setMaximumHeight(2);
    progressBar->setMaximumWidth(36);
    setCssProperty(progressBar, "progress-sync");
    progressBar->show();
    progressBar->raise();
    progressBar->move(0, 34);

    // New button
    ui->pushButtonFAQ->setButtonClassStyle("cssClass", "btn-check-faq");
    ui->pushButtonFAQ->setButtonText(tr("FAQ"));

    ui->pushButtonConnection->setButtonClassStyle("cssClass", "btn-check-connect-inactive");
    ui->pushButtonConnection->setButtonText(tr("No Connection"));

    ui->pushButtonStack->setButtonClassStyle("cssClass", "btn-check-stack-inactive");
    ui->pushButtonStack->setButtonText(tr("Staking Disabled"));
    ui->pushButtonStack->setVisible(false);

    ui->pushButtonColdStaking->setButtonClassStyle("cssClass", "btn-check-cold-staking-inactive");
    ui->pushButtonColdStaking->setButtonText(tr("Cold Staking Disabled"));
    ui->pushButtonColdStaking->setVisible(false);

    ui->pushButtonLeasing->setButtonClassStyle("cssClass", "btn-check-leasing-inactive");
    ui->pushButtonLeasing->setButtonText(tr("Leasing Disabled"));
   ui->pushButtonLeasing->setVisible(false);

    ui->pushButtonSync->setButtonClassStyle("cssClass", "btn-check-sync");
    ui->pushButtonSync->setButtonText(tr(" %54 Synchronizing.."));

    ui->pushButtonLock->setButtonClassStyle("cssClass", "btn-check-lock");

    if(isLightTheme()){
        ui->pushButtonTheme->setButtonClassStyle("cssClass", "btn-check-theme-light");
        ui->pushButtonTheme->setButtonText(tr("Light Theme"));
    }else{
        ui->pushButtonTheme->setButtonClassStyle("cssClass", "btn-check-theme-dark");
        ui->pushButtonTheme->setButtonText(tr("Dark Theme"));
    }
   ui->pushButtonTheme->setVisible(false);

    //setCssProperty(ui->qrContainer, "container-qr");
    //setCssProperty(ui->pushButtonQR, "btn-qr");

    // QR image
    //QPixmap pixmap("://img-qr-test");
    /*ui->btnQr->setIcon(
                QIcon(pixmap.scaled(
                         70,
                         70,
                         Qt::KeepAspectRatio))
                );*/

    ui->pushButtonLock->setButtonText(tr("Wallet Locked  "));
    ui->pushButtonLock->setButtonClassStyle("cssClass", "btn-check-status-lock");


    //connect(ui->pushButtonQR, SIGNAL(clicked()), this, SLOT(onBtnReceiveClicked()));
    //connect(ui->btnQr, SIGNAL(clicked()), this, SLOT(onBtnReceiveClicked()));
    connect(ui->pushButtonLock, SIGNAL(Mouse_Pressed()), this, SLOT(onBtnLockClicked()));
    connect(ui->pushButtonTheme, SIGNAL(Mouse_Pressed()), this, SLOT(onThemeClicked()));
    connect(ui->pushButtonFAQ, SIGNAL(Mouse_Pressed()), _mainWindow, SLOT(openFAQ()));
    connect(ui->pushButtonColdStaking, SIGNAL(Mouse_Pressed()), this, SLOT(onColdStakingClicked()));
    connect(ui->pushButtonLeasing, SIGNAL(Mouse_Pressed()), this, SLOT(onLeasingClicked()));
    connect(ui->pushButtonSync, &ExpandableButton::Mouse_HoverLeave, this, &TopBar::refreshProgressBarSize);
    connect(ui->pushButtonSync, &ExpandableButton::Mouse_Hover, this, &TopBar::refreshProgressBarSize);
    //connect(ui->comboBoxTokens, SIGNAL(currentIndexChanged(int)), this, SLOT(ChangedTokens(int)));
    connect(this, SIGNAL(SaveOptionsTokens()), parent, SLOT(onSaveOptionsClicked()));
}

void TopBar::onThemeClicked(){
    // Store theme
    bool lightTheme = !isLightTheme();

    setTheme(lightTheme);

    if(lightTheme){
        ui->pushButtonTheme->setButtonClassStyle("cssClass", "btn-check-theme-light",  true);
        ui->pushButtonTheme->setButtonText(tr("Light Theme"));
    }else{
        ui->pushButtonTheme->setButtonClassStyle("cssClass", "btn-check-theme-dark", true);
        ui->pushButtonTheme->setButtonText(tr("Dark Theme"));
    }
    updateStyle(ui->pushButtonTheme);

    Q_EMIT themeChanged(lightTheme);
}


void TopBar::onBtnLockClicked(){
    if(walletModel) {
        if (walletModel->getEncryptionStatus() == WalletModel::Unencrypted) {
            encryptWallet();
        } else {
            if (!lockUnlockWidget) {
                lockUnlockWidget = new LockUnlock(window);
                lockUnlockWidget->setStyleSheet("margin:0px; padding:0px;");
                connect(lockUnlockWidget, SIGNAL(Mouse_Leave()), this, SLOT(lockDropdownMouseLeave()));
                connect(ui->pushButtonLock, &ExpandableButton::Mouse_HoverLeave, [this](){
                    QMetaObject::invokeMethod(this, "lockDropdownMouseLeave", Qt::QueuedConnection);
                });
                connect(lockUnlockWidget, SIGNAL(lockClicked(
                const StateClicked&)),this, SLOT(lockDropdownClicked(
                const StateClicked&)));
            }

            lockUnlockWidget->updateStatus(walletModel->getEncryptionStatus());
            if (ui->pushButtonLock->width() <= 40) {
                ui->pushButtonLock->setExpanded();
            }
            // Keep it open
            ui->pushButtonLock->setKeepExpanded(true);
            QMetaObject::invokeMethod(this, "openLockUnlock", Qt::QueuedConnection);
        }
    }
}

void TopBar::openLockUnlock(){
    lockUnlockWidget->setFixedWidth(ui->pushButtonLock->width());
    lockUnlockWidget->adjustSize();

    lockUnlockWidget->move(
            ui->pushButtonLock->pos().rx() + window->getNavWidth() + 10,
            ui->pushButtonLock->y() + 36
    );

    lockUnlockWidget->raise();
    lockUnlockWidget->activateWindow();
    lockUnlockWidget->show();
}

void TopBar::encryptWallet() {
    if (!walletModel)
        return;

    showHideOp(true);
    AskPassphraseDialog *dlg = new AskPassphraseDialog(AskPassphraseDialog::Mode::Encrypt, window,
                            walletModel, AskPassphraseDialog::Context::Encrypt);
    dlg->adjustSize();
    openDialogWithOpaqueBackgroundY(dlg, window);

    refreshStatus();
    dlg->deleteLater();
}

static bool isExecuting = false;
void TopBar::lockDropdownClicked(const StateClicked& state){
    lockUnlockWidget->close();
    if(walletModel && !isExecuting) {
        isExecuting = true;

        switch (lockUnlockWidget->lock) {
            case 0: {
                if (walletModel->getEncryptionStatus() == WalletModel::Locked)
                    break;
                walletModel->setWalletLocked(true);
                ui->pushButtonLock->setButtonText(tr("Wallet Locked"));
                ui->pushButtonLock->setButtonClassStyle("cssClass", "btn-check-status-lock", true);
                // Directly update the staking status icon when the wallet is manually locked here
                // so the feedback is instant (no need to wait for the polling timeout)
                setStakingStatusActive(false);
                break;
            }
            case 1: {
                if (walletModel->getEncryptionStatus() == WalletModel::Unlocked)
                    break;
                showHideOp(true);
                AskPassphraseDialog *dlg = new AskPassphraseDialog(AskPassphraseDialog::Mode::Unlock, window, walletModel,
                                        AskPassphraseDialog::Context::ToggleLock);
                dlg->adjustSize();
                openDialogWithOpaqueBackgroundY(dlg, window);
                if (walletModel->getEncryptionStatus() == WalletModel::Unlocked) {
                    ui->pushButtonLock->setButtonText(tr("Wallet Unlocked"));
                    ui->pushButtonLock->setButtonClassStyle("cssClass", "btn-check-status-unlock", true);
                }
                dlg->deleteLater();
                break;
            }
            case 2: {
                WalletModel::EncryptionStatus status = walletModel->getEncryptionStatus();
                if (status == WalletModel::UnlockedForAnonymizationOnly)
                    break;

                if (status == WalletModel::Unlocked) {
                    walletModel->lockForStakingOnly();
                } else {
                    showHideOp(true);
                    AskPassphraseDialog *dlg = new AskPassphraseDialog(AskPassphraseDialog::Mode::UnlockAnonymize,
                                                                       window, walletModel,
                                                                       AskPassphraseDialog::Context::ToggleLock);
                    dlg->adjustSize();
                    openDialogWithOpaqueBackgroundY(dlg, window);
                    dlg->deleteLater();
                }
                if (walletModel->getEncryptionStatus() == WalletModel::UnlockedForAnonymizationOnly) {
                    ui->pushButtonLock->setButtonText(tr("Wallet Unlocked for staking"));
                    ui->pushButtonLock->setButtonClassStyle("cssClass", "btn-check-status-staking", true);
                }
                break;
            }
        }

        ui->pushButtonLock->setKeepExpanded(false);
        ui->pushButtonLock->setSmall();
        ui->pushButtonLock->update();

        isExecuting = false;
    }
}

void TopBar::lockDropdownMouseLeave(){
    if (lockUnlockWidget->isVisible() && !lockUnlockWidget->isHovered()) {
        lockUnlockWidget->hide();
        ui->pushButtonLock->setKeepExpanded(false);
        ui->pushButtonLock->setSmall();
        ui->pushButtonLock->update();
    }
}

void TopBar::onBtnReceiveClicked(){
    if(walletModel) {
        QString addressStr = walletModel->getAddressTableModel()->getAddressToShow();
        if (addressStr.isNull()) {
            inform(tr("Error generating address"));
            return;
        }
        showHideOp(true);
        ReceiveDialog *receiveDialog = new ReceiveDialog(window);
        receiveDialog->updateQr(addressStr);
        if (openDialogWithOpaqueBackground(receiveDialog, window)) {
            inform(tr("Address Copied"));
        }
        receiveDialog->deleteLater();
    }
}

void TopBar::showTop(){
    /*if(ui->bottom_container->isVisible()){
        ui->bottom_container->setVisible(false);
        ui->widgetTopAmount->setVisible(true);
        this->setFixedHeight(75);
    }*/
   /*if(ui->bottom_container->isVisible()){
      //ui->bottom_container->setVisible(false);
      ui->widgetTopAmount->setVisible(false);
      this->setFixedHeight(100);
   }*/
   //this->setFixedHeight(100);
   //this->adjustSize();
}

void TopBar::showBottom(){
    ui->widgetTopAmount->setVisible(true);
    ui->bottom_container->setVisible(true);
    //ui->qrContainer->setVisible(false);
    //this->setFixedHeight(100);
    //this->adjustSize();
}

void TopBar::onColdStakingClicked() {

    bool isColdStakingEnabled = walletModel->isColdStaking();
    ui->pushButtonColdStaking->setChecked(isColdStakingEnabled);

    bool show = (isInitializing) ? walletModel->getOptionsModel()->isColdStakingScreenEnabled() :
            walletModel->getOptionsModel()->invertColdStakingScreenStatus();
    QString className;
    QString text;

    if (isColdStakingEnabled) {
        text = tr("Cold Staking Active");
        className = (show) ? "btn-check-cold-staking-checked" : "btn-check-cold-staking-unchecked";
    } else if (show) {
        className = "btn-check-cold-staking";
        text = tr("Cold Staking Enabled");
    } else {
        className = "btn-check-cold-staking-inactive";
        text = tr("Cold Staking Disabled");
    }

    ui->pushButtonColdStaking->setButtonClassStyle("cssClass", className, true);
    ui->pushButtonColdStaking->setButtonText(text);
    updateStyle(ui->pushButtonColdStaking);

    Q_EMIT onShowHideColdStakingChanged(show);
}

void TopBar::onLeasingClicked() {

    bool isLeasingEnabled = walletModel->isLeasing();
    ui->pushButtonLeasing->setChecked(isLeasingEnabled);

    bool show = (isInitializing) ? walletModel->getOptionsModel()->isLeasingScreenEnabled() :
                walletModel->getOptionsModel()->invertLeasingScreenStatus();
    QString className;
    QString text;

    if (isLeasingEnabled) {
        text = tr("Leasing Active");
        className = (show) ? "btn-check-leasing-checked" : "btn-check-leasing-unchecked";
    } else if (show) {
        className = "btn-check-leasing";
        text = tr("Leasing Enabled");
    } else {
        className = "btn-check-leasing-inactive";
        text = tr("Leasing Disabled");
    }

    ui->pushButtonLeasing->setButtonClassStyle("cssClass", className, true);
    ui->pushButtonLeasing->setButtonText(text);
    updateStyle(ui->pushButtonLeasing);

    Q_EMIT onShowHideLeasingChanged(show);
}

TopBar::~TopBar(){
    if(timerStakingIcon){
        timerStakingIcon->stop();
    }
    delete ui;
}

void TopBar::loadClientModel(){
    if(clientModel){
        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks());
        connect(clientModel, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));

        timerStakingIcon = new QTimer(ui->pushButtonStack);
        connect(timerStakingIcon, SIGNAL(timeout()), this, SLOT(updateStakingStatus()));
        timerStakingIcon->start(50000);
        updateStakingStatus();
    }
}

void TopBar::setStakingStatusActive(bool fActive)
{
    if (ui->pushButtonStack->isChecked() != fActive) {
        ui->pushButtonStack->setButtonText(fActive ? tr("Staking active") : tr("Staking not active"));
        ui->pushButtonStack->setChecked(fActive);
        ui->pushButtonStack->setButtonClassStyle("cssClass", (fActive ?
                                                                "btn-check-stack" :
                                                                "btn-check-stack-inactive"), true);
    }
}
void TopBar::updateStakingStatus(){
    setStakingStatusActive(walletModel &&
                           !walletModel->isWalletLocked() &&
                           walletModel->isStakingStatusActive());
}

void TopBar::setNumConnections(int count) {
    if(count > 0){
        if(!ui->pushButtonConnection->isChecked()) {
            ui->pushButtonConnection->setChecked(true);
            ui->pushButtonConnection->setButtonClassStyle("cssClass", "btn-check-connect", true);
        }
    }else{
        if(ui->pushButtonConnection->isChecked()) {
            ui->pushButtonConnection->setChecked(false);
            ui->pushButtonConnection->setButtonClassStyle("cssClass", "btn-check-connect-inactive", true);
        }
    }

    ui->pushButtonConnection->setButtonText(tr("%n active connection(s)", "", count));
}

void TopBar::setNumBlocks(int count) {
// TODO: Disabling macOS App Nap on initial sync, disk and reindex operations.

    if (!clientModel)
        return;

    // Acquire current block source
    enum BlockSource blockSource = clientModel->getBlockSource();
    std::string text = "";
    switch (blockSource) {
        case BLOCK_SOURCE_NETWORK:
            text = "Synchronizing..";
            break;
        case BLOCK_SOURCE_DISK:
            text = "Importing blocks from disk..";
            break;
        case BLOCK_SOURCE_REINDEX:
            text = "Reindexing blocks on disk..";
            break;
        case BLOCK_SOURCE_NONE:
            // Case: not Importing, not Reindexing and no network connection
            text = "No block source available..";
            ui->pushButtonSync->setChecked(false);
            break;
    }

    bool needState = true;
    if (masternodeSync.IsBlockchainSynced()) {
        // chain synced
        Q_EMIT walletSynced(true);
        if (masternodeSync.IsSynced()) {
            // Node synced
            ui->pushButtonSync->setButtonText(tr("Synchronized - Block: %1").arg(QString::number(count)));
            progressBar->setRange(0,100);
            progressBar->setValue(100);
            return;
        } else {

            // TODO: Show out of sync warning
            int nAttempt = masternodeSync.RequestedMasternodeAttempt < MASTERNODE_SYNC_THRESHOLD ?
                       masternodeSync.RequestedMasternodeAttempt + 1 :
                       MASTERNODE_SYNC_THRESHOLD;
            int progress = nAttempt + (masternodeSync.RequestedMasternodeAssets - 1) * MASTERNODE_SYNC_THRESHOLD;
            if(progress >= 0){
                // todo: MN progress..
                text = strprintf("Synchronizing masternodes data... - Block: %d", count);
                //progressBar->setMaximum(4 * MASTERNODE_SYNC_THRESHOLD);
                //progressBar->setValue(progress);
                needState = false;
            }
        }
    } else {
        Q_EMIT walletSynced(false);
    }

    if(needState) {
        // Represent time from last generated block in human readable text
        QDateTime lastBlockDate = clientModel->getLastBlockDate();
        QDateTime currentDate = QDateTime::currentDateTime();
        int secs = lastBlockDate.secsTo(currentDate);

        QString timeBehindText;
        const int HOUR_IN_SECONDS = 60 * 60;
        const int DAY_IN_SECONDS = 24 * 60 * 60;
        const int WEEK_IN_SECONDS = 7 * 24 * 60 * 60;
        const int YEAR_IN_SECONDS = 31556952; // Average length of year in Gregorian calendar
        if (secs < 2 * DAY_IN_SECONDS) {
            timeBehindText = tr("%n hour(s)", "", secs / HOUR_IN_SECONDS);
        } else if (secs < 2 * WEEK_IN_SECONDS) {
            timeBehindText = tr("%n day(s)", "", secs / DAY_IN_SECONDS);
        } else if (secs < YEAR_IN_SECONDS) {
            timeBehindText = tr("%n week(s)", "", secs / WEEK_IN_SECONDS);
        } else {
            int years = secs / YEAR_IN_SECONDS;
            int remainder = secs % YEAR_IN_SECONDS;
            timeBehindText = tr("%1 and %2").arg(tr("%n year(s)", "", years)).arg(
                    tr("%n week(s)", "", remainder / WEEK_IN_SECONDS));
        }
        QString timeBehind(" behind. Scanning block ");
        QString str = timeBehindText + timeBehind + QString::number(count);
        text = str.toStdString();

        progressBar->setMaximum(1000000000);
        progressBar->setValue(clientModel->getVerificationProgress() * 1000000000.0 + 0.5);
    }

    if(text.empty()){
        text = "No block source available..";
    }

    ui->pushButtonSync->setButtonText(tr(text.data()));
}

void TopBar::loadWalletModel(){
    connect(walletModel, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)), this,
            SLOT(updateBalances(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));
    connect(walletModel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    connect(walletModel, &WalletModel::encryptionStatusChanged, this, &TopBar::refreshStatus);

    leasingModel.reset(new LeasingModel(walletModel->getAddressTableModel(), this));

    // update the display unit, to not use the default ("BTCU")
    updateDisplayUnit();

    refreshStatus();
    onColdStakingClicked();
    onLeasingClicked();

    isInitializing = false;
}

void TopBar::refreshStatus(){
    // Check lock status
    if (!this->walletModel)
        return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    switch (encStatus){
        case WalletModel::EncryptionStatus::Unencrypted:
            ui->pushButtonLock->setButtonText(tr("Wallet Unencrypted"));
            ui->pushButtonLock->setButtonClassStyle("cssClass", "btn-check-status-unlock", true);
            break;
        case WalletModel::EncryptionStatus::Locked:
            ui->pushButtonLock->setButtonText(tr("Wallet Locked"));
            ui->pushButtonLock->setButtonClassStyle("cssClass", "btn-check-status-lock", true);
            break;
        case WalletModel::EncryptionStatus::UnlockedForAnonymizationOnly:
            ui->pushButtonLock->setButtonText(tr("Wallet Unlocked for staking"));
            ui->pushButtonLock->setButtonClassStyle("cssClass", "btn-check-status-staking", true);
            break;
        case WalletModel::EncryptionStatus::Unlocked:
            ui->pushButtonLock->setButtonText(tr("Wallet Unlocked"));
            ui->pushButtonLock->setButtonClassStyle("cssClass", "btn-check-status-unlock", true);
            break;
    }
    updateStyle(ui->pushButtonLock);
}

void TopBar::updateDisplayUnit()
{
    if (walletModel && walletModel->getOptionsModel()) {
        int displayUnitPrev = nDisplayUnit;
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if (displayUnitPrev != nDisplayUnit)
        {
           updateBalances(walletModel->getBalance(), walletModel->getUnconfirmedBalance(),
                          walletModel->getImmatureBalance(), walletModel->getZerocoinBalance(),
                          leasingModel->getTotalAmount()/*walletModel->getUnconfirmedZerocoinBalance()*/, walletModel->getImmatureZerocoinBalance(),
                          walletModel->getWatchBalance(), walletModel->getWatchUnconfirmedBalance(),
                          walletModel->getWatchImmatureBalance(), walletModel->getDelegatedBalance(),
                          walletModel->getColdStakedBalance());
           //ui->comboBoxTokens->setCurrentIndex(nDisplayUnit);

        }
    }
}

void TopBar::updateBalances(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                            const CAmount& zerocoinBalance, const CAmount& unconfirmedZerocoinBalance, const CAmount& immatureZerocoinBalance,
                            const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance,
                            const CAmount& delegatedBalance, const CAmount& coldStakedBalance){

    // Locked balance. //TODO move this to the signal properly in the future..
    CAmount nLockedBalance = 0;
    if (walletModel) {
        nLockedBalance = walletModel->getLockedBalance();
    }
    ui->labelTitle1->setText(nLockedBalance > 0 ? tr("Available (Locked included)") : tr("Available"));

    // BTCU Total
    CAmount btcuAvailableBalance = balance;
    // zBTCU Balance
    CAmount matureZerocoinBalance = zerocoinBalance - unconfirmedZerocoinBalance - immatureZerocoinBalance;

    // Set
    QString totalPiv = GUIUtil::formatBalance(btcuAvailableBalance, nDisplayUnit);
    QString totalzPiv = GUIUtil::formatBalance(matureZerocoinBalance, nDisplayUnit, true);
    // Top
    //ui->labelAmountTopPiv->setText(totalPiv);
    //ui->labelAmountTopzPiv->setText(totalzPiv);

    // Expanded
    ui->labelAmountPiv->setText(totalPiv);
    //ui->labelAmountzPiv->setText(totalzPiv);

    ui->labelPendingPiv->setText(GUIUtil::formatBalance(unconfirmedBalance, nDisplayUnit));
    ui->labelLeasing->setText(GUIUtil::formatBalance(leasingModel->getTotalAmount()/*unconfirmedZerocoinBalance*/, nDisplayUnit, true));

    ui->labelImmaturePiv->setText(GUIUtil::formatBalance(immatureBalance, nDisplayUnit));
    ui->labelImmaturezPiv->setText(GUIUtil::formatBalance(immatureZerocoinBalance, nDisplayUnit, true));
}

void TopBar::resizeEvent(QResizeEvent *event){
    if (lockUnlockWidget && lockUnlockWidget->isVisible()) lockDropdownMouseLeave();
    QWidget::resizeEvent(event);
}

void TopBar::refreshProgressBarSize() {
    QMetaObject::invokeMethod(this, "expandSync", Qt::QueuedConnection);
}

void TopBar::expandSync() {
    if (progressBar) {
        progressBar->setMaximumWidth(ui->pushButtonSync->maximumWidth());
        progressBar->setFixedWidth(ui->pushButtonSync->width());
        progressBar->setMinimumWidth(ui->pushButtonSync->width() - 2);
    }
}

void TopBar::init()
{
   /*CBTokenRow * cbRow = new CBTokenRow();
   cbRow->setText("Afiikiticate (AUC)", "0.32313");
   CBTokenRow * cbRow1 = new CBTokenRow();
   cbRow1->setText("Afiikiticate1 (AUC)", "0.32313");
   CBTokenModel *CBModel = new CBTokenModel();
   CBModel->AddRow(cbRow);
   CBModel->AddRow(cbRow1);
   QPoint pos = ui->lineEditTocens->pos();
   CbTocenDropdown * cbtd = new CbTocenDropdown(310,300, this);
   cbtd->setTokenModel(CBModel,"");
   cbtd->move(pos);*/

}


void TopBar::onTocensClicked()
{
   int height;
   int width;
   QPoint pos;

   height = ui->lineEditTocens->height();
   width = ui->lineEditTocens->width();
   pos = ui->lineEditTocens->pos();
   pos.setY(pos.y() + height + 10);
   pos.setX(pos.x() + ui->lineEditTocens->rect().bottomRight().x() + 110 - width);


   if(!menuTocen){
      menuTocen = new CbTocenDropdown(
      width,
      height,
      window
      );

      CBTokenRow * cbRow = new CBTokenRow();
      cbRow->setText("Afiikiticate (AUC)", "0.32313");
      CBTokenRow * cbRow2 = new CBTokenRow();
      cbRow2->setText("Afiikiticate2 (AUC)", "0.32313");
      CBTokenRow * cbRow3 = new CBTokenRow();
      cbRow3->setText("Afiikiticate3 (AUC)", "0.32313");
      CBTokenRow * cbRow4 = new CBTokenRow();
      cbRow4->setText("Afiikiticate4 (AUC)", "0.32313");
      CBTokenRow * cbRow5 = new CBTokenRow();
      cbRow5->setText("Afiikiticate5 (AUC)", "0.32313");
      CBTokenRow * cbRow1 = new CBTokenRow();
      cbRow1->setText("Afiikiticate1 (AUC)", "0.32313");
      CBModel = new CBTokenModel();
      CBModel->AddRow(cbRow);
      CBModel->AddRow(cbRow1);
      CBModel->AddRow(cbRow2);
      /*CBModel->AddRow(cbRow3);
      CBModel->AddRow(cbRow4);
      CBModel->AddRow(cbRow5);*/
      menuTocen->setTokenModel(CBModel,"");
      connect(menuTocen, &CbTocenDropdown::contactSelected, [this](QString Tocen,  QPixmap pixmap){
         ui->lineEditTocens->setText(Tocen);
         btnOwnerTocen->setIcon(QIcon("://ic-contact-arrow-down"));
         //QPixmap p =pixmap.scaled(13, 13,Qt::KeepAspectRatioByExpanding);
         if(!icoTocen)
         {
            icoTocen = ui->lineEditTocens->addAction(QIcon("://img-tokens-svg"), QLineEdit::LeadingPosition);
            ui->lineEditTocens->setStyleSheet("padding-left: 0px;");
         }else
         {
            icoTocen->setIcon(QIcon("://img-tokens-svg"));
         }
      });
      connect(menuTocen, &CbTocenDropdown::TimerHide, [this](){
         hideMenuTocen();
      });
   }

   if(menuTocen->isVisible()){
      menuTocen->hide();
      btnOwnerTocen->setIcon(QIcon("://ic-contact-arrow-down"));
      return;
   }
   btnOwnerTocen->setIcon(QIcon("://ic-contact-arrow-up"));
   int Count = CBModel->rowCount();
   height = Count < 5 ? 25 + (Count * 30) : 25 + 5* 30;

   menuTocen->resizeList(width, height);
   menuTocen->setStyleSheet(styleSheet());
   menuTocen->adjustSize();
   menuTocen->move(pos);
   menuTocen->show();

}

void TopBar::ChangedTokens(int current)
{
   if (walletModel && walletModel->getOptionsModel())
   {
      if(current != walletModel->getOptionsModel()->getDisplayUnit())
      {
         walletModel->getOptionsModel()->setDisplayUnit(current);
         SaveOptionsTokens();

      }
   }

}

bool TopBar::eventFilter(QObject *obj, QEvent *event)
{
   QEvent::Type type = event->type();
   if (obj ==  ui->lineEditTocens) {
      if  (type == QEvent::HoverLeave) {
         QTimer::singleShot(100, this, SLOT(hideMenuTocen()));
      } else if (type == QEvent::HoverEnter)
      {
         if(!menuTocen || !menuTocen->isVisible()) onTocensClicked();
      }
   }/*else if(menuTocen && menuTocen->isVisible() && obj ==  menuTocen)
   {
      if  (type == QEvent::HoverLeave) {
         QTimer::singleShot(1000, this, SLOT(hideMenuTocen()));
      }
   }*/

   return QWidget::eventFilter(obj, event);
}

void TopBar::hideMenuTocen()
{
   if(!menuTocen->underMouse() && !ui->lineEditTocens->underMouse() && menuTocen->isVisible())
   {
      onTocensClicked();
   }
}