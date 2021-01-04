// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COINCONTROLBTCUWIDGET_H
#define COINCONTROLBTCUWIDGET_H

#include <QDialog>

namespace Ui {
class CoinControlBTCUWidget;
}

class CoinControlBTCUWidget : public QDialog
{
    Q_OBJECT

public:
    explicit CoinControlBTCUWidget(QWidget *parent = nullptr);
    ~CoinControlBTCUWidget();

private:
    Ui::CoinControlBTCUWidget *ui;
};

#endif // COINCONTROLBTCUWIDGET_H
