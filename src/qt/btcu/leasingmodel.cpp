// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/leasingmodel.h"
#include "uint256.h"
#include "bitcoinunits.h"
#include "guiutil.h"
#include "addressbook.h"
#include "addresstablemodel.h"
#include "transactionrecord.h"
#include "walletmodel.h"

#include <iostream>



struct Leasing {
    Leasing() = default;

    CKeyID leaserKeyID;
    QString leaserAddress;
    bool isLeaser = false;

    CKeyID ownerKeyID;
    QString ownerAddress;
    bool isOwner = false;

    /// Map of txId --> index num for P2L utxos
    std::map<std::string, int> p2lUtxos;
    /// Map of txId --> index num for leasing reward utxos
    std::map<std::string, int> lrUtxos;

    /// Sum of all leasing to this owner address
    CAmount cachedTotalAmount = 0;
    /// Sum of all leasing rewards
    CAmount cachedTotalReward = 0;
};



struct LeasingModel::Impl {
    Impl(AddressTableModel* addressTableModel): pAddressTableModel(addressTableModel)
    { }

    ~Impl() = default;

    AddressTableModel* pAddressTableModel = nullptr;

    /**
     * List with all of the grouped leasings received by this wallet
     */
    std::map<CKeyID, Leasing> cachedLeasings;
    CAmount cachedAmount = 0;
    CAmount cachedReward = 0;

    bool parseP2L(const COutput& utxo, Leasing& leasing) {
        txnouttype type;
        std::vector<CTxDestination> addresses;
        int nRequired;
        const auto& out = utxo.tx->vout[utxo.i];

        if (!ExtractDestinations(out.scriptPubKey, type, addresses, nRequired) || type != TX_LEASE || addresses.size() != 2)
            return error("%s : Error extracting P2L destinations for utxo: %s-%d", __func__, utxo.tx->GetHash().GetHex(), utxo.i);

        leasing.leaserKeyID = boost::get<CKeyID>(addresses[0]);
        leasing.leaserAddress = QString::fromStdString(CBTCUAddress(leasing.leaserKeyID, CChainParams::PUBKEY_ADDRESS).ToString());
        leasing.isLeaser = pwalletMain->HaveKey(leasing.leaserKeyID);

        leasing.ownerKeyID = boost::get<CKeyID>(addresses[1]);
        leasing.ownerAddress = QString::fromStdString(CBTCUAddress(leasing.ownerKeyID, CChainParams::PUBKEY_ADDRESS).ToString());
        leasing.isOwner = pwalletMain->HaveKey(leasing.ownerKeyID);

        return true;
    }

    bool parseLR(const COutput& utxo, Leasing& leasing) {
        txnouttype type;
        std::vector<CTxDestination> addresses;
        int nRequired;
        const auto& out = utxo.tx->vout[utxo.i];

        if (!ExtractDestinations(out.scriptPubKey, type, addresses, nRequired) || type != TX_LEASINGREWARD || addresses.size() != 1)
            return error("%s : Error extracting LR destination for utxo: %s-%d", __func__, utxo.tx->GetHash().GetHex(), utxo.i);;

        leasing.ownerKeyID = boost::get<CKeyID>(addresses[0]);
        leasing.ownerAddress = QString::fromStdString(CBTCUAddress(leasing.ownerKeyID, CChainParams::PUBKEY_ADDRESS).ToString());
        leasing.isOwner = true;
        return true;
    }

    void refresh() {
        cachedLeasings.clear();
        cachedAmount = 0;
        // First get all of the p2l utxo inside the wallet
        std::vector<COutput> utxoP2LList;
        pwalletMain->GetAvailableP2LCoins(utxoP2LList, false);
        // Second get all of leasing rewards utxo inside the wallet
        std::vector<COutput> utxoLRList;
        pwalletMain->GetAvailableLeasingRewards(utxoLRList);

        for (const auto& utxo : utxoP2LList) {
            Leasing leasing;
            if (!this->parseP2L(utxo, leasing))
                continue;

            const auto& out = utxo.tx->vout[utxo.i];
            const auto& txID = utxo.tx->GetHash().GetHex();

            if (leasing.isLeaser) {
                auto itr = cachedLeasings.emplace(leasing.leaserKeyID, leasing).first;
                (*itr).second.cachedTotalAmount += out.nValue;
                (*itr).second.p2lUtxos.emplace(txID, utxo.i);
            }

            if (leasing.isOwner) {
                auto itr = cachedLeasings.emplace(leasing.ownerKeyID, leasing).first;
                (*itr).second.cachedTotalAmount += out.nValue;
                (*itr).second.p2lUtxos.emplace(txID, utxo.i);
            }

            cachedAmount += out.nValue;
        }

        // Loop over each COutput into a Leasing
        for (const auto& utxo : utxoLRList) {
            Leasing leasing;
            if (!parseLR(utxo, leasing))
                continue;

            const auto& out = utxo.tx->vout[utxo.i];
            const auto& txID = utxo.tx->GetHash().GetHex();

            auto itr = cachedLeasings.emplace(leasing.ownerKeyID, leasing).first;

            (*itr).second.cachedTotalReward += out.nValue;
            (*itr).second.lrUtxos.emplace(txID, utxo.i);

            cachedReward += out.nValue;
        }
    }
};



LeasingModel::LeasingModel(
    AddressTableModel* addressTableModel,
    QObject *parent
) :
    QAbstractTableModel(parent),
    pImpl(new Impl(addressTableModel))
{ }

LeasingModel::~LeasingModel() = default;

void LeasingModel::updateLeasingList() {
    refresh();
    QMetaObject::invokeMethod(this, "emitDataSetChanged", Qt::QueuedConnection);
}

CAmount LeasingModel::getTotalAmount() const {
    return pImpl->cachedAmount;
}

CAmount LeasingModel::getTotalReward() const {
    return pImpl->cachedReward;
}

void LeasingModel::emitDataSetChanged() {
    Q_EMIT dataChanged(
        index(0, 0, QModelIndex()),
        index(pImpl->cachedLeasings.size(), COLUMN_COUNT, QModelIndex()) );
}

void LeasingModel::refresh() {
    pImpl->refresh();
}

int LeasingModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return pImpl->cachedLeasings.size();
}

int LeasingModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return COLUMN_COUNT;
}

QVariant LeasingModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
            return QVariant();

    int row = index.row();
    auto itr = pImpl->cachedLeasings.begin();
    std::advance(itr, row);
    const Leasing& rec = (*itr).second;
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
            case ADDRESS:
                if (rec.isLeaser)
                    return rec.leaserAddress;
                else
                    return rec.ownerAddress;
            case ADDRESS_LABEL:
                if (rec.isLeaser)
                    return pImpl->pAddressTableModel->labelForAddress(rec.leaserAddress);
                else
                    return pImpl->pAddressTableModel->labelForAddress(rec.ownerAddress);
            case TOTAL_AMOUNT:
                return GUIUtil::formatBalance(rec.cachedTotalAmount);
            case TOTAL_REWARD:
                return GUIUtil::formatBalance(rec.cachedTotalReward);
            case IS_LEASER:
                return rec.isLeaser;
        }
    }

    return QVariant();
}

void LeasingModel::removeRowAndEmitDataChanged(const int idx)
{
    beginRemoveRows(QModelIndex(), idx, idx);
    endRemoveRows();
    Q_EMIT dataChanged(
        index(idx, 0, QModelIndex()),
        index(idx, COLUMN_COUNT, QModelIndex()) );
}

