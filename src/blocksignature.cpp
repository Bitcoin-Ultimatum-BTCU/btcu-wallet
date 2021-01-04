// Copyright (c) 2017-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blocksignature.h"
#include "main.h"
#include "zbtcuchain.h"
#include "validators_voting.h"

bool SignBlockWithKey(CBlock& block, const CKey& key)
{
    if (!key.Sign(block.GetHash(), block.vchBlockSig))
        return error("%s: failed to sign block hash with key", __func__);

    return true;
}

bool GetKeyIDFromUTXO(const CTxOut& txout, CKeyID& keyID)
{
    std::vector<valtype> vSolutions;
    txnouttype whichType;
    if (!Solver(txout.scriptPubKey, whichType, vSolutions))
        return false;
    if (whichType == TX_PUBKEY) {
        keyID = CPubKey(vSolutions[0]).GetID();
    } else if (whichType == TX_PUBKEYHASH || whichType == TX_COLDSTAKE) {
        keyID = CKeyID(uint160(vSolutions[0]));
    } else {
        return false;
    }

    return true;
}

boost::optional<std::pair<CTxIn, CKey>> GetVinKey(const std::string &strAlias);
boost::optional<std::pair<CTxIn, CKey>> GetGenesisVinKey();
boost::optional<std::pair<CTxIn, CKey>> GetRegisteredValidatorVinKey();

bool ValidatorSignBlock(CBlock& block)
{
    CTxIn vin;
    CKey key;

    // First check if current node has a genesis validator's key
    auto genesisVinKeyOpt = GetGenesisVinKey();
    if(genesisVinKeyOpt.is_initialized())
    {
        vin = genesisVinKeyOpt.value().first;
        key = genesisVinKeyOpt.value().second;
        LogPrintf(("ValidatorSignBlock() : signing " + block.GetHash().ToString() + " as a Genesis Validator\n").c_str());
    }
    else
    {
        auto vinKeyOpt = GetRegisteredValidatorVinKey();
        if(vinKeyOpt.is_initialized() &&                                // check if current node is registered as a masternode
           GetValidatorInfo(vinKeyOpt.value().first).is_initialized())  // check if current masternode is among validators for a current epoch
        {
            vin = vinKeyOpt.value().first;
            key = vinKeyOpt.value().second;
            LogPrintf(("ValidatorSignBlock() : signing " + block.GetHash().ToString() + " as a Regular Validator\n").c_str());
        }
    }
    
    auto status = false;
    
    if(key.IsValid())
    {
        block.validatorVin = vin;
        status = key.Sign(block.GetHashForValidator(), block.vchValidatorSig);
    }
    
    return status;
}

bool CheckValidatorSignature(const CBlock& block)
{
    CPubKey pubKey;
    
    // First check if block is signed with genesis validator's key
    auto genesisPubKeyOpt = GetGenesisValidatorPubKey(block.validatorVin);
    if(genesisPubKeyOpt.is_initialized())
    {
        pubKey = genesisPubKeyOpt.value();
        LogPrintf(("CheckValidatorSignature() : checking " + block.GetHash().ToString() + " as a Genesis Validator\n").c_str());
    }
    else
    {
        auto pubKeyOpt = GetValidatorPubKey(block.validatorVin);
        if(pubKeyOpt.is_initialized())
        {
            pubKey = pubKeyOpt.value();
            LogPrintf(("CheckValidatorSignature() : checking " + block.GetHash().ToString() + " as a Regular Validator\n").c_str());
        }
    }
    
    auto status = false;
    
    if(pubKey.IsValid())
    {
        status = pubKey.Verify(block.GetHashForValidator(), block.vchValidatorSig);
        LogPrintf(("CheckValidatorSignature() : signature of " + block.GetHash().ToString() + (status ? ": Valid\n" : ": Invalid\n")).c_str());
    }
    
    return status;
}

bool SignBlock(CBlock& block, const CKeyStore& keystore)
{
    CKeyID keyID;
    if (block.IsProofOfWork()) {
        bool fFoundID = false;
        for (const CTxOut& txout :block.vtx[0].vout) {
            if (!GetKeyIDFromUTXO(txout, keyID))
                continue;
            fFoundID = true;
            break;
        }
        if (!fFoundID)
            return error("%s: failed to find key for PoW", __func__);
    } else {
        if (!GetKeyIDFromUTXO(block.vtx[1].vout[1], keyID))
            return error("%s: failed to find key for PoS", __func__);
    }

    CKey key;
    if (!keystore.GetKey(keyID, key))
        return error("%s: failed to get key from keystore", __func__);

    return SignBlockWithKey(block, key);
}

bool CheckBlockSignature(const CBlock& block)
{
    if (block.IsProofOfWork())
        return block.vchBlockSig.empty();

    if (block.vchBlockSig.empty())
        return error("%s: vchBlockSig is empty!", __func__);

    /** Each block is signed by the private key of the input that is staked. This can be either zBTCU or normal UTXO
     *  zBTCU: Each zBTCU has a keypair associated with it. The serial number is a hash of the public key.
     *  UTXO: The public key that signs must match the public key associated with the first utxo of the coinstake tx.
     */
    CPubKey pubkey;
    bool fzBTCUStake = block.vtx[1].vin[0].IsZerocoinSpend();
    if (fzBTCUStake) {
        libzerocoin::CoinSpend spend = TxInToZerocoinSpend(block.vtx[1].vin[0]);
        pubkey = spend.getPubKey();
    } else {
        txnouttype whichType;
        std::vector<valtype> vSolutions;
        const CTxOut& txout = block.vtx[1].vout[1];
        if (!Solver(txout.scriptPubKey, whichType, vSolutions))
            return false;
        if (whichType == TX_PUBKEY || whichType == TX_PUBKEYHASH) {
            valtype& vchPubKey = vSolutions[0];
            pubkey = CPubKey(vchPubKey);
        } else if (whichType == TX_COLDSTAKE) {
            // pick the public key from the P2CS input
            const CTxIn& txin = block.vtx[1].vin[0];
            int start = 1 + (int) *txin.scriptSig.begin(); // skip sig
            start += 1 + (int) *(txin.scriptSig.begin()+start); // skip flag
            pubkey = CPubKey(txin.scriptSig.begin()+start+1, txin.scriptSig.end());
        }
    }

    if (!pubkey.IsValid())
        return error("%s: invalid pubkey %s", __func__, HexStr(pubkey));

    return pubkey.Verify(block.GetHash(), block.vchBlockSig);
}
