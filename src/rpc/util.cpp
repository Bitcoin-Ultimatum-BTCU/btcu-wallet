// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <outputtype.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <script/signingprovider.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <tuple>

InitInterfaces* g_rpc_interfaces = nullptr;

CAmount AmountFromValue(const UniValue& value)
{
    if (!value.isNum() && !value.isStr())
        throw JSONRPCError(RPC_TYPE_ERROR, "Amount is not a number or string");
    CAmount amount;
    if (!ParseFixedPoint(value.getValStr(), 8, &amount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    if (!MoneyRange(amount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Amount out of range");
    return amount;
}

uint256 ParseHashV(const UniValue& v, std::string strName)
{
    std::string strHex(v.get_str());
    if (64 != strHex.length())
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be of length %d (not %d, for '%s')", strName, 64, strHex.length(), strHex));
    if (!IsHex(strHex)) // Note: IsHex("") is false
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    return uint256S(strHex);
}
uint256 ParseHashO(const UniValue& o, std::string strKey)
{
    return ParseHashV(find_value(o, strKey), strKey);
}
std::vector<unsigned char> ParseHexV(const UniValue& v, std::string strName)
{
    std::string strHex;
    if (v.isStr())
        strHex = v.get_str();
    if (!IsHex(strHex))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    return ParseHex(strHex);
}
std::vector<unsigned char> ParseHexO(const UniValue& o, std::string strKey)
{
    return ParseHexV(find_value(o, strKey), strKey);
}

// Converts a hex string to a public key if possible
CPubKey HexToPubKey(const std::string& hex_in)
{
    if (!IsHex(hex_in)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid public key: " + hex_in);
    }
    CPubKey vchPubKey(ParseHex(hex_in));
    if (!vchPubKey.IsFullyValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid public key: " + hex_in);
    }
    return vchPubKey;
}

// Retrieves a public key for an address from the given FillableSigningProvider
CPubKey AddrToPubKey(FillableSigningProvider* const keystore, const std::string& addr_in)
{
    CTxDestination dest = DecodeDestination(addr_in);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address: " + addr_in);
    }
    CKeyID key = GetKeyForDestination(*keystore, dest);
    if (key.IsNull()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("%s does not refer to a key", addr_in));
    }
    CPubKey vchPubKey;
    if (!keystore->GetPubKey(key, vchPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("no full public key for address %s", addr_in));
    }
    if (!vchPubKey.IsFullyValid()) {
       throw JSONRPCError(RPC_INTERNAL_ERROR, "Wallet contains an invalid public key");
    }
    return vchPubKey;
}

class DescribeAddressVisitor : public boost::static_visitor<UniValue>
{
public:
    explicit DescribeAddressVisitor() {}

    UniValue operator()(const CNoDestination& dest) const
    {
        return UniValue(UniValue::VOBJ);
    }

    UniValue operator()(const CKeyID& keyID) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", false);
        obj.pushKV("iswitness", false);
        return obj;
    }

    UniValue operator()(const CScriptID& scriptID) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", true);
        obj.pushKV("iswitness", false);
        return obj;
    }

    UniValue operator()(const WitnessV0KeyHash& id) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", false);
        obj.pushKV("iswitness", true);
        obj.pushKV("witness_version", 0);
        obj.pushKV("witness_program", HexStr(id.begin(), id.end()));
        return obj;
    }

    UniValue operator()(const WitnessV0ScriptHash& id) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("isscript", true);
        obj.pushKV("iswitness", true);
        obj.pushKV("witness_version", 0);
        obj.pushKV("witness_program", HexStr(id.begin(), id.end()));
        return obj;
    }

    UniValue operator()(const WitnessUnknown& id) const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("iswitness", true);
        obj.pushKV("witness_version", (int)id.version);
        obj.pushKV("witness_program", HexStr(id.program, id.program + id.length));
        return obj;
    }
};

UniValue DescribeAddress(const CTxDestination& dest)
{
    return boost::apply_visitor(DescribeAddressVisitor(), dest);
}

unsigned int ParseConfirmTarget(const UniValue& value, unsigned int max_target)
{
    int target = value.get_int();
    if (target < 1 || (unsigned int)target > max_target) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid conf_target, must be between %u - %u", 1, max_target));
    }
    return (unsigned int)target;
}

static std::pair<int64_t, int64_t> ParseRange(const UniValue& value)
{
    if (value.isNum()) {
        return {0, value.get_int64()};
    }
    if (value.isArray() && value.size() == 2 && value[0].isNum() && value[1].isNum()) {
        int64_t low = value[0].get_int64();
        int64_t high = value[1].get_int64();
        if (low > high) throw JSONRPCError(RPC_INVALID_PARAMETER, "Range specified as [begin,end] must not have begin after end");
        return {low, high};
    }
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Range must be specified as end or as [begin,end]");
}

std::pair<int64_t, int64_t> ParseDescriptorRange(const UniValue& value)
{
    int64_t low, high;
    std::tie(low, high) = ParseRange(value);
    if (low < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Range should be greater or equal than 0");
    }
    if ((high >> 31) != 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "End of range is too high");
    }
    if (high >= low + 1000000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Range is too large");
    }
    return {low, high};
}
