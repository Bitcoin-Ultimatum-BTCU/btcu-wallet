// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/keyorigin.h>
#include <script/signingprovider.h>
#include <script/standard.h>

#include <util/system.h>

//const SigningProvider& DUMMY_SIGNING_PROVIDER = SigningProvider();
//
//template<typename M, typename K, typename V>
//bool LookupHelper(const M& map, const K& key, V& value)
//{
//    auto it = map.find(key);
//    if (it != map.end()) {
//        value = it->second;
//        return true;
//    }
//    return false;
//}
//
//bool HidingSigningProvider::GetCScript(const CScriptID& scriptid, CScript& script) const
//{
//    return m_provider->GetCScript(scriptid, script);
//}
//
//bool HidingSigningProvider::GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const
//{
//    return m_provider->GetPubKey(keyid, pubkey);
//}
//
//bool HidingSigningProvider::GetKey(const CKeyID& keyid, CKey& key) const
//{
//    if (m_hide_secret) return false;
//    return m_provider->GetKey(keyid, key);
//}
//
//bool HidingSigningProvider::GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const
//{
//    if (m_hide_origin) return false;
//    return m_provider->GetKeyOrigin(keyid, info);
//}
//
//bool FlatSigningProvider::GetCScript(const CScriptID& scriptid, CScript& script) const { return LookupHelper(scripts, scriptid, script); }
//bool FlatSigningProvider::GetPubKey(const CKeyID& keyid, CPubKey& pubkey) const { return LookupHelper(pubkeys, keyid, pubkey); }
//bool FlatSigningProvider::GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const
//{
//    std::pair<CPubKey, KeyOriginInfo> out;
//    bool ret = LookupHelper(origins, keyid, out);
//    if (ret) info = std::move(out.second);
//    return ret;
//}
//bool FlatSigningProvider::GetKey(const CKeyID& keyid, CKey& key) const { return LookupHelper(keys, keyid, key); }
//
//FlatSigningProvider Merge(const FlatSigningProvider& a, const FlatSigningProvider& b)
//{
//    FlatSigningProvider ret;
//    ret.scripts = a.scripts;
//    ret.scripts.insert(b.scripts.begin(), b.scripts.end());
//    ret.pubkeys = a.pubkeys;
//    ret.pubkeys.insert(b.pubkeys.begin(), b.pubkeys.end());
//    ret.keys = a.keys;
//    ret.keys.insert(b.keys.begin(), b.keys.end());
//    ret.origins = a.origins;
//    ret.origins.insert(b.origins.begin(), b.origins.end());
//    return ret;
//}

CKeyID GetKeyForDestination(const CCryptoKeyStore& store, const CTxDestination& dest)
{
    // Only supports destinations which map to single public keys, i.e. P2PKH,
    // P2WPKH, and P2SH-P2WPKH.
    if (auto id = boost::get<CKeyID>(&dest)) {
        return CKeyID(*id);
    }
    if (auto witness_id = boost::get<WitnessV0KeyHash>(&dest)) {
        return CKeyID(*witness_id);
    }
    if (auto script_hash = boost::get<CScriptID>(&dest)) {
        CScript script;
        CScriptID script_id(*script_hash);
        CTxDestination inner_dest;
        if (store.GetCScript(script_id, script) && ExtractDestination(script, inner_dest)) {
            if (auto inner_witness_id = boost::get<WitnessV0KeyHash>(&inner_dest)) {
                return CKeyID(*inner_witness_id);
            }
        }
    }
    return CKeyID();
}
