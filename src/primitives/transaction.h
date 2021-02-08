// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2015-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_TRANSACTION_H
#define BITCOIN_PRIMITIVES_TRANSACTION_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"
#include "tx_in_out.h"
#include "masternode-validators.h"

#include <list>
#include <memory>

class CTransaction;


struct CMutableTransaction;
struct CCoins;

/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class CTransaction
{
private:
    /** Memory only. */
    const uint256 hash;
    void UpdateHash() const;

protected:
    const bool fSaveHash = false;

public:
    static const int32_t BITCOIN_VERSION=1;
    static const int32_t BTCU_START_VERSION=2;
    static const int32_t CURRENT_VERSION=BTCU_START_VERSION;

    // The local variables are made const to prevent unintended modification
    // without updating the cached hash value. However, CTransaction is not
    // actually immutable; deserialization and assignment are implemented,
    // and bypass the constness. This is safe, as they update the entire
    // structure, including the hash.
    const int32_t nVersion;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    const uint32_t nLockTime;
    //const unsigned int nTime;

   // Operation codes
   enum OpCode
   {
      OpNone = 0,
      OpCall = 1,
      OpCreate = 2
   };


    /** Here vector is used as a wrapping object, that can be empty, if transaction doesn't contain CValidatorRegister or CValidatorVote */
    std::vector<CValidatorRegister> validatorRegister;
    std::vector<CValidatorVote> validatorVote;
    
    /** Construct a CTransaction that qualifies as IsNull() */
    CTransaction();

    /** Convert a CMutableTransaction into a CTransaction. */
    CTransaction(const CMutableTransaction &tx);

    /** Convert a CCoins into a CTransaction. For Bitcoin Genesis cases.*/
    CTransaction(const uint256& coinsHash, const CCoins& coins);

    CTransaction& operator=(const CTransaction& tx);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion){
        READWRITE(*const_cast<int32_t*>(&this->nVersion));
        nVersion = this->nVersion;
        READWRITE(*const_cast<std::vector<CTxIn>*>(&vin));
        READWRITE(*const_cast<std::vector<CTxOut>*>(&vout));
        READWRITE(*const_cast<uint32_t*>(&nLockTime));
        if (nVersion >= BTCU_START_VERSION && nVersion <= CURRENT_VERSION) {
            READWRITE(*const_cast<std::vector<CValidatorRegister>*>(&validatorRegister));
            READWRITE(*const_cast<std::vector<CValidatorVote>*>(&validatorVote));
        }
        if (fSaveHash && this->nVersion == CTransaction::BITCOIN_VERSION)
            READWRITE(*const_cast<uint256*>(&hash));
        else if (ser_action.ForRead())
            UpdateHash();
    };
    
    bool IsNull() const {
        return vin.empty() && vout.empty();
    }

    const uint256& GetHash() const {
        return hash;
    }

    // Return sum of txouts.
    CAmount GetValueOut() const;
    // GetValueIn() is a method on CCoinsViewCache, because
    // inputs must be known to compute value in.

    // Compute priority, given priority of inputs and (optionally) tx size
    double ComputePriority(double dPriorityInputs, unsigned int nTxSize=0) const;

    // Compute modified tx size for priority calculation (optionally given tx size)
    unsigned int CalculateModifiedSize(unsigned int nTxSize=0) const;

    bool HasZerocoinSpendInputs() const;
    bool HasZerocoinPublicSpendInputs() const;

    bool HasZerocoinMintOutputs() const;

    bool ContainsZerocoins() const
    {
        return HasZerocoinSpendInputs() || HasZerocoinPublicSpendInputs() || HasZerocoinMintOutputs();
    }

    CAmount GetZerocoinMinted() const;
    CAmount GetZerocoinSpent() const;
    int GetZerocoinMintCount() const;

    bool UsesUTXO(const COutPoint out);
    std::list<COutPoint> GetOutPoints() const;

   //////////////////////////////////////// // qtum
   bool HasCreateOrCall() const;
   bool HasOpSpend() const;
////////////////////////////////////////
   bool HasOpCreate() const;
   bool HasOpCall() const;
   inline int GetCreateOrCall() const
   {
      return (HasOpCall() ? OpCode::OpCall : 0) + (HasOpCreate() ? OpCode::OpCreate : 0);
   }
   bool HasOpSender() const;


    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull() && !ContainsZerocoins());
    }

    bool IsCoinStake() const;
    bool IsValidatorRegister() const;
    bool IsValidatorVote() const;
    bool IsLeasingReward() const;
    bool CheckColdStake(const CScript& script) const;
    bool HasP2CSOutputs() const;
    bool HasP2LOutputs() const;

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return a.hash != b.hash;
    }

    unsigned int GetTotalSize() const;

    std::string ToString() const;

};

/** A mutable version of CTransaction. */
struct CMutableTransaction
{
    int32_t nVersion;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    uint32_t nLockTime;
    
    std::vector<CValidatorRegister> validatorRegister;
    std::vector<CValidatorVote> validatorVote;
    
    CMutableTransaction();
    CMutableTransaction(const CTransaction& tx);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(vin);
        READWRITE(vout);
        READWRITE(nLockTime);
        if (nVersion >= CTransaction::BTCU_START_VERSION && nVersion <= CTransaction::CURRENT_VERSION) {
            READWRITE(validatorRegister);
            READWRITE(validatorVote);
        }
    }

    /** Compute the hash of this CMutableTransaction. This is computed on the
     * fly, as opposed to GetHash() in CTransaction, which uses a cached result.
     */
    uint256 GetHash() const;
    bool HasOpSender() const;
    std::string ToString() const;
};

typedef std::shared_ptr<const CTransaction> CTransactionRef;
static inline CTransactionRef MakeTransactionRef() { return std::make_shared<const CTransaction>(); }
template <typename Tx> static inline CTransactionRef MakeTransactionRef(Tx&& txIn) { return std::make_shared<const CTransaction>(std::forward<Tx>(txIn)); }

#endif // BITCOIN_PRIMITIVES_TRANSACTION_H
