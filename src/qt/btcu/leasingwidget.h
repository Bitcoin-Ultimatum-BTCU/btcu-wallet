// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LEASINGWIDGET_H
#define LEASINGWIDGET_H

#include "qt/btcu/pwidget.h"
#include "qt/btcu/furabstractlistitemdelegate.h"
#include "qt/btcu/txviewholder.h"
#include "qt/btcu/tooltipmenu.h"
#include "qt/btcu/sendmultirow.h"
#include "qt/btcu/leasingmodel.h"
#include "qt/btcu/contactsdropdown.h"
#include "qt/btcu/addressholder.h"
#include "transactiontablemodel.h"
#include "addresstablemodel.h"
#include "addressfilterproxymodel.h"
#include "coincontroldialog.h"

#include <QAction>
#include <QLabel>
#include <QWidget>
#include <QSpacerItem>
#include <atomic>
#include <memory>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QPushButton>

class BTCUGUI;
class WalletModel;
class LeasingHolder;

namespace Ui {
class LeasingWidget;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class LeasingWidget : public PWidget
{
    Q_OBJECT

public:
    explicit LeasingWidget(BTCUGUI* parent);
    ~LeasingWidget();

    void loadWalletModel() override;
    void run(int type) override;
    void onError(QString error, int type) override;

public Q_SLOTS:
    void walletSynced(bool sync);

private Q_SLOTS:
    void changeTheme(bool isLightTheme, QString &theme) override;
    void handleAddressClicked(const QModelIndex &index);
    void handleMyLeasingAddressClicked(const QModelIndex &rIndex);
    void onCoinControlClicked();
    void onLeasingClicked();
    void updateDisplayUnit();
    void showList(bool show);
    void onSendClicked();
    void onLeaseSelected(bool lease);
    void onCopyOwnerClicked();
    void onAddressCopyClicked();
    void onAddressEditClicked();
    void onTxArrived(const QString& hash, const bool& isLeasing, const bool& isCSAnyType, const bool& isLAnyType);
    void onContactsClicked(bool ownerAdd);
    void onContactsClicked();
    void clearAll();
    void onLabelClicked();
    void onMyLeasingAddressesClicked();
    void onLeasingsRefreshed();
    void onTempADD(QString Address, QString Name, QString Amount);
    void onpbnCONFIRM();
    void onpbnMenuClicked();
    void onNewLeasingClicked();
    void onMoreInformationClicked();

private:
    std::unique_ptr<Ui::LeasingWidget> ui;
    FurAbstractListItemDelegate *leasing = nullptr;
    FurAbstractListItemDelegate *addressLeasing = nullptr;
    TransactionTableModel* txModel = nullptr;
    std::unique_ptr<AddressHolder> addressHolder;
    AddressTableModel* addressTableModel = nullptr;
    AddressFilterProxyModel *addressesFilter = nullptr;
    std::unique_ptr<LeasingModel> leasingModel;
    std::unique_ptr<LeasingHolder> txHolder;
    CoinControlDialog *coinControlDialog = nullptr;
    QAction *btnOwnerContact = nullptr;
    QAction *btnUpOwnerContact = nullptr;
    std::unique_ptr<QSpacerItem> spacerDiv;

    bool isInLeasing = true;
    bool isLeasingAddressListVisible = false;

    ContactsDropdown *menuContacts = nullptr;
    TooltipMenu* menu = nullptr;
    TooltipMenu* menuAddresses = nullptr;
    std::unique_ptr<SendMultiRow> sendMultiRow;
    bool isShowingDialog = false;
    bool isChainSync = false;

    bool isContactOwnerSelected;
    int64_t lastRefreshTime = 0;
    std::atomic<bool> isLoading;

    // Cached index
    QModelIndex index;
    QModelIndex addressIndex;

    int nDisplayUnit;
   QSpacerItem* SpacerHistory = nullptr;
   QSpacerItem* SpacerTop = nullptr;
    // temp
      bool bShowHistory = false;
      void showHistory();
      int n = 0;
      QString curentAddress;
      QString curentName;
    //

    void showAddressGenerationDialog();
    void tryRefreshLeasings();
    bool refreshLeasings();
    void onLabelClicked(QString dialogTitle, const QModelIndex &index, const bool& isMyLeasingAddresses);
    void updateLeasingTotalLabel();
   QWidget* createLeasinghistoryrow(QString Address, QString Name, QString Amount);
   QWidget* createLeasingTop(int Nun, QString Address);
};

#endif // LEASINGWIDGET_H
