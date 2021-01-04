// Copyright (c) 2017-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "stakeinput.h"

#include "chain.h"
#include "main.h"
#include "txdb.h"
#include "zbtcu/deterministicmint.h"
#include "wallet/wallet.h"


bool CBTCUStake::SetInput(CTransaction txPrev, unsigned int n)
{
    this->txFrom = txPrev;
    this->nPosition = n;
    return true;
}

bool CBTCUStake::GetTxFrom(CTransaction& tx) const
{
    tx = txFrom;
    return true;
}

bool CBTCUStake::CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut)
{
    txIn = CTxIn(txFrom.GetHash(), nPosition);
    return true;
}

CAmount CBTCUStake::GetValue() const
{
    return txFrom.vout[nPosition].nValue;
}

bool CBTCUStake::CreateTxOuts(CWallet* pwallet, std::vector<CTxOut>& vout, CAmount nTotal) {
    std::vector<valtype> vSolutions;
    txnouttype whichType;
    CScript scriptPubKeyKernel = txFrom.vout[nPosition].scriptPubKey;
    if (!Solver(scriptPubKeyKernel, whichType, vSolutions))
        return error("%s: failed to parse kernel", __func__);

    if (whichType != TX_PUBKEY && whichType != TX_PUBKEYHASH && whichType != TX_COLDSTAKE)
        return error("%s: type=%d (%s) not supported for scriptPubKeyKernel", __func__, whichType,
                     GetTxnOutputType(whichType));

    CScript scriptPubKey;
    CKey key;
    if (whichType == TX_PUBKEYHASH) {
        // if P2PKH check that we have the input private key
        if (!pwallet->GetKey(CKeyID(uint160(vSolutions[0])), key))
            return error("%s: Unable to get staking private key", __func__);

        // convert to P2PK inputs
        scriptPubKey << key.GetPubKey() << OP_CHECKSIG;
    } else {
        // if P2CS, check that we have the coldstaking private key
        if ( whichType == TX_COLDSTAKE && !pwallet->GetKey(CKeyID(uint160(vSolutions[0])), key) )
            return error("%s: Unable to get cold staking private key", __func__);

        // keep the same script
        scriptPubKey = scriptPubKeyKernel;
    }

    vout.emplace_back(CTxOut(0, scriptPubKey));

    // Calculate if we need to split the output
    int nSplit = nTotal / (static_cast<CAmount>(pwallet->nStakeSplitThreshold * COIN));
    if (nSplit > 1) {
        // if nTotal is twice or more of the threshold; create more outputs
        int txSizeMax = MAX_STANDARD_TX_SIZE >> 11; // limit splits to <10% of the max TX size (/2048)
        if (nSplit > txSizeMax)
            nSplit = txSizeMax;
        for (int i = nSplit; i > 1; i--) {
            LogPrintf("%s: StakeSplit: nTotal = %d; adding output %d of %d\n", __func__, nTotal, (nSplit-i)+2, nSplit);
            vout.emplace_back(CTxOut(0, scriptPubKey));
        }
    }

    return true;
}

CDataStream CBTCUStake::GetUniqueness() const
{
    //The unique identifier for a BTCU stake is the outpoint
    CDataStream ss(SER_NETWORK, 0);
    ss << nPosition << txFrom.GetHash();
    return ss;
}

//The block that the UTXO was added to the chain
CBlockIndex* CBTCUStake::GetIndexFrom()
{
    if (pindexFrom)
        return pindexFrom;
    uint256 hashBlock = 0;
    CTransaction tx;
    if (GetTransaction(txFrom.GetHash(), tx, hashBlock, true)) {
        // If the index is in the chain, then set it as the "index from"
        if (mapBlockIndex.count(hashBlock)) {
            CBlockIndex* pindex = mapBlockIndex.at(hashBlock);
            if (chainActive.Contains(pindex))
                pindexFrom = pindex;
        }
    } else {
        LogPrintf("%s : failed to find tx %s\n", __func__, txFrom.GetHash().GetHex());
    }

    return pindexFrom;
}

//////////
////////// Genesis stake data ////////////////
/////////

bool CGenesisStake::SetInput(CTransaction txPrev, unsigned int n)
{
    this->txFrom = txPrev;
    this->nPosition = n;
    return true;
}

CBlockIndex* CGenesisStake::GetIndexFrom() {
    if (pindexFrom)
        return pindexFrom;

    uint256 hashBlock = Params().HashGenesisBlock();
    // If the index is in the chain, then set it as the "index from"
    if (mapBlockIndex.count(hashBlock)) {
        CBlockIndex* pindex = mapBlockIndex.at(hashBlock);
        if (chainActive.Contains(pindex))
            pindexFrom = pindex;
    }

    return pindexFrom;
}

CDataStream CGenesisStake::GetUniqueness() const {
    //The unique identifier for a BTCU stake is the outpoint
    CDataStream ss(SER_NETWORK, 0);
    ss << nPosition << txFrom.GetHash();
    return ss;
}

bool CGenesisStake::GetTxFrom(CTransaction& tx) const {
    throw std::runtime_error("Wrong usage of CGenesisStake::GetTxFrom()");
}

CAmount CGenesisStake::GetValue() const {
    throw std::runtime_error("Wrong usage of CGenesisStake::GetValue()");
}

bool CGenesisStake::CreateTxIn(CWallet*, CTxIn&, uint256) {
    throw std::runtime_error("Wrong usage of CGenesisStake::CreateTxIn()");
}

bool CGenesisStake::CreateTxOuts(CWallet*, std::vector<CTxOut>&, CAmount) {
    throw std::runtime_error("Wrong usage of CGenesisStake::CreateTxOuts()");
}