// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TXROW_H
#define TXROW_H

#include <QWidget>
#include <QDateTime>
#include "transactionrecord.h"
#include <QGraphicsDropShadowEffect>

namespace Ui {
class TxRow;
}

class TxRow : public QWidget
{
    Q_OBJECT

public:
    explicit TxRow(QWidget *parent = nullptr);
    ~TxRow();

    void init(bool isLightTheme);
    void updateStatus(bool isLightTheme, bool isHover, bool isSelected);

    void setDate(QDateTime);
    void setLabel(QString);
    void setAmount(QString);
    void setType(bool isLightTheme, int type, bool isConfirmed);
    void setConfirmStatus(bool isConfirmed);

private:
    Ui::TxRow *ui;
    bool isConfirmed = false;
    QGraphicsDropShadowEffect* shadowEffect =nullptr;
};

#endif // TXROW_H
