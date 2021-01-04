// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2015-2019 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCU_TX_IN_OUT_H
#define BTCU_TX_IN_OUT_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"

#include <list>

class CTransaction;

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    uint32_t n;
    
    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, uint32_t nIn) { hash = hashIn; n = nIn; }
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(FLATDATA(*this));
    }
    
    void SetNull() { hash.SetNull(); n = (uint32_t) -1; }
    bool IsNull() const { return (hash.IsNull() && n == (uint32_t) -1); }
    bool IsMasternodeReward(const CTransaction* tx) const;

    void SetLeasingReward(const uint32_t nHeight) { hash.SetNull(); n = nHeight; }
    bool IsLeasingReward() const { return (hash.IsNull() && n > 0); }
    
    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
    }
    
    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }
    
    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }
    
    std::string ToString() const;
    std::string ToStringShort() const;
    
    uint256 GetHash();
    
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;
    CScript prevPubKey;
    
    CTxIn()
    {
        nSequence = std::numeric_limits<unsigned int>::max();
    }
    
    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=std::numeric_limits<unsigned int>::max());
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=std::numeric_limits<uint32_t>::max());
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(prevout);
        READWRITE(scriptSig);
        READWRITE(nSequence);
    }
    
    bool IsFinal() const
    {
        return (nSequence == std::numeric_limits<uint32_t>::max());
    }
    
    bool IsZerocoinSpend() const;
    bool IsZerocoinPublicSpend() const;
    bool IsLeasingReward() const { return prevout.IsLeasingReward(); }
    
    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }
    
    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }
    
    // Needed for sorting
    friend bool operator>(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout.hash > b.prevout.hash);
    }
    
    friend bool operator<(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout.hash < b.prevout.hash);
    }
    
    std::string ToString() const;
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
public:
    CAmount nValue;
    CScript scriptPubKey;
    int nRounds;
    
    CTxOut()
    {
        SetNull();
    }
    
    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn);
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nValue);
        READWRITE(scriptPubKey);
    }
    
    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
        nRounds = -10; // an initial value, should be no way to get this by calculations
    }
    
    bool IsNull() const
    {
        return (nValue == -1);
    }
    
    void SetEmpty()
    {
        nValue = 0;
        scriptPubKey.clear();
    }
    
    bool IsEmpty() const
    {
        return (nValue == 0 && scriptPubKey.empty());
    }
    
    uint256 GetHash() const;

    CAmount GetMinNotDustSize(const CFeeRate& minRelayTxFee) const
    {
        // "Dust" is defined in terms of CTransaction::minRelayTxFee, which has units ubtcu-per-kilobyte.
        // If you'd pay more than 1/3 in fees to spend something, then we consider it dust.
        // A typical txout is 34 bytes big, and will need a CTxIn of at least 148 bytes to spend
        // i.e. total is 148 + 32 = 182 bytes. Default -minrelaytxfee is 100 ubtcu per kB
        // and that means that fee per txout is 182 * 100 / 1000 = 18.2 ubtcu.
        // So dust is a txout less than 18.2 *3 = 54.6 ubtcu
        // with default -minrelaytxfee = minRelayTxFee = 100 ubtcu per kB.
        size_t nSize = GetSerializeSize(SER_DISK,0)+148u;
        return 3*minRelayTxFee.GetFee(nSize);
    }

    bool IsDust(CFeeRate minRelayTxFee) const
    {
        // "Dust" is defined in terms of CTransaction::minRelayTxFee, which has units ubtcu-per-kilobyte.
        // If you'd pay more than 1/3 in fees to spend something, then we consider it dust.
        // A typical txout is 34 bytes big, and will need a CTxIn of at least 148 bytes to spend
        // i.e. total is 148 + 32 = 182 bytes. Default -minrelaytxfee is 100 ubtcu per kB
        // and that means that fee per txout is 182 * 100 / 1000 = 18.2 ubtcu.
        // So dust is a txout less than 18.2 *3 = 54.6 ubtcu
        // with default -minrelaytxfee = minRelayTxFee = 10000 ubtcu per kB.
        size_t nSize = GetSerializeSize(SER_DISK,0)+148u;
        return (nValue < 3*minRelayTxFee.GetFee(nSize));
    }

    bool IsPayToLeasing() const;
    bool IsLeasingReward() const;
    bool IsZerocoinMint() const;
    CAmount GetZerocoinMinted() const;
    
    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey &&
                a.nRounds      == b.nRounds);
    }
    
    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }
    
    std::string ToString() const;
};

#endif //BTCU_TX_IN_OUT_H
