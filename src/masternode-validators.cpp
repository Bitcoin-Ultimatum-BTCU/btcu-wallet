// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2015-2019 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternode-validators.h"
#include "timedata.h"
#include "util.h"

bool CSigner::SignHash(const uint256& hash, const CKey& key, std::vector<unsigned char>& vchSigRet)
{
    return key.SignCompact(hash, vchSigRet);
}

bool CSigner::VerifyHash(const uint256& hash, const CPubKey& pubkey, const std::vector<unsigned char>& vchSig, std::string& strErrorRet)
{
    return VerifyHash(hash, pubkey.GetID(), vchSig, strErrorRet);
}

bool CSigner::VerifyHash(const uint256& hash, const CKeyID& keyID, const std::vector<unsigned char>& vchSig, std::string& strErrorRet)
{
    CPubKey pubkeyFromSig;
    if(!pubkeyFromSig.RecoverCompact(hash, vchSig)) {
        strErrorRet = "Error recovering public key";
        return false;
    }
    if(pubkeyFromSig.GetID() != keyID) {
        strErrorRet = "Keys don't match";
        return false;
    }
    return true;
}

bool CSignature::Create(const CKey &key, const uint256 &hash)
{
    signature.clear();
    std::string status;
    
    if(!CSigner::SignHash(hash, key, signature)) {
        return error("%s : SignHash() failed", __func__);
    }
    if (!CSigner::VerifyHash(hash, key.GetPubKey(), signature, status)) {
        return error("%s : VerifyHash() failed, error: %s", __func__, status);
    }
    return true;
}

bool CSignature::Verify(const CPubKey& pubKey, const uint256 &hash) const
{
    std::string status;
    
    if(!CSigner::VerifyHash(hash, pubKey, signature, status)){
        return error("%s : VerifyHash failed: %s", __func__, status);
    }
    return true;
}

CSignature& CSignature::operator= (const CSignature& other)
{
    signature = other.signature;
    return *this;
};

bool CSignature::operator== (const CSignature& other) const
{
    return (signature == other.signature);
}
        
CValidatorRegister::CValidatorRegister(const CTxIn &vinIn, const CPubKey &pubKeyIn):
        vin(vinIn), pubKey(pubKeyIn), nTime(GetAdjustedTime()){}
        
uint256 CValidatorRegister::GetSignatureHash() const
{
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin;
    ss << pubKey;
    ss << nTime;
    return ss.GetHash();
};

bool CValidatorRegister::Sign(const CKey& key)
{
    return signature.Create(key, GetSignatureHash());
}

bool CValidatorRegister::Verify(const CPubKey& pubKey) const
{
    if (IsSigned()){
        return signature.Verify(pubKey, GetSignatureHash());
    } else {
        return false;
    }
}

bool CValidatorRegister::IsSigned() const
{
    return !signature.GetSignature().empty();
}

CValidatorRegister& CValidatorRegister::operator= (const CValidatorRegister &other)
{
    vin       = other.vin;
    pubKey    = other.pubKey;
    nTime     = other.nTime;
    signature = other.signature;
    return *this;
}

bool CValidatorRegister::operator== (const CValidatorRegister& other) const
{
    return (vin       == other.vin &&
            pubKey    == other.pubKey &&
            nTime     == other.nTime &&
            signature == other.signature);
}

CValidatorVote::CValidatorVote(const CTxIn &vinIn, const CPubKey &pubKeyIn, const std::vector<MNVote> &votesIn):
        vin(vinIn), pubKey(pubKeyIn), nTime (GetAdjustedTime()), votes(votesIn){}
        
uint256 CValidatorVote::GetSignatureHash() const
{
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << vin;
    ss << pubKey;
    ss << nTime;
    ss << votes;
    return ss.GetHash();
};

bool CValidatorVote::Sign(const CKey& key)
{
    return signature.Create(key, GetSignatureHash());
}

bool CValidatorVote::Verify(const CPubKey& pubKey) const
{
    if (IsSigned()){
        return signature.Verify(pubKey, GetSignatureHash());
    } else {
        return false;
    }
}

bool CValidatorVote::IsSigned() const
{
    return !signature.GetSignature().empty();
}

CValidatorVote& CValidatorVote::operator= (const CValidatorVote &other)
{
    vin = other.vin;
    pubKey = other.pubKey;
    nTime = other.nTime;
    votes = other.votes;
    signature = other.signature;
    return *this;
}

bool CValidatorVote::operator== (const CValidatorVote& other) const
{
    return (vin       == other.vin &&
            pubKey    == other.pubKey &&
            nTime     == other.nTime &&
            votes     == other.votes &&
            signature == other.signature);
}