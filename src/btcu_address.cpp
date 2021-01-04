// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2017-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "btcu_address.h"
#include "utilstrencodings.h"

namespace
{
    class CBTCUAddressVisitor : public boost::static_visitor<bool>
    {
    private:
        CBTCUAddress* addr;
        CChainParams::Base58Type type;

    public:
        CBTCUAddressVisitor(
            CBTCUAddress* addrIn,
            const CChainParams::Base58Type addrType = CChainParams::PUBKEY_ADDRESS)
        :
            addr(addrIn),
            type(addrType)
        {}

        bool operator()(const CKeyID& id) const { return addr->Set(id, type); }
        bool operator()(const CScriptID& id) const { return addr->Set(id); }
        bool operator()(const WitnessV0ScriptHash& id) const { return addr->Set(id); }
        bool operator()(const WitnessV0KeyHash& id) const { return addr->Set(id); }
        bool operator()(const WitnessUnknown& id) const { return addr->Set(id); }
        bool operator()(const CNoDestination& no) const { addr->Clear(); return false; }
    };

} // anon namespace

CBTCUAddress::CBTCUAddress(const CTxDestination& dest, const CChainParams::Base58Type addrType)
{
    Set(dest, addrType);
}

CBTCUAddress::CBTCUAddress(const char* pAddress)
{
    SetString(std::string(pAddress));
}

CBTCUAddress::CBTCUAddress(const std::string& strAddress)
{
    SetString(strAddress);
}

bool CBTCUAddress::SetString(const std::string& strAddress)
{
    if (base58.SetString(strAddress) && IsValidBase58Size() && IsValidBase58Version(Params())) {
        data_type = DataType::Base58;
    } else if (bech32.SetString(strAddress) && IsValidBech32Size()) {
        data_type = DataType::Bech32;
    } else {
        data_type = DataType::Unknown;
        return false;
    }
    return true;
}

bool CBTCUAddress::Set(const CKeyID& id, const CChainParams::Base58Type addrType)
{
    data_type = DataType::Base58;
    base58.SetData(Params().Base58Prefix(addrType), &id, 20);
    return true;
}

bool CBTCUAddress::Set(const CScriptID& id)
{
    data_type = DataType::Base58;
    base58.SetData(Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS), &id, 20);
    return true;
}

bool CBTCUAddress::Set(const WitnessV0ScriptHash& id)
{
    data_type = DataType::Bech32;
    bech32.SetData(0, &id, id.size());
    return true;
}

bool CBTCUAddress::Set(const WitnessV0KeyHash& id)
{
    data_type = DataType::Bech32;
    bech32.SetData(0, &id, id.size());
    return true;
}

bool CBTCUAddress::Set(const WitnessUnknown& id)
{
    data_type = DataType::Bech32;
    bech32.SetData(id.version, &id, id.length);
    return true;
}

bool CBTCUAddress::Set(const CTxDestination& dest, const CChainParams::Base58Type addrType)
{
    return boost::apply_visitor(CBTCUAddressVisitor(this, addrType), dest);
}

bool CBTCUAddress::IsValidBech32Size() const
{
    if (!IsValidBech32Version()) {
        return false;
    }

    if (bech32.nVersion == 0) {
        WitnessV0KeyHash wKeyHash;
        WitnessV0ScriptHash wScriptHash;

        if (bech32.vchData.size() == wKeyHash.size() || bech32.vchData.size() == wScriptHash.size()) {
            return true;
        }
    }

    if (bech32.vchData.size() < 2 || bech32.vchData.size() > 40) {
        return false;
    }

    return true;
}

bool CBTCUAddress::IsValidBech32Version() const
{
    return (bech32.nVersion >= 0 && bech32.nVersion <= 16);
}

bool CBTCUAddress::IsValidBase58Version(const CChainParams& params) const
{
    return
        base58.vchVersion == params.Base58Prefix(CChainParams::PUBKEY_ADDRESS) ||
        base58.vchVersion == params.Base58Prefix(CChainParams::SCRIPT_ADDRESS) ||
        base58.vchVersion == params.Base58Prefix(CChainParams::STAKING_ADDRESS);
}

bool CBTCUAddress::IsValidBase58Size() const
{
    return (base58.vchData.size() == 20);
}

bool CBTCUAddress::IsValid(const CChainParams& params) const
{
    if (data_type == DataType::Base58) {
        return IsValidBase58Size() && IsValidBase58Version(params);
    } else if (data_type == DataType::Bech32) {
        return IsValidBech32Size();
    }
    return false;
}

bool CBTCUAddress::IsValid() const
{
    return IsValid(Params());
}

void CBTCUAddress::Clear()
{
    data_type = DataType::Unknown;
    base58.Clear();
    bech32.Clear();
}

