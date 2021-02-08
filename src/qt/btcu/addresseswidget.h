// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ADDRESSESWIDGET_H
#define ADDRESSESWIDGET_H

#include "qt/btcu/pwidget.h"
#include "addresstablemodel.h"
#include "qt/btcu/tooltipmenu.h"
#include "furabstractlistitemdelegate.h"
#include "qt/btcu/addressfilterproxymodel.h"
#include "qt/btcu/addresslabelrow.h"

#include <QWidget>
#include <QSpacerItem>

class AddressViewDelegate;
class TooltipMenu;
class BTCUGUI;
class WalletModel;

namespace Ui {
class AddressesWidget;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class AddressesWidget : public PWidget
{
    Q_OBJECT

public:
    explicit AddressesWidget(BTCUGUI* parent);
    ~AddressesWidget();

    void loadWalletModel() override;
    void onNewContactClicked();

private Q_SLOTS:
    void handleAddressClicked(const QModelIndex &index);
    void onStoreContactClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onCopyClicked();
    void onAddContactShowHideClicked();
    void onpbnMenuClicked();

    void changeTheme(bool isLightTheme, QString &theme) override;
private:
    Ui::AddressesWidget *ui;

    FurAbstractListItemDelegate* delegate = nullptr;
    AddressTableModel* addressTablemodel = nullptr;
    AddressFilterProxyModel *filter = nullptr;

    bool isOnMyAddresses = true;
    TooltipMenu* menu = nullptr;
    QSpacerItem* SpacerAddresses = nullptr;
    AddressLabelRow* AddressesRow = nullptr;
    // Cached index
    QModelIndex index;

    void updateListView();
   void addRows();
    void updateAddresses();
};

#endif // ADDRESSESWIDGET_H
