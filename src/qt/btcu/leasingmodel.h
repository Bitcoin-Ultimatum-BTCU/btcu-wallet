// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LEASINGMODEL_H
#define LEASINGMODEL_H

#include <QAbstractTableModel>
#include "amount.h"

#include <memory>


class AddressTableModel;


class LeasingModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit LeasingModel(AddressTableModel* addressTableModel, QObject *parent = nullptr);
    ~LeasingModel();

    enum ColumnIndex {
        ADDRESS = 0,
        ADDRESS_LABEL = 1,
        TOTAL_AMOUNT = 2,
        TOTAL_REWARD = 3,
        IS_LEASER = 4,
        COLUMN_COUNT = 5
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    void removeRowAndEmitDataChanged(const int idx);
    void updateLeasingList();
    CAmount getTotalAmount() const;
    CAmount getTotalReward() const;

    void refresh();

public Q_SLOTS:
    void emitDataSetChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif // LEASINGMODEL_H
