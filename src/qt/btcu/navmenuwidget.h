// Copyright (c) 2019-2020 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVMENUWIDGET_H
#define NAVMENUWIDGET_H

#include <QWidget>
#include "qt/btcu/pwidget.h"

class BTCUGUI;

namespace Ui {
class NavMenuWidget;
}

class NavMenuWidget : public PWidget
{
    Q_OBJECT

public:
    explicit NavMenuWidget(BTCUGUI* mainWindow, QWidget *parent = nullptr);
    ~NavMenuWidget();

    void loadWalletModel() override;
    virtual void showEvent(QShowEvent *event) override;

public Q_SLOTS:
    void selectSettings();
    void onShowHideColdStakingChanged(bool show);
    void onShowHideLeasingChanged(bool show);

private Q_SLOTS:
    void onSendClicked();
    void onDashboardClicked();
    void onPrivacyClicked();
    void onAddressClicked();
    void onMasterNodesClicked();
    void onColdStakingClicked();
    void onLeasingClicked();
    void onSettingsClicked();
    void onReceiveClicked();
    void updateButtonStyles();
private:
    Ui::NavMenuWidget *ui;
    QList<QWidget*> btns;

    void connectActions();
    void onNavSelected(QWidget* active, bool startup = false);

    bool init = false;
};

#endif // NAVMENUWIDGET_H
