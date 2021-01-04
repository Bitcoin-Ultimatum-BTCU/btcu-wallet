// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef OBFUSCATION_RELAY_H
#define OBFUSCATION_RELAY_H

#include "activemasternode.h"
#include "main.h"
#include "masternodeman.h"


class CObfuScationRelay : public CSignedMessage
{
private:
    std::vector<unsigned char> vchSig2;

public:
    CTxIn vinMasternode;
    int nBlockHeight;
    int nRelayType;
    CTxIn in;
    CTxOut out;

    CObfuScationRelay();
    CObfuScationRelay(CTxIn& vinMasternodeIn, std::vector<unsigned char>& vchSigIn, int nBlockHeightIn, int nRelayTypeIn, CTxIn& in2, CTxOut& out2);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(vinMasternode);
        READWRITE(vchSig);
        READWRITE(vchSig2);
        READWRITE(nBlockHeight);
        READWRITE(nRelayType);
        READWRITE(in);
        READWRITE(out);
        try
        {
            READWRITE(nMessVersion);
        } catch (...) {
            nMessVersion = MessageVersion::MESS_VER_STRMESS;
        }
    }

    std::string ToString();

    // override CSignedMessage functions
    uint256 GetSignatureHash() const override;
    std::string GetStrMessage() const override;
    const CTxIn GetVin() const override { return vinMasternode; };
    bool Sign(std::string strSharedKey);   // use vchSig2

    void Relay();
    void RelayThroughNode(int nRank);
};


#endif
