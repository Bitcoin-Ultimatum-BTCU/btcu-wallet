// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2015-2019 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCU_MASTERNODE_VALIDATORS_H
#define BTCU_MASTERNODE_VALIDATORS_H

#include "key.h"
#include "primitives/tx_in_out.h"

/** Helper class for signing hashes and checking their signatures
 */
class CSigner
{
public:
    /// Sign the hash, returns true if successful
    static bool SignHash(const uint256& hash, const CKey& key, std::vector<unsigned char>& vchSigRet);
    /// Verify the hash signature, returns true if successful
    static bool VerifyHash(const uint256& hash, const CPubKey& pubkey, const std::vector<unsigned char>& vchSig, std::string& strErrorRet);
    /// Verify the hash signature, returns true if successful
    static bool VerifyHash(const uint256& hash, const CKeyID& keyID, const std::vector<unsigned char>& vchSig, std::string& strErrorRet);
};

/** Class for creating and storing signature for specified hash
 */
class CSignature
{
private:
    std::vector<unsigned char> signature;

public:
    CSignature() : signature(){}
    CSignature(const CSignature& other){ signature = other.signature; }

    bool Create(const CKey& key, const uint256 &hash);
    bool Verify(const CPubKey& pubKey, const uint256 &hash) const;
    
    CSignature& operator= (const CSignature& other);
    bool operator== (const CSignature& other) const;
    const std::vector<unsigned char>& GetSignature() const { return signature; }
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(signature);
    }
};

class CValidatorRegister
{
public:
    CTxIn vin;
    CPubKey pubKey;
    int64_t nTime;
    CSignature signature;
    
    CValidatorRegister(): vin(), pubKey(), nTime(0), signature(){}
    explicit CValidatorRegister(const CTxIn &vin, const CPubKey &pubKey);
 
    uint256 GetSignatureHash() const;
    
    bool Sign(const CKey& key);
    bool Verify(const CPubKey& pubKey) const;
    bool IsSigned() const;
    
    CValidatorRegister& operator= (const CValidatorRegister& other);
    bool operator== (const CValidatorRegister& other) const;
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vin);
        READWRITE(pubKey);
        READWRITE(nTime);
        READWRITE(signature);
    }
};

#define MAX_VALIDATORS_DEFAULT 10

#define VoteAbstain 0
#define VoteYes 1
#define VoteNo -1

// Vote for a masternode
class MNVote
{
public:
    CTxIn vin;      // MN-candidate for whom the vote is casted
    int vote;       // vote for MN-candidate
    
    MNVote(): vin(), vote(VoteAbstain){};
    MNVote(CTxIn &vinIn, int voteIn): vin(vinIn), vote(voteIn){}
    
    bool operator== (const MNVote& other) const { return ((vin == other.vin) && (vote == other.vote));}
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vin);
        READWRITE(vote);
    }
};

class CValidatorVote
{
public:
    CTxIn vin;
    CPubKey pubKey;
    int64_t nTime;
    std::vector<MNVote> votes;
    CSignature signature;
    
    CValidatorVote(): vin(), pubKey(), nTime(0), votes(), signature(){}
    explicit CValidatorVote(const CTxIn &vin, const CPubKey &pubKey, const std::vector<MNVote> &votes);
    
    uint256 GetSignatureHash() const;
    
    bool Sign(const CKey& key);
    bool Verify(const CPubKey& pubKey) const;
    bool IsSigned() const;
    
    CValidatorVote& operator= (const CValidatorVote& other);
    bool operator== (const CValidatorVote& other) const ;
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vin);
        READWRITE(pubKey);
        READWRITE(nTime);
        READWRITE(votes);
        READWRITE(signature);
    }
};

class CValidatorInfo
{
public:
    CTxIn vin;
    CPubKey pubKey;
    
    CValidatorInfo(): vin(), pubKey(){}
    CValidatorInfo(const CTxIn &vinIn, const CPubKey &pubKeyIn) : vin(vinIn), pubKey(pubKeyIn) {}
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vin);
        READWRITE(pubKey);
    }
};

#endif //BTCU_MASTERNODE_VALIDATORS_H