std::string CBTCUAddress::ToString() const
{
    if (data_type == DataType::Base58 && IsValidBase58Version(Params())) {
        return base58.ToString();
    } else if (data_type == DataType::Bech32 && IsValidBech32Size()) {
        return bech32.ToString();
    }
    return std::string();
}

CTxDestination CBTCUAddress::Get() const
{
    if (data_type == DataType::Base58 && IsValidBase58Size()) {
        uint160 id;
        memcpy(&id, &base58.vchData[0], 20);

        if (base58.vchVersion == Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS) ||
            base58.vchVersion == Params().Base58Prefix(CChainParams::STAKING_ADDRESS)) {
            return CKeyID(id);
        } else if (base58.vchVersion == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS)) {
            return CScriptID(id);
        }
    } else if (data_type == DataType::Bech32) {

        if (bech32.nVersion == 0) {
            WitnessV0KeyHash witnKeyHash;
            WitnessV0ScriptHash witnScriptHash;

            if (bech32.vchData.size() == witnKeyHash.size()) {
                std::copy(bech32.vchData.begin(), bech32.vchData.end(), witnKeyHash.begin());
                return witnKeyHash;
            } else if (bech32.vchData.size() == witnScriptHash.size()) {
                std::copy(bech32.vchData.begin(), bech32.vchData.end(), witnScriptHash.begin());
                return witnScriptHash;
            }
        }

        if (!IsValidBech32Version() || bech32.vchData.size() < 2 || bech32.vchData.size() > 40) {
            return CNoDestination();
        }

        WitnessUnknown witnUnk;
        witnUnk.version = bech32.nVersion;
        std::copy(bech32.vchData.begin(), bech32.vchData.end(), witnUnk.program);
        witnUnk.length = bech32.vchData.size();
        return witnUnk;
    }

    return CNoDestination();
}

bool CBTCUAddress::GetKeyID(CKeyID& keyID) const
{
    if (data_type != DataType::Base58 || !IsValidBase58Size()) {
        return false;
    }

    if (base58.vchVersion != Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS) &&
        base58.vchVersion != Params().Base58Prefix(CChainParams::STAKING_ADDRESS)) {
        return false;
    }

    uint160 id;
    memcpy(&id, &base58.vchData[0], 20);
    keyID = CKeyID(id);
    return true;
}

bool CBTCUAddress::IsScript() const
{
    if (data_type != DataType::Base58 || !IsValidBase58Size()) {
        return false;
    }

    return base58.vchVersion == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS);
}

bool CBTCUAddress::IsStakingAddress() const
{
    if (data_type != DataType::Base58 || !IsValidBase58Size()) {
        return false;
    }

    return base58.vchVersion == Params().Base58Prefix(CChainParams::STAKING_ADDRESS);
}

bool CBTCUAddress::IsLeasingAddress() const
{
    if (data_type != DataType::Base58 || !IsValidBase58Size()) {
        return false;
    }

    return base58.vchVersion == Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS);
}

int CBTCUAddress::CompareTo(const CBTCUAddress& ba) const
{
    if (ba.data_type == data_type) {
        if (data_type == DataType::Base58) {
            return base58.CompareTo(ba.base58);
        }
        return bech32.CompareTo(ba.bech32);
    }

    if (static_cast<int>(data_type) < static_cast<int>(ba.data_type)) {
        return -1;
    }
    return 1;
}

//
// CBTCUSecret
//

void CBTCUSecret::SetKey(const CKey& vchSecret)
{
    assert(vchSecret.IsValid());
    base58.SetData(Params().Base58Prefix(CChainParams::SECRET_KEY), vchSecret.begin(), vchSecret.size());
    if (vchSecret.IsCompressed()) {
        base58.vchData.push_back(1);
    }
}

CKey CBTCUSecret::GetKey()
{
    CKey ret;
    assert(base58.vchData.size() >= 32);
    const bool fCompressed = base58.vchData.size() > 32 && base58.vchData[32] == 1;
    ret.Set(base58.vchData.begin(), base58.vchData.begin() + 32, fCompressed);
    return ret;
}

bool CBTCUSecret::IsValid() const
{
    bool fExpectedFormat = base58.vchData.size() == 32 || (base58.vchData.size() == 33 && base58.vchData[32] == 1);
    bool fCorrectVersion = base58.vchVersion == Params().Base58Prefix(CChainParams::SECRET_KEY);
    return fExpectedFormat && fCorrectVersion;
}

bool CBTCUSecret::SetString(const std::string& strSecret)
{
    return base58.SetString(strSecret) && IsValid();
}

std::string CBTCUSecret::ToString() const
{
    return base58.ToString();
}