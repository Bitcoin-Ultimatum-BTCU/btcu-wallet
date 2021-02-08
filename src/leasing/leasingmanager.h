// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCU_LEASING_MANAGER_H
#define BTCU_LEASING_MANAGER_H

#if defined(HAVE_CONFIG_H)
#include "config/btcu-config.h"
#endif

#ifdef ENABLE_LEASING_MANAGER

#include "primitives/transaction.h"
#include "primitives/block.h"

#include "validationinterface.h"

#include <memory>

#define LeasingLogPrint(fmt, ...) LogPrint("leasing", "Leasing - (%d)%s: " fmt "\n", __LINE__, __func__, __VA_ARGS__)
#define LeasingError(fmt, ...) error("Leasing - (%d)%s: " fmt, __LINE__, __func__, __VA_ARGS__)



enum class LeaserType: int {
    ValidatorNode = 1,
    MasterNode = 2
};


class CLeasingManager: public CValidationInterface {
public:
    CLeasingManager();
    ~CLeasingManager();

    bool GetLeasingRewards(const LeaserType type, const CKeyID& leaserID, const size_t nLimit, std::vector<CTxOut>& vRewards) const;
    CTxOut CalcLeasingReward(const COutPoint& point, const CKeyID& keyID) const;
    void GetAllAmountsLeasedTo(CPubKey &pubKey, CAmount &amount) const;

    const uint256& GetBlockHash() const;
    
protected:
    void UpdatedBlockTip(const CBlockIndex*)  override;
    void SyncTransaction(const CTransaction& tx, const CBlock* pBlock) override;

private:
    class CImpl;
    std::unique_ptr<CImpl> pImpl;
};



#endif // ENABLE_LEASING_MANAGER

#endif // BTCU_LEASING_MANAGER_H