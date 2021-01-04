// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2016-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet_ismine.h"

#include "key.h"
#include "keystore.h"
#include "script/script.h"
#include "script/standard.h"
#include "util.h"

#include "utilstrencodings.h"


/**
 * This is an enum that tracks the execution context of a script, similar to
 * SigVersion in script/interpreter. It is separate however because we want to
 * distinguish between top-level scriptPubKey execution and P2SH redeemScript
 * execution (a distinction that has no impact on consensus rules).
 */
enum class IsMineSigVersion
{
    TOP = 0,        //!< scriptPubKey execution
    P2SH = 1,       //!< P2SH redeemScript
    WITNESS_V0 = 2, //!< P2WSH witness script execution
};

bool PermitsUncompressed(IsMineSigVersion sigversion)
{
    return sigversion == IsMineSigVersion::TOP || sigversion == IsMineSigVersion::P2SH;
}

typedef std::vector<unsigned char> valtype;

unsigned int HaveKeys(const std::vector<valtype>& pubkeys, const CKeyStore& keystore)
{
    unsigned int nResult = 0;
    for (const valtype& pubkey : pubkeys) {
        CKeyID keyID = CPubKey(pubkey).GetID();
        if(keystore.HaveKey(keyID))
            ++nResult;
    }
    return nResult;
}

isminetype IsMine(const CKeyStore& keystore, const CScript& scriptPubKey, IsMineSigVersion sigversion)
{
    if(keystore.HaveWatchOnly(scriptPubKey))
        return ISMINE_WATCH_ONLY;
    if(keystore.HaveMultiSig(scriptPubKey))
        return ISMINE_MULTISIG;

    std::vector<valtype> vSolutions;
    txnouttype whichType;
    if(!Solver(scriptPubKey, whichType, vSolutions)) {
        if(keystore.HaveWatchOnly(scriptPubKey))
            return ISMINE_WATCH_ONLY;
        if(keystore.HaveMultiSig(scriptPubKey))
            return ISMINE_MULTISIG;

        return ISMINE_NO;
    }

    CKeyID keyID;
    switch (whichType) {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
    case TX_WITNESS_UNKNOWN:
        break;
    case TX_ZEROCOINMINT:
    case TX_PUBKEY: {
        keyID = CPubKey(vSolutions[0]).GetID();
        if (PermitsUncompressed(sigversion) && vSolutions[0].size() == 33 && keystore.HaveKey(keyID)) {
            return ISMINE_SPENDABLE;
        }
        break;
    }
    case TX_WITNESS_V0_KEYHASH: {
        if (sigversion == IsMineSigVersion::WITNESS_V0) {
            // P2WPKH inside P2WSH is invalid.
            break;
        }
        if (sigversion == IsMineSigVersion::TOP &&
            !keystore.HaveCScript(CScriptID(CScript() << OP_0 << vSolutions[0]))) {
            // We do not support bare witness outputs unless the P2SH version of it would be
            // acceptable as well. This protects against matching before segwit activates.
            // This also applies to the P2WSH case.
            break;
        }
        return IsMine(keystore, GetScriptForDestination(CKeyID(uint160(vSolutions[0]))), IsMineSigVersion::WITNESS_V0);
    }
    case TX_PUBKEYHASH: {
        keyID = CKeyID(uint160(vSolutions[0]));
        if (!PermitsUncompressed(sigversion)) {
            CPubKey pubkey;
            if (keystore.GetPubKey(keyID, pubkey) && !pubkey.IsCompressed()) {
                break;
            }
        }
        if (keystore.HaveKey(keyID)) {
            return ISMINE_SPENDABLE;
        }
        break;
    }
    case TX_SCRIPTHASH: {
        if (sigversion != IsMineSigVersion::TOP) {
            // P2SH inside P2WSH or P2SH is invalid.
            break;
        }
        CScriptID scriptID = CScriptID(uint160(vSolutions[0]));
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            return IsMine(keystore, subscript, IsMineSigVersion::P2SH);
        }
        break;
    }
    case TX_WITNESS_V0_SCRIPTHASH: {
        if (sigversion == IsMineSigVersion::WITNESS_V0) {
            // P2WSH inside P2WSH is invalid.
            break;
        }
        if (sigversion == IsMineSigVersion::TOP && !keystore.HaveCScript(CScriptID(CScript() << OP_0 << vSolutions[0]))) {
            break;
        }
        uint160 hash;
        CRIPEMD160().Write(&vSolutions[0][0], vSolutions[0].size()).Finalize(hash.begin());
        CScriptID scriptID = CScriptID(hash);
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            return IsMine(keystore, subscript, IsMineSigVersion::WITNESS_V0);
        }
        break;
    }
    case TX_COLDSTAKE: {
        if (sigversion != IsMineSigVersion::TOP) {
            break;
        }

        CKeyID stakeKeyID = CKeyID(uint160(vSolutions[0]));
        bool stakeKeyIsMine = keystore.HaveKey(stakeKeyID);
        CKeyID ownerKeyID = CKeyID(uint160(vSolutions[1]));
        bool spendKeyIsMine = keystore.HaveKey(ownerKeyID);

        if (spendKeyIsMine && stakeKeyIsMine)
            return ISMINE_SPENDABLE_STAKEABLE;
        else if (stakeKeyIsMine)
            return ISMINE_COLD;
        else if (spendKeyIsMine)
            return ISMINE_SPENDABLE_DELEGATED;
        break;
    }
    case TX_LEASE: {
        if (sigversion != IsMineSigVersion::TOP) {
            break;
        }

        CKeyID leaserKeyID = CKeyID(uint160(vSolutions[0]));
        bool leaserKeyIsMine = keystore.HaveKey(leaserKeyID);
        CKeyID ownerKeyID = CKeyID(uint160(vSolutions[1]));
        bool spendKeyIsMine = keystore.HaveKey(ownerKeyID);

        if (spendKeyIsMine && leaserKeyIsMine)
            return ISMINE_SPENDABLE_LEASING;
        else if (leaserKeyIsMine)
            return ISMINE_LEASING;
        else if (spendKeyIsMine)
            return ISMINE_LEASED;
        break;
    }
    case TX_LEASINGREWARD: {
        if (sigversion != IsMineSigVersion::TOP) {
            break;
        }

        // 0. TRXHASH
        // 1. N
        CKeyID keyID = CKeyID(uint160(vSolutions[2]));
        if(keystore.HaveKey(keyID))
            return ISMINE_SPENDABLE;
        break;
    }
    case TX_MULTISIG: {
        // Only consider transactions "mine" if we own ALL the
        // keys involved. multi-signature transactions that are
        // partially owned (somebody else has a key that can spend
        // them) enable spend-out-from-under-you attacks, especially
        // in shared-wallet situations.
        std::vector<valtype> keys(vSolutions.begin() + 1, vSolutions.begin() + vSolutions.size() - 1);
        if(HaveKeys(keys, keystore) == keys.size())
            return ISMINE_SPENDABLE;
        break;
    }
    }

    if(keystore.HaveWatchOnly(scriptPubKey))
        return ISMINE_WATCH_ONLY;
    if(keystore.HaveMultiSig(scriptPubKey))
        return ISMINE_MULTISIG;

    return ISMINE_NO;
}

isminetype IsMine(const CKeyStore& keystore, const CScript& scriptPubKey)
{
    return IsMine(keystore, scriptPubKey, IsMineSigVersion::TOP);
}

isminetype IsMine(const CKeyStore& keystore, const CTxDestination& dest)
{
    CScript script = GetScriptForDestination(dest);
    return IsMine(keystore, script, IsMineSigVersion::TOP);
}