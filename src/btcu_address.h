// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2017-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCU_ADDRESS_H
#define BTCU_ADDRESS_H

#include "base58.h"
#include "bech32.h"

/** base58/bech32-encoded BTCU addresses.
 * Public-key-hash-addresses have version 0 (or 111 testnet).
 * The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
 * Script-hash-addresses have version 5 (or 196 testnet).
 * The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
 */
class CBTCUAddress
{
    enum class DataType: int {
        Unknown = 0,
        Base58,
        Bech32
    };

    DataType data_type = DataType::Unknown;
    CBase58Data base58;
    CBech32Data bech32;

public:
    bool SetString(const std::string& strAddress);
    bool Set(const CKeyID& id, const CChainParams::Base58Type addrType = CChainParams::PUBKEY_ADDRESS);
    bool Set(const CScriptID& id);
    bool Set(const WitnessV0ScriptHash& id);
    bool Set(const WitnessV0KeyHash& id);
    bool Set(const WitnessUnknown& id);
    bool Set(const CTxDestination& dest, const CChainParams::Base58Type addrType = CChainParams::PUBKEY_ADDRESS);
    void Clear();
    bool IsValid(const CChainParams& params) const;
    bool IsValid() const;

    CBTCUAddress() = default;
    ~CBTCUAddress() = default;
    CBTCUAddress(const CTxDestination& dest, const CChainParams::Base58Type addrType = CChainParams::PUBKEY_ADDRESS);
    CBTCUAddress(const char* pAddress);
    CBTCUAddress(const std::string& strAddress);


    std::string ToString() const;

    CTxDestination Get() const;
    bool GetKeyID(CKeyID& keyID) const;
    bool IsScript() const;
    bool IsStakingAddress() const;
    bool IsLeasingAddress() const;


    // Helpers
    static const CBTCUAddress newCSInstance(const CTxDestination& dest) {
        return CBTCUAddress(dest, CChainParams::STAKING_ADDRESS);
    }

    static const CBTCUAddress newLInstance(const CTxDestination& dest) {
        return CBTCUAddress(dest, CChainParams::PUBKEY_ADDRESS);
    }

    static const CBTCUAddress newInstance(const CTxDestination& dest) {
        return CBTCUAddress(dest, CChainParams::PUBKEY_ADDRESS);
    }

    int CompareTo(const CBTCUAddress& ba) const;

    bool operator==(const CBTCUAddress& ba) const { return CompareTo(ba) == 0; }
    bool operator< (const CBTCUAddress& ba) const { return CompareTo(ba) <  0; }

protected:
    bool IsValidBase58Size() const;
    bool IsValidBase58Version(const CChainParams& params) const;
    bool IsValidBech32Size() const;
    bool IsValidBech32Version() const;
};

/**
 * A base58-encoded secret key
 */
class CBTCUSecret
{
    CBase58Data base58;

public:
    void SetKey(const CKey& vchSecret);
    CKey GetKey();
    bool IsValid() const;
    bool SetString(const std::string& strSecret);

    CBTCUSecret(const CKey& vchSecret) { SetKey(vchSecret); }
    CBTCUSecret() = default;
    ~CBTCUSecret() = default;

    std::string ToString() const;
};

#endif // BTCU_ADDRESS_H

