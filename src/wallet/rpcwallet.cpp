// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2020 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "addressbook.h"
#include "amount.h"
#include "btcu_address.h"
#include "core_io.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "rpc/server.h"
#include "timedata.h"
#include "util.h"
#include "utilmoneystr.h"
#include "wallet.h"
#include "walletdb.h"
#include "zbtcuchain.h"
#include <string>
#include <stdint.h>

#include "libzerocoin/Coin.h"
#include "spork.h"
#include "zbtcu/deterministicmint.h"
#include <boost/assign/list_of.hpp>
#include <boost/thread/thread.hpp>

#include <univalue.h>
#include <iostream>

#include "validation.h"
#include "coincontrol.h"
#include "contract.h"

int64_t nWalletUnlockTime;
static CCriticalSection cs_nWalletUnlockTime;

std::string HelpRequiringPassphrase()
{
    return pwalletMain && pwalletMain->IsCrypted() ? "\nRequires wallet passphrase to be set with walletpassphrase call." : "";
}

void EnsureWalletIsUnlocked(bool fAllowAnonOnly)
{
    if (pwalletMain->IsLocked() || (!fAllowAnonOnly && pwalletMain->fWalletUnlockAnonymizeOnly))
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
}

void WalletTxToJSON(const CWalletTx& wtx, UniValue& entry)
{
    int confirms = wtx.GetDepthInMainChain(false);
    int confirmsTotal = GetIXConfirmations(wtx.GetHash()) + confirms;
    entry.push_back(Pair("confirmations", confirmsTotal));
    entry.push_back(Pair("bcconfirmations", confirms));
    if (wtx.IsCoinBase() || wtx.IsCoinStake())
        entry.push_back(Pair("generated", true));
    if (confirms > 0) {
        entry.push_back(Pair("blockhash", wtx.hashBlock.GetHex()));
        entry.push_back(Pair("blockindex", wtx.nIndex));
        entry.push_back(Pair("blocktime", mapBlockIndex[wtx.hashBlock]->GetBlockTime()));
    } else {
        entry.push_back(Pair("trusted", wtx.IsTrusted()));
    }
    uint256 hash = wtx.GetHash();
    entry.push_back(Pair("txid", hash.GetHex()));
    UniValue conflicts(UniValue::VARR);
    for (const uint256& conflict : wtx.GetConflicts())
        conflicts.push_back(conflict.GetHex());
    entry.push_back(Pair("walletconflicts", conflicts));
    entry.push_back(Pair("time", wtx.GetTxTime()));
    entry.push_back(Pair("timereceived", (int64_t)wtx.nTimeReceived));
    for (const PAIRTYPE(std::string, std::string) & item : wtx.mapValue)
        entry.push_back(Pair(item.first, item.second));
}

std::string AccountFromValue(const UniValue& value)
{
    std::string strAccount = value.get_str();
    if (strAccount == "*")
        throw JSONRPCError(RPC_WALLET_INVALID_ACCOUNT_NAME, "Invalid account name");
    return strAccount;
}

CBTCUAddress GetNewAddressFromAccount(const std::string purpose, const UniValue &params,
        const CChainParams::Base58Type addrType = CChainParams::PUBKEY_ADDRESS)
{
    LOCK2(cs_main, pwalletMain->cs_wallet);
    // Parse the account first so we don't generate a key if there's an error
    std::string strAccount;
    if (!params.isNull() && params.size() > 0)
        strAccount = AccountFromValue(params[0]);

    CBTCUAddress address;
    PairResult r = pwalletMain->getNewAddress(address, strAccount, purpose, addrType);
    if(!r.result)
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, *r.status);
    return address;
}

bool IsValidContractSenderAddressKey(const CTxDestination &dest)
{
    const CKeyID *keyID = boost::get<CKeyID>(&dest);
    return keyID != 0;
}
bool SetDefaultSignSenderAddress(CWallet* const pwallet, CTxDestination& destAdress)
{
    // Set default sender address if none provided
    // Select any valid unspent output that can be used for contract sender address
    std::vector<COutput> vecOutputs;

    assert(pwallet != NULL);
    pwallet->AvailableCoins(&vecOutputs, false, NULL, true);

    for (const COutput& out : vecOutputs) {
        const CScript& scriptPubKey = out.tx->vout[out.i].scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, destAdress)
                             && IsValidContractSenderAddressKey(destAdress);

        if (!fValidAddress)
            continue;
        break;
    }

    return !boost::get<CNoDestination>(&destAdress);
}
bool SetDefaultPayForContractAddress(CWallet* const pwallet, CCoinControl & coinControl)
{
    // Set default coin to pay for the contract
    // Select any valid unspent output that can be used to pay for the contract
    std::vector<COutput> vecOutputs;
    coinControl.fAllowOtherInputs=true;

    assert(pwallet != NULL);
    pwallet->AvailableCoins(&vecOutputs, false, NULL, true);

    for (const COutput& out : vecOutputs) {
        CTxDestination destAdress;
        const CScript& scriptPubKey = out.tx->vout[out.i].scriptPubKey;//->tx->vout[out.i].scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, destAdress)
                             && IsValidContractSenderAddressKey(destAdress);

        if (!fValidAddress)
            continue;

        coinControl.Select(COutPoint(out.tx->GetHash(),out.i));
        break;
    }

    return coinControl.HasSelected();
}
bool IsValidDestinationKey(const CTxDestination& dest) {
    return dest.which() != 0;
}

bool GetSenderDest(CWallet * const pwallet, const CTransaction& tx, CTxDestination& txSenderDest)
{
    // Initialize variables
    CScript senderPubKey;

    // Get sender destination
    if(tx.HasOpSender())
    {
        // Get destination from the outputs
        for(CTxOut out : tx.vout)
        {
            if(out.scriptPubKey.HasOpSender())
            {
                ExtractSenderData(out.scriptPubKey, &senderPubKey, 0);
                break;
            }
        }
    }
    else
    {
        // Get destination from the inputs
        senderPubKey = pwallet->mapWallet.at(tx.vin[0].prevout.hash).tx->vout[tx.vin[0].prevout.n].scriptPubKey;
    }

    // Extract destination from script
    return ExtractDestination(senderPubKey, txSenderDest);
}

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

UniValue createcontract(const UniValue& params, bool fHelp){

    //std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    //CWallet* const pwallet = wallet.get();


    auto locked_chain = nullptr;//pwalletMain->chain().lock();
    LOCK2(cs_main, pwalletMain->cs_wallet);
    //QtumDGP qtumDGP(globalState.get(), fGettingValuesDGP);
    uint64_t blockGasLimit = 40000000;//= qtumDGP.getBlockGasLimit(::ChainActive().Height());
    uint64_t minGasPrice = 40;//CAmount(qtumDGP.getMinGasPrice(::ChainActive().Height()));
    CAmount nGasPrice = 40;//(minGasPrice>DEFAULT_GAS_PRICE)?minGasPrice:DEFAULT_GAS_PRICE;

    if (fHelp || params.size() < 1)
        throw std::runtime_error("createcontract"
               "\nCreate a contract with bytcode." +
               HelpRequiringPassphrase() + "\n"

                       "bytecode"//, RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "contract bytcode."},
                       "gasLimit"//, RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasLimit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_CREATE)+", max: "+i64tostr(blockGasLimit)},
                       "gasPrice"//, RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasPrice QTUM price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                       "senderaddress"//, RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The qtum address that will be used to create the contract."},
                       "broadcast"//, RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Whether to broadcast the transaction or not."},
                       "changeToSender"//, RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Return the change to the sender."},

                       "[\n"
                       "  {\n"
                       "    \"txid\" : (string) The transaction id.\n"
                       "    \"sender\" : (string) CURRENCY_UNIT  address of the sender.\n"
                                                                      "    \"hash160\" : (string) ripemd-160 hash of the sender.\n"
                                                                      "    \"address\" : (string) expected contract address.\n"
                                                                      "  }\n"
                                                                      "]\n" +

                       HelpExampleCli("createcontract", "\"60606040525b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff02191690836c010000000000000000000000009081020402179055506103786001600050819055505b600c80605b6000396000f360606040526008565b600256\"")
                       + HelpExampleCli("createcontract", "\"60606040525b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff02191690836c010000000000000000000000009081020402179055506103786001600050819055505b600c80605b6000396000f360606040526008565b600256\" 6000000 +FormatMoney(minGasPrice)+\"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" true")
    );


    std::string bytecode=params[0].get_str();

    if(bytecode.size() % 2 != 0 || !CheckHex(bytecode))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid data (data not hex)");

    uint64_t nGasLimit=DEFAULT_GAS_LIMIT_OP_CREATE;
    if (params.size() > 1){
        nGasLimit = params[1].get_int();
        if (nGasLimit > blockGasLimit)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Maximum is: "+i64tostr(blockGasLimit)+")");
        if (nGasLimit < MINIMUM_GAS_LIMIT)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Minimum is: "+i64tostr(MINIMUM_GAS_LIMIT)+")");
        if (nGasLimit <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit");
    }

    if (params.size() > 2){
        nGasPrice = params[2].get_int();
        if (nGasPrice <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice");
        CAmount maxRpcGasPrice = 10000000;//gArgs.GetArg("-rpcmaxgasprice", MAX_RPC_GAS_PRICE);
        if (nGasPrice > (int64_t)maxRpcGasPrice)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice, Maximum allowed in RPC calls is: "+FormatMoney(maxRpcGasPrice)+" (use -rpcmaxgasprice to change it)");
        if (nGasPrice < (int64_t)minGasPrice)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice (Minimum is: "+FormatMoney(minGasPrice)+")");
    }

    bool fHasSender=false;
    CTxDestination senderAddress;
    if (params.size() > 3){
        senderAddress = CBTCUAddress(params[3].get_str()).Get();
        if (!IsValidDestinationKey(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Qtum address to send from");
        if (!IsValidContractSenderAddressKey(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract sender address. Only P2PK and P2PKH allowed");
        else
            fHasSender=true;
    }

    bool fBroadcast=true;
    if (params.size() > 4){
        fBroadcast=params[4].get_bool();
    }

    bool fChangeToSender=true;
    if (params.size() > 5){
        fChangeToSender=params[5].get_bool();
    }

    std::unique_ptr<CCoinControl> coinControl;
    CTxDestination signSenderAddress = CNoDestination();

    if(fHasSender){
        // Find a UTXO with sender address
        std::vector<COutput> vecOutputs;

        coinControl.reset(new CCoinControl());
        coinControl->fAllowOtherInputs=true;

        assert(pwalletMain != NULL);
        pwalletMain->AvailableCoins(&vecOutputs, false, NULL, true);

        for (const COutput& out : vecOutputs) {
            CTxDestination destAdress;
            const CScript scriptPubKey = out.tx->vout[out.i].scriptPubKey;
            bool fValidAddress = ExtractDestination(scriptPubKey, destAdress);

            if (!fValidAddress || senderAddress != destAdress)
                continue;

            coinControl->Select(COutPoint(out.tx->GetHash(),out.i));
            break;
        }

        if (coinControl->HasSelected()) {
            // Change to the sender
            if(fChangeToSender){
                coinControl->destChange=senderAddress;
            }
            signSenderAddress = senderAddress;
        } else
            coinControl.reset();

    } else {
        coinControl.reset();
        SetDefaultSignSenderAddress(pwalletMain, signSenderAddress);
    }

    EnsureWalletIsUnlocked();

    CAmount nGasFee=nGasPrice*nGasLimit;

    CAmount curBalance = pwalletMain->GetBalance();

    // Check amount
    if (nGasFee <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount");

    if (nGasFee > curBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    // Build OP_EXEC script
    CScript scriptPubKey = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(nGasLimit) << CScriptNum(nGasPrice) << ParseHex(bytecode) <<OP_CREATE;
    if(IsValidDestination(signSenderAddress))
    {
        CKeyID key_id = GetKeyForDestination(*pwalletMain, signSenderAddress);
        CKey key;
        if (!pwalletMain->GetKey(key_id, key)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available");
        }
        std::vector<unsigned char> scriptSig;
        scriptPubKey = (CScript() << CScriptNum(addresstype::PUBKEYHASH) << ToByteVector(key_id) << ToByteVector(scriptSig) << OP_SENDER) + scriptPubKey;
    }
    else
    {
            // OP_SENDER will always be used when QIP5Height is active
            throw JSONRPCError(RPC_TYPE_ERROR, "Sender address fail to set for OP_SENDER.");
    }

    // Create and send the transaction
    CAmount nFeeRequired = 0;
    std::string strError;
    CAmount nValue = 0;

    //CTransactionRef tx;
    // make our change address
    CReserveKey reservekey(pwalletMain);
    CWalletTx wtx;
    if (!pwalletMain->CreateTransaction(scriptPubKey, 0, wtx, reservekey, nFeeRequired, strError, coinControl.get(), ALL_COINS, true, nGasFee, true, true, true, signSenderAddress)) {
        if (nFeeRequired > pwalletMain->GetBalance())
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    CTxDestination txSenderDest;
    GetSenderDest(pwalletMain, wtx, txSenderDest);

    if (fHasSender && !(senderAddress == txSenderDest)){
        throw JSONRPCError(RPC_TYPE_ERROR, "Sender could not be set, transaction was not committed!");
    }

    UniValue result(UniValue::VOBJ);
    if(fBroadcast){
        CValidationState state;
        if (!pwalletMain->CommitTransaction(wtx, reservekey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of the wallet and coins were spent in the copy but not marked as spent here.");

        std::string txId=wtx.GetHash().GetHex();
        result.pushKV("txid", txId);

        CTxDestination txSenderAdress(txSenderDest);
        CKeyID keyid = GetKeyForDestination(*pwalletMain, txSenderAdress);

        result.pushKV("sender", CBTCUAddress(txSenderAdress).ToString());
        result.pushKV("hash160", HexStr(valtype(keyid.begin(),keyid.end())));

        std::vector<unsigned char> SHA256TxVout(32);
        std::vector<unsigned char> contractAddress(20);
        std::vector<unsigned char> txIdAndVout(wtx.GetHash().begin(), wtx.GetHash().end());
        uint32_t voutNumber=0;
        for (const CTxOut& txout : wtx.vout) {
            if(txout.scriptPubKey.HasOpCreate()){
                std::vector<unsigned char> voutNumberChrs;
                if (voutNumberChrs.size() < sizeof(voutNumber))voutNumberChrs.resize(sizeof(voutNumber));
                std::memcpy(voutNumberChrs.data(), &voutNumber, sizeof(voutNumber));
                txIdAndVout.insert(txIdAndVout.end(),voutNumberChrs.begin(),voutNumberChrs.end());
                break;
            }
            voutNumber++;
        }
        CSHA256().Write(txIdAndVout.data(), txIdAndVout.size()).Finalize(SHA256TxVout.data());
        CRIPEMD160().Write(SHA256TxVout.data(), SHA256TxVout.size()).Finalize(contractAddress.data());
        result.pushKV("address", HexStr(contractAddress));
    }else{
        std::string strHex = EncodeHexTx(wtx);
        result.pushKV("raw transaction", strHex);
    }
    return result;
}

UniValue sendtocontract(const UniValue& params, bool fHelp){

    LOCK2(cs_main, pwalletMain->cs_wallet);
    QtumDGP qtumDGP(globalState.get(), fGettingValuesDGP);
    uint64_t blockGasLimit = 40000000;//qtumDGP.getBlockGasLimit(::ChainActive().Height());
    uint64_t minGasPrice = 40;//CAmount(qtumDGP.getMinGasPrice(::ChainActive().Height()));
    CAmount nGasPrice = 40;// (minGasPrice>DEFAULT_GAS_PRICE)?minGasPrice:DEFAULT_GAS_PRICE;

    if (fHelp || params.size() < 2) {
        throw std::runtime_error(std::string("sendtocontract"
                                             "\nSend funds and data to a contract.") +

                                 std::string(
                                         "contractaddress" //RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The contract address that will receive the funds and data."},
                                         "datahex"//, RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "data to send."},
                                         "amount"//, RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "The amount in " + CURRENCY_UNIT + " to send. eg 0.1, default: 0"},
                                         "gasLimit"//, RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasLimit, default: "+i64tostr(DEFAULT_GAS_LIMIT_OP_SEND)+", max: "+i64tostr(blockGasLimit)},
                                         "gasPrice"//, RPCArg::Type::AMOUNT, RPCArg::Optional::OMITTED, "gasPrice Qtum price per gas unit, default: "+FormatMoney(nGasPrice)+", min:"+FormatMoney(minGasPrice)},
                                         "senderaddress"//, RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The qtum address that will be used as sender."},
                                         "broadcast"//, RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Whether to broadcast the transaction or not."},
                                         "changeToSender"//, RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Return the change to the sender."},
                                         //},
                                         //RPCResult{
                                         "[\n"
                                         "  {\n"
                                         "    \"txid\" : (string) The transaction id.\n"
                                         "    \"sender\" : (string)  + CURRENCY_UNIT +  address of the sender.\n"
                                         "    \"hash160\" : (string) ripemd-160 hash of the sender.\n"
                                         "  }\n"
                                         "]\n") +
                                 //},
                                 //RPCExamples{
                                 HelpExampleCli("sendtocontract",
                                                "\"c6ca2697719d00446d4ea51f6fac8fd1e9310214\" \"54f6127f\"")
                                 + HelpExampleCli("sendtocontract",
                                                  "\"c6ca2697719d00446d4ea51f6fac8fd1e9310214\" \"54f6127f\" 12.0015 6000000 " +
                                                  FormatMoney(minGasPrice) +
                                                  " \"QM72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\""));
        //           },
        //}.Check(request);
    }

    std::string contractaddress = params[0].get_str();
    if(contractaddress.size() != 40 || !CheckHex(contractaddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Incorrect contract address");

    dev::Address addrAccount(contractaddress);
    if(!globalState->addressInUse(addrAccount))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "contract address does not exist");

    std::string datahex = params[1].get_str();
    if(datahex.size() % 2 != 0 || !CheckHex(datahex))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid data (data not hex)");

    CAmount nAmount = 0;
    if (params.size() > 2){
        nAmount = params[2].get_int64();
        if (nAmount < 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount for send");
    }

    uint64_t nGasLimit=DEFAULT_GAS_LIMIT_OP_SEND;
    if (params.size() > 3){
        nGasLimit = params[3].get_int64();
        if (nGasLimit == blockGasLimit)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Maximum is: "+i64tostr(blockGasLimit)+")");
        if (nGasLimit < MINIMUM_GAS_LIMIT)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit (Minimum is: "+i64tostr(MINIMUM_GAS_LIMIT)+")");
        if (nGasLimit <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasLimit");
    }

    if (params.size() > 4){
        nGasPrice = params[4].get_int64();
        if (nGasPrice <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice");
        CAmount maxRpcGasPrice = 10000000; //gArgs.GetArg("-rpcmaxgasprice", MAX_RPC_GAS_PRICE);
        if (nGasPrice > (int64_t)maxRpcGasPrice)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice, Maximum allowed in RPC calls is: "+FormatMoney(maxRpcGasPrice)+" (use -rpcmaxgasprice to change it)");
        if (nGasPrice < (int64_t)minGasPrice)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid value for gasPrice (Minimum is: "+FormatMoney(minGasPrice)+")");
    }

    bool fHasSender=false;
    CTxDestination senderAddress;
    if (params.size() > 5){
        senderAddress = CBTCUAddress(params[5].get_str()).Get();
        if (!IsValidDestination(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Qtum address to send from");
        if (!IsValidContractSenderAddress(senderAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract sender address. Only P2PK and P2PKH allowed");
        else
            fHasSender=true;
    }

    bool fBroadcast=true;
    if (params.size() > 6){
        fBroadcast=params[6].get_bool();
    }

    bool fChangeToSender=true;
    if (params.size() > 7){
        fChangeToSender=params[7].get_bool();
    }

    CCoinControl coinControl;

    CTxDestination signSenderAddress = CNoDestination();
    if(fHasSender){
        // Find a UTXO with sender address
        std::vector<COutput> vecOutputs;

        coinControl.fAllowOtherInputs=true;

        assert(pwalletMain != NULL);
        pwalletMain->AvailableCoins(&vecOutputs, false, NULL, true);

        for (const COutput& out : vecOutputs) {

            CTxDestination destAdress;
            const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
            bool fValidAddress = ExtractDestination(scriptPubKey, destAdress);

            if (!fValidAddress || senderAddress != destAdress)
                continue;

            coinControl.Select(COutPoint(out.tx->GetHash(),out.i));

            break;

        }

        if(coinControl.HasSelected())
        {
            // Change to the sender
            if(fChangeToSender){
                coinControl.destChange=senderAddress;
            }
        }
        else
        {
            throw JSONRPCError(RPC_TYPE_ERROR, "Sender address does not have any unspent outputs");
        }
            signSenderAddress = senderAddress;
    }
    else
    {
            SetDefaultSignSenderAddress(pwalletMain, signSenderAddress);
    }

    EnsureWalletIsUnlocked(pwalletMain);

    CAmount nGasFee=nGasPrice*nGasLimit;

    CAmount curBalance = pwalletMain->GetBalance();

    // Check amount
    if (nGasFee <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount for gas fee");

    if (nAmount+nGasFee > curBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    // Select default coin that will pay for the contract if none selected
    if(!coinControl.HasSelected() && !SetDefaultPayForContractAddress(pwalletMain, coinControl))
        throw JSONRPCError(RPC_TYPE_ERROR, "Does not have any P2PK or P2PKH unspent outputs to pay for the contract.");

    // Build OP_EXEC_ASSIGN script
    CScript scriptPubKey = CScript() << CScriptNum(VersionVM::GetEVMDefault().toRaw()) << CScriptNum(nGasLimit) << CScriptNum(nGasPrice) << ParseHex(datahex) << ParseHex(contractaddress) << OP_CALL;
    //if(::ChainActive().Height() >= Params().GetConsensus().QIP5Height)
    //{
        if(IsValidDestination(signSenderAddress))
        {
            CKeyID key_id = GetKeyForDestination(*pwalletMain, signSenderAddress);
            CKey key;
            if (!pwalletMain->GetKey(key_id, key)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available");
            }
            std::vector<unsigned char> scriptSig;
            scriptSig.push_back(0);
            scriptPubKey = (CScript() << CScriptNum(addresstype::PUBKEYHASH) << ToByteVector(key_id) << ToByteVector(scriptSig) << OP_SENDER) + scriptPubKey;
        }
        else
        {
            // OP_SENDER will always be used when QIP5Height is active
            throw JSONRPCError(RPC_TYPE_ERROR, "Sender address fail to set for OP_SENDER.");
        }
    //}

    // Create and send the transaction
    CAmount nFeeRequired;
    std::string strError;
    int nChangePosRet = -1;

    CWalletTx wtx;
    CReserveKey reservekey(pwalletMain);
    if (!pwalletMain->CreateTransaction(scriptPubKey, nAmount, wtx, reservekey, nFeeRequired, strError, &coinControl, ALL_COINS, true, nGasFee, true)) {
        if (nFeeRequired > pwalletMain->GetBalance())
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    CTxDestination txSenderDest;
    GetSenderDest(pwalletMain, wtx, txSenderDest);

    if (fHasSender && !(senderAddress == txSenderDest)){
        throw JSONRPCError(RPC_TYPE_ERROR, "Sender could not be set, transaction was not committed!");
    }

    UniValue result(UniValue::VOBJ);

    if(fBroadcast){


        CValidationState state;
        if (!pwalletMain->CommitTransaction(wtx, reservekey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of the wallet and coins were spent in the copy but not marked as spent here.");

        std::string txId=wtx.GetHash().GetHex();
        result.pushKV("txid", txId);

        CTxDestination txSenderAdress(txSenderDest);
        CKeyID keyid = GetKeyForDestination(*pwalletMain, txSenderAdress);

        result.pushKV("sender", CBTCUAddress(txSenderAdress).ToString());
        result.pushKV("hash160", HexStr(valtype(keyid.begin(),keyid.end())));
    }else{
        //std::string strHex = EncodeHexTx(*tx, RPCSerializationFlags());
        //result.pushKV("raw transaction", strHex);
    }

    return result;
}

//////////////////////////////////////////////////////////////////////
UniValue listcontracts(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(std::string("listcontracts"
               "\nGet the contracts list.\n"));
               /*{
                       {"start", RPCArg::Type::NUM,  "1", "The starting account index"},
                       {"maxDisplay", RPCArg::Type::NUM,  "20", "Max accounts to list"},
               },
               RPCResult{
                       "{\n"
                       "  \"account\": n,                            (numeric) balance for the account\n"
                       "  ...\n"
                       "}\n"
               },
               RPCExamples{
                       HelpExampleCli("listcontracts", "")
                       + HelpExampleRpc("listcontracts", "")
               },
    }.Check(request);*/

    LOCK(cs_main);

    int start=1;
    if (params.size() > 0){
        start = params[0].get_int();
        if (start<= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid start, min=1");
    }

    int maxDisplay=20;
    if (params.size() > 1){
        maxDisplay = params[1].get_int();
        if (maxDisplay <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid maxDisplay");
    }

    UniValue result(UniValue::VOBJ);

    auto map = globalState->addresses();
    int contractsCount=(int)map.size();

    if (contractsCount>0 && start > contractsCount)
        throw JSONRPCError(RPC_TYPE_ERROR, "start greater than max index "+ itostr(contractsCount));

    int itStartPos=std::min(start-1,contractsCount);
    int i=0;
    for (auto it = std::next(map.begin(),itStartPos); it!=map.end(); it++)
    {
        result.pushKV(it->first.hex(),ValueFromAmount(CAmount(globalState->balance(it->first))));
        i++;
        if(i==maxDisplay)break;
    }

    return result;
}

UniValue executionResultToJSON(const dev::eth::ExecutionResult& exRes)
{
    UniValue result(UniValue::VOBJ);
    result.pushKV("gasUsed", CAmount(exRes.gasUsed));
    std::stringstream ss;
    ss << exRes.excepted;
    result.pushKV("excepted", ss.str());
    result.pushKV("newAddress", exRes.newAddress.hex());
    result.pushKV("output", HexStr(exRes.output));
    result.pushKV("codeDeposit", static_cast<int32_t>(exRes.codeDeposit));
    result.pushKV("gasRefunded", CAmount(exRes.gasRefunded));
    result.pushKV("depositSize", static_cast<int32_t>(exRes.depositSize));
    result.pushKV("gasForDeposit", CAmount(exRes.gasForDeposit));
    result.pushKV("exceptedMessage", exceptedMessage(exRes.excepted, exRes.output));
    return result;
}

UniValue transactionReceiptToJSON(const QtumTransactionReceipt& txRec)
{
    UniValue result(UniValue::VOBJ);
    result.pushKV("stateRoot", txRec.stateRoot().hex());
    result.pushKV("utxoRoot", txRec.utxoRoot().hex());
    result.pushKV("gasUsed", CAmount(txRec.cumulativeGasUsed()));
    result.pushKV("bloom", txRec.bloom().hex());
    UniValue logEntries(UniValue::VARR);
    dev::eth::LogEntries logs = txRec.log();
    for(dev::eth::LogEntry log : logs){
        UniValue logEntrie(UniValue::VOBJ);
        logEntrie.pushKV("address", log.address.hex());
        UniValue topics(UniValue::VARR);
        for(dev::h256 l : log.topics){
            topics.push_back(l.hex());
        }
        logEntrie.pushKV("topics", topics);
        logEntrie.pushKV("data", HexStr(log.data));
        logEntries.push_back(logEntrie);
    }
    result.pushKV("log", logEntries);
    return result;
}

UniValue callcontract(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw std::runtime_error(std::string("callcontract\n"
               "\nCall contract methods offline.\n"
               //{
                       "address, RPCArg::Type::STR_HEX, RPCArg::Optional::NO, The contract address\n"
                       "data RPCArg::Type::STR_HEX, RPCArg::Optional::NO, The data hex string\n"
                       "senderAddress RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, The sender address string\n"
                       "gasLimit RPCArg::Type::NUM, RPCArg::Optional::OMITTED_NAMED_ARG, The gas limit for executing the contract.\n"
               //},
               //RPCResult{
                       "{\n"
                       "  \"address\": \"contract address\",             (string)  address of the contract\n"
                       "  \"executionResult\": {                       (object)  method execution result\n"
                       "    \"gasUsed\": n,                            (numeric) gas used\n"
                       "    \"excepted\": \"exception\",                 (string)  thrown exception\n"
                       "    \"newAddress\": \"contract address\",        (string)  new address of the contract\n"
                       "    \"output\": \"data\",                        (string)  returned data from the method\n"
                       "    \"codeDeposit\": n,                        (numeric) code deposit\n"
                       "    \"gasRefunded\": n,                        (numeric) gas refunded\n"
                       "    \"depositSize\": n,                        (numeric) deposit size\n"
                       "    \"gasForDeposit\": n                       (numeric) gas for deposit\n"
                       "  },\n"
                       "  \"transactionReceipt\": {                    (object)  transaction receipt\n"
                       "    \"stateRoot\": \"hash\",                     (string)  state root hash\n"
                       "    \"gasUsed\": n,                            (numeric) gas used\n"
                       "    \"bloom\": \"bloom\",                        (string)  bloom\n"
                       "    \"log\": [                                 (array)  logs from the receipt\n"
                       "      {\n"
                       "        \"address\": \"address\",                (string)  contract address\n"
                       "        \"topics\":                            (array)  topics\n"
                       "        [\n"
                       "          \"topic\",                           (string)  topic\n"
                       "        ],\n"
                       "        \"data\": \"data\"                       (string)  logged data\n"
                       "      }\n"
                       "    ]\n"
                       "  }\n"
                       "}\n") +

               //RPCExamples{
                       HelpExampleCli("callcontract", "eb23c0b3e6042821da281a2e2364feb22dd543e3 06fdde03")
                       + HelpExampleRpc("callcontract", "eb23c0b3e6042821da281a2e2364feb22dd543e3 06fdde03"));
               //},
    //}.Check(request);

    LOCK(cs_main);

    std::string strAddr = params[0].get_str();
    std::string data = params[1].get_str();

    if(data.size() % 2 != 0 || !CheckHex(data))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid data (data not hex)");

    if(strAddr.size() != 40 || !CheckHex(strAddr))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Incorrect address");

    dev::Address addrAccount(strAddr);
    if(!globalState->addressInUse(addrAccount))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address does not exist");

    dev::Address senderAddress;
    if(params.size() >= 3){
        CTxDestination qtumSenderAddress = CBTCUAddress(params[2].get_str()).Get();
        if (IsValidDestination(qtumSenderAddress)) {
            const CKeyID *keyid = boost::get<CKeyID>(&qtumSenderAddress);
            senderAddress = dev::Address(HexStr(valtype(keyid->begin(),keyid->end())));
        }else{
            senderAddress = dev::Address(params[2].get_str());
        }

    }
    uint64_t gasLimit=0;
    if(params.size() >= 4){
        gasLimit = params[3].get_int64();
    }


    std::vector<ResultExecute> execResults = CallContract(addrAccount, ParseHex(data), senderAddress, gasLimit);

    if(fRecordLogOpcodes){
        writeVMlog(execResults);
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("address", strAddr);
    result.pushKV("executionResult", executionResultToJSON(execResults[0].execRes));
    result.pushKV("transactionReceipt", transactionReceiptToJSON(execResults[0].txRec));

    return result;
}

UniValue getnewaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "getnewaddress ( \"account\" )\n"
            "\nReturns a new BTCU address for receiving payments.\n"
            "If 'account' is specified (DEPRECATED), it is added to the address book \n"
            "so payments received with the address will be credited to 'account'.\n"

            "\nArguments:\n"
            "1. \"account\"        (string, optional) DEPRECATED. The account name for the address to be linked to. if not provided, the default account \"\" is used. It can also be set to the empty string \"\" to represent the default account. The account does not need to exist, it will be created if there is no account by the given name.\n"

            "\nResult:\n"
            "\"btcuaddress\"    (string) The new btcu address\n"

            "\nExamples:\n" +
            HelpExampleCli("getnewaddress", "") + HelpExampleRpc("getnewaddress", ""));

    return GetNewAddressFromAccount(AddressBook::AddressBookPurpose::RECEIVE, params).ToString();
}

UniValue getnewstakingaddress(const UniValue& params, bool fHelp)
{

    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "getnewstakingaddress ( \"account\" )\n"
            "\nReturns a new BTCU cold staking address for receiving delegated cold stakes.\n"

            "\nArguments:\n"
            "1. \"account\"        (string, optional) DEPRECATED. The account name for the address to be linked to. if not provided, the default account \"\" is used. It can also be set to the empty string \"\" to represent the default account. The account does not need to exist, it will be created if there is no account by the given name.\n"


            "\nResult:\n"
            "\"btcuaddress\"    (string) The new btcu address\n"

            "\nExamples:\n" +
            HelpExampleCli("getnewstakingaddress", "") + HelpExampleRpc("getnewstakingaddress", ""));

    return GetNewAddressFromAccount(AddressBook::AddressBookPurpose::COLD_STAKING, params, CChainParams::STAKING_ADDRESS).ToString();
}

UniValue getnewleasingaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw std::runtime_error(
            "getnewleasingaddress ( \"account\" )\n"
            "\nReturns a new BTCU leasing address for receiving leases.\n"

            "\nResult:\n"
            "\"btcuaddress\"    (string) The new btcu address\n"

            "\nExamples:\n" +
            HelpExampleCli("getnewleasingaddress", "") + HelpExampleRpc("getnewleasingaddress", ""));

    return GetNewAddressFromAccount(AddressBook::AddressBookPurpose::LEASING, params, CChainParams::PUBKEY_ADDRESS).ToString();
}

UniValue delegatoradd(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "delegatoradd \"addr\" ( \"label\" )\n"
            "\nAdd the provided address <addr> into the allowed delegators AddressBook.\n"
            "This enables the staking of coins delegated to this wallet, owned by <addr>\n"

            "\nArguments:\n"
            "1. \"addr\"        (string, required) The address to whitelist\n"
            "2. \"label\"       (string, optional) A label for the address to whitelist\n"

            "\nResult:\n"
            "true|false           (boolean) true if successful.\n"

            "\nExamples:\n" +
            HelpExampleCli("delegatoradd", "DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6") +
            HelpExampleRpc("delegatoradd", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\"") +
            HelpExampleRpc("delegatoradd", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" \"myPaperWallet\""));

    CBTCUAddress address(params[0].get_str());
    if (!address.IsValid() || address.IsStakingAddress())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU address");

    const std::string strLabel = (params.size() > 1 ? params[1].get_str() : "");

    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to get KeyID from BTCU address");

    return pwalletMain->SetAddressBook(keyID, strLabel, AddressBook::AddressBookPurpose::DELEGATOR);
}

UniValue delegatorremove(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
            "delegatorremove \"addr\"\n"
            "\nUpdates the provided address <addr> from the allowed delegators keystore to a \"delegable\" status.\n"
            "This disables the staking of coins delegated to this wallet, owned by <addr>\n"

            "\nArguments:\n"
            "1. \"addr\"        (string, required) The address to blacklist\n"

            "\nResult:\n"
            "true|false           (boolean) true if successful.\n"

            "\nExamples:\n" +
            HelpExampleCli("delegatorremove", "DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6") +
            HelpExampleRpc("delegatorremove", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\""));

    CBTCUAddress address(params[0].get_str());
    if (!address.IsValid() || address.IsStakingAddress())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU address");

    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to get KeyID from BTCU address");

    if (!pwalletMain->HasAddressBook(keyID))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to get BTCU address from addressBook");

    std::string label = "";
    {
        LOCK(pwalletMain->cs_wallet);
        std::map<CTxDestination, AddressBook::CAddressBookData>::iterator mi = pwalletMain->mapAddressBook.find(address.Get());
        if (mi != pwalletMain->mapAddressBook.end()) {
            label = mi->second.name;
        }
    }

    return pwalletMain->SetAddressBook(keyID, label, AddressBook::AddressBookPurpose::DELEGABLE);
}

UniValue ListaddressesForPurpose(const std::string strPurpose)
{
    const CChainParams::Base58Type addrType = [&]() -> CChainParams::Base58Type {
        if (AddressBook::IsColdStakingPurpose(strPurpose)) {
            return CChainParams::STAKING_ADDRESS;
        } else if (AddressBook::IsLeasingPurpose(strPurpose)) {
            return CChainParams::PUBKEY_ADDRESS;
        }
        return CChainParams::PUBKEY_ADDRESS;
    }();

    UniValue ret(UniValue::VARR);
    {
        LOCK(pwalletMain->cs_wallet);
        for (const auto& addr : pwalletMain->mapAddressBook) {
            if (addr.second.purpose != strPurpose) continue;
            UniValue entry(UniValue::VOBJ);
            entry.push_back(Pair("label", addr.second.name));
            entry.push_back(Pair("address", CBTCUAddress(addr.first, addrType).ToString()));
            ret.push_back(entry);
        }
    }

    return ret;
}

UniValue listdelegators(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "listdelegators ( fBlacklist )\n"
            "\nShows the list of allowed delegator addresses for cold staking.\n"

            "\nArguments:\n"
            "1. fBlacklist             (boolean, optional, default = false) Show addresses removed\n"
            "                          from the delegators whitelist\n"

            "\nResult:\n"
            "[\n"
            "   {\n"
            "   \"label\": \"yyy\",    (string) account label\n"
            "   \"address\": \"xxx\",  (string) BTCU address string\n"
            "   }\n"
            "  ...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("listdelegators" , "") +
            HelpExampleRpc("listdelegators", ""));

    const bool fBlacklist = (params.size() > 0 ? params[0].get_bool() : false);
    return (fBlacklist ?
            ListaddressesForPurpose(AddressBook::AddressBookPurpose::DELEGABLE) :
            ListaddressesForPurpose(AddressBook::AddressBookPurpose::DELEGATOR));
}

UniValue listleasees(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "listleasees ( fBlacklist )\n"
            "\nShows the list of allowed leasee addresses for leasing.\n"

            "\nArguments:\n"
            "1. fBlacklist             (boolean, optional, default = false) Show addresses removed\n"
            "                          from the leasees whitelist\n"

            "\nResult:\n"
            "[\n"
            "   {\n"
            "   \"label\": \"yyy\",    (string) account label\n"
            "   \"address\": \"xxx\",  (string) BTCU address string\n"
            "   }\n"
            "  ...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("listleasees" , "") +
            HelpExampleRpc("listleasees", ""));

    const bool fBlacklist = (params.size() > 0 ? params[0].get_bool() : false);
    return (fBlacklist ?
            ListaddressesForPurpose(AddressBook::AddressBookPurpose::LEASABLE) :
            ListaddressesForPurpose(AddressBook::AddressBookPurpose::LEASEE));
}

UniValue liststakingaddresses(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "liststakingaddresses \"addr\"\n"
            "\nShows the list of staking addresses for this wallet.\n"

            "\nResult:\n"
            "[\n"
            "   {\n"
            "   \"label\": \"yyy\",  (string) account label\n"
            "   \"address\": \"xxx\",  (string) BTCU address string\n"
            "   }\n"
            "  ...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("liststakingaddresses" , "") +
            HelpExampleRpc("liststakingaddresses", ""));

    return ListaddressesForPurpose(AddressBook::AddressBookPurpose::COLD_STAKING);
}

UniValue listleasingaddresses(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "listleasingaddresses \"addr\"\n"
            "\nShows the list of leasing addresses for this wallet.\n"

            "\nResult:\n"
            "[\n"
            "   {\n"
            "   \"label\": \"yyy\",  (string) account label\n"
            "   \"address\": \"xxx\",  (string) BTCU address string\n"
            "   }\n"
            "  ...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("listleasingaddresses" , "") +
            HelpExampleRpc("listleasingaddresses", ""));

    return ListaddressesForPurpose(AddressBook::AddressBookPurpose::LEASING);
}

CBTCUAddress GetAccountAddress(std::string strAccount, bool bForceNew = false)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);

    CAccount account;
    walletdb.ReadAccount(strAccount, account);

    bool bKeyUsed = false;

    // Check if the current key has been used
    if (account.vchPubKey.IsValid()) {
        CScript scriptPubKey = GetScriptForDestination(account.vchPubKey.GetID());
        for (std::map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin();
             it != pwalletMain->mapWallet.end() && account.vchPubKey.IsValid();
             ++it) {
            const CWalletTx& wtx = (*it).second;
            for (const CTxOut& txout : wtx.vout)
                if (txout.scriptPubKey == scriptPubKey)
                    bKeyUsed = true;
        }
    }

    // Generate a new key
    if (!account.vchPubKey.IsValid() || bForceNew || bKeyUsed) {
        if (!pwalletMain->GetKeyFromPool(account.vchPubKey))
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");

        pwalletMain->SetAddressBook(account.vchPubKey.GetID(), strAccount, AddressBook::AddressBookPurpose::RECEIVE);
        walletdb.WriteAccount(strAccount, account);
    }

    return CBTCUAddress(account.vchPubKey.GetID());
}

UniValue getaccountaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
            "getaccountaddress \"account\"\n"
            "\nDEPRECATED. Returns the current BTCU address for receiving payments to this account.\n"

            "\nArguments:\n"
            "1. \"account\"       (string, required) The account name for the address. It can also be set to the empty string \"\" to represent the default account. The account does not need to exist, it will be created and a new address created  if there is no account by the given name.\n"

            "\nResult:\n"
            "\"btcuaddress\"   (string) The account btcu address\n"

            "\nExamples:\n" +
            HelpExampleCli("getaccountaddress", "") + HelpExampleCli("getaccountaddress", "\"\"") +
            HelpExampleCli("getaccountaddress", "\"myaccount\"") + HelpExampleRpc("getaccountaddress", "\"myaccount\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    // Parse the account first so we don't generate a key if there's an error
    std::string strAccount = AccountFromValue(params[0]);

    UniValue ret(UniValue::VSTR);

    ret = GetAccountAddress(strAccount).ToString();
    return ret;
}


UniValue getrawchangeaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "getrawchangeaddress\n"
            "\nReturns a new BTCU address, for receiving change.\n"
            "This is for use with raw transactions, NOT normal use.\n"

            "\nResult:\n"
            "\"address\"    (string) The address\n"

            "\nExamples:\n" +
            HelpExampleCli("getrawchangeaddress", "") + HelpExampleRpc("getrawchangeaddress", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();

    CReserveKey reservekey(pwalletMain);
    CPubKey vchPubKey;
    if (!reservekey.GetReservedKey(vchPubKey))
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");

    reservekey.KeepKey();

    CKeyID keyID = vchPubKey.GetID();

    return CBTCUAddress(keyID).ToString();
}


UniValue setaccount(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "setaccount \"btcuaddress\" \"account\"\n"
            "\nDEPRECATED. Sets the account associated with the given address.\n"

            "\nArguments:\n"
            "1. \"btcuaddress\"  (string, required) The btcu address to be associated with an account.\n"
            "2. \"account\"         (string, required) The account to assign the address to.\n"

            "\nExamples:\n" +
            HelpExampleCli("setaccount", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" \"tabby\"") + HelpExampleRpc("setaccount", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\", \"tabby\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CBTCUAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU address");


    std::string strAccount;
    if (params.size() > 1)
        strAccount = AccountFromValue(params[1]);

    // Only add the account if the address is yours.
    if (IsMine(*pwalletMain, address.Get())) {
        // Detect when changing the account of an address that is the 'unused current key' of another account:
        if (pwalletMain->mapAddressBook.count(address.Get())) {
            std::string strOldAccount = pwalletMain->mapAddressBook[address.Get()].name;
            if (address == GetAccountAddress(strOldAccount))
                GetAccountAddress(strOldAccount, true);
        }
        pwalletMain->SetAddressBook(address.Get(), strAccount, AddressBook::AddressBookPurpose::RECEIVE);
    } else
        throw JSONRPCError(RPC_MISC_ERROR, "setaccount can only be used with own address");

    return NullUniValue;
}


UniValue getaccount(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
            "getaccount \"btcuaddress\"\n"
            "\nDEPRECATED. Returns the account associated with the given address.\n"

            "\nArguments:\n"
            "1. \"btcuaddress\"  (string, required) The btcu address for account lookup.\n"

            "\nResult:\n"
            "\"accountname\"        (string) the account address\n"

            "\nExamples:\n" +
            HelpExampleCli("getaccount", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\"") + HelpExampleRpc("getaccount", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CBTCUAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU address");

    std::string strAccount;
    std::map<CTxDestination, AddressBook::CAddressBookData>::iterator mi = pwalletMain->mapAddressBook.find(address.Get());
    if (mi != pwalletMain->mapAddressBook.end() && !(*mi).second.name.empty())
        strAccount = (*mi).second.name;
    return strAccount;
}


UniValue getaddressesbyaccount(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
            "getaddressesbyaccount \"account\"\n"
            "\nDEPRECATED. Returns the list of addresses for the given account.\n"

            "\nArguments:\n"
            "1. \"account\"  (string, required) The account name.\n"

            "\nResult:\n"
            "[                     (json array of string)\n"
            "  \"btcuaddress\"  (string) a btcu address associated with the given account\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("getaddressesbyaccount", "\"tabby\"") + HelpExampleRpc("getaddressesbyaccount", "\"tabby\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    std::string strAccount = AccountFromValue(params[0]);

    // Find all addresses that have the given account
    UniValue ret(UniValue::VARR);
    for (const PAIRTYPE(CBTCUAddress, AddressBook::CAddressBookData) & item : pwalletMain->mapAddressBook) {
        const CBTCUAddress& address = item.first;
        const std::string& strName = item.second.name;
        if (strName == strAccount)
            ret.push_back(address.ToString());
    }
    return ret;
}

void SendMoney(const CTxDestination& address, CAmount nValue, CWalletTx& wtxNew, bool fUseIX = false,
        const std::vector<CValidatorRegister> &validatorRegister = std::vector<CValidatorRegister>(),
        const std::vector<CValidatorVote> &validatorVote = std::vector<CValidatorVote>())
{
    // Check amount
    if (nValue <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount");

    if (nValue > pwalletMain->GetBalance())
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    std::string strError;
    if (pwalletMain->IsLocked()) {
        strError = "Error: Wallet locked, unable to create transaction!";
        LogPrintf("SendMoney() : %s", strError);
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    // Parse BTCU address
    CScript scriptPubKey = GetScriptForDestination(address);

    // Create and send the transaction
    CReserveKey reservekey(pwalletMain);
    CAmount nFeeRequired;
    if (!pwalletMain->CreateTransaction(scriptPubKey, nValue, wtxNew, reservekey, nFeeRequired, strError, NULL, ALL_COINS, fUseIX, (CAmount)0, false, false,false, CNoDestination(),validatorRegister, validatorVote)) {
        if (nValue + nFeeRequired > pwalletMain->GetBalance())
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
        LogPrintf("SendMoney() : %s\n", strError);
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    if (!pwalletMain->CommitTransaction(wtxNew, reservekey, (!fUseIX ? "tx" : "ix")))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
}

UniValue sendtoaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 4)
        throw std::runtime_error(
            "sendtoaddress \"btcuaddress\" amount ( \"comment\" \"comment-to\" )\n"
            "\nSend an amount to a given address. The amount is a real and is rounded to the nearest 0.00000001\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"btcuaddress\"  (string, required) The btcu address to send to.\n"
            "2. \"amount\"      (numeric, required) The amount in BTCU to send. eg 0.1\n"
            "3. \"comment\"     (string, optional) A comment used to store what the transaction is for. \n"
            "                             This is not part of the transaction, just kept in your wallet.\n"
            "4. \"comment-to\"  (string, optional) A comment to store the name of the person or organization \n"
            "                             to which you're sending the transaction. This is not part of the \n"
            "                             transaction, just kept in your wallet.\n"

            "\nResult:\n"
            "\"transactionid\"  (string) The transaction id.\n"

            "\nExamples:\n" +
            HelpExampleCli("sendtoaddress", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" 0.1") +
            HelpExampleCli("sendtoaddress", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" 0.1 \"donation\" \"seans outpost\"") +
            HelpExampleRpc("sendtoaddress", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\", 0.1, \"donation\", \"seans outpost\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CBTCUAddress address(params[0].get_str());
    if (!address.IsValid() || address.IsStakingAddress())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU address");

    // Amount
    CAmount nAmount = AmountFromValue(params[1]);

    // Wallet comments
    CWalletTx wtx;
    if (params.size() > 2 && !params[2].isNull() && !params[2].get_str().empty())
        wtx.mapValue["comment"] = params[2].get_str();
    if (params.size() > 3 && !params[3].isNull() && !params[3].get_str().empty())
        wtx.mapValue["to"] = params[3].get_str();

    EnsureWalletIsUnlocked();

    SendMoney(address.Get(), nAmount, wtx);

    return wtx.GetHash().GetHex();
}

UniValue CreateColdStakeDelegation(const UniValue& params, CWalletTx& wtxNew, CReserveKey& reservekey)
{
    LOCK2(cs_main, pwalletMain->cs_wallet);

    // Check that Cold Staking has been enforced or fForceNotEnabled = true
    bool fForceNotEnabled = false;
    if (params.size() > 5 && !params[5].isNull())
        fForceNotEnabled = params[5].get_bool();

    if (!sporkManager.IsSporkActive(SPORK_17_COLDSTAKING_ENFORCEMENT) && !fForceNotEnabled) {
        std::string errMsg = "Cold Staking disabled with SPORK 17.\n"
                "You may force the stake delegation setting fForceNotEnabled to true.\n"
                "WARNING: If relayed before activation, this tx will be rejected resulting in a ban.\n";
        throw JSONRPCError(RPC_WALLET_ERROR, errMsg);
    }

    // Get Staking Address
    CBTCUAddress stakeAddr(params[0].get_str());
    CKeyID stakeKey;
    if (!stakeAddr.IsValid() || !stakeAddr.IsStakingAddress())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU staking address");
    if (!stakeAddr.GetKeyID(stakeKey))
        throw JSONRPCError(RPC_WALLET_ERROR, "Unable to get stake pubkey hash from stakingaddress");

    // Get Amount
    CAmount nValue = AmountFromValue(params[1]);
    if (nValue < Params().GetMinColdStakingAmount())
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid amount (%d). Min amount: %d",
                nValue, Params().GetMinColdStakingAmount()));

    // include already delegated coins
    bool fUseDelegated = false;
    if (params.size() > 4 && !params[4].isNull())
        fUseDelegated = params[4].get_bool();

    // Check amount
    CAmount currBalance = pwalletMain->GetBalance() + (fUseDelegated ? pwalletMain->GetDelegatedBalance() : 0);
    if (nValue > currBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    std::string strError;
    EnsureWalletIsUnlocked();

    // Get Owner Address
    CBTCUAddress ownerAddr;
    CKeyID ownerKey;
    if (params.size() > 2 && !params[2].isNull() && !params[2].get_str().empty()) {
        // Address provided
        ownerAddr.SetString(params[2].get_str());
        if (!ownerAddr.IsValid() || ownerAddr.IsStakingAddress())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU spending address");
        if (!ownerAddr.GetKeyID(ownerKey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Unable to get spend pubkey hash from owneraddress");
        // Check that the owner address belongs to this wallet, or fForceExternalAddr is true
        bool fForceExternalAddr = params.size() > 3 && !params[3].isNull() ? params[3].get_bool() : false;
        if (!fForceExternalAddr && !pwalletMain->HaveKey(ownerKey)) {
            std::string errMsg = strprintf("The provided owneraddress \"%s\" is not present in this wallet.\n"
                    "Set 'fExternalOwner' argument to true, in order to force the stake delegation to an external owner address.\n"
                    "e.g. delegatestake stakingaddress amount owneraddress true.\n"
                    "WARNING: Only the owner of the key to owneraddress will be allowed to spend these coins after the delegation.",
                    ownerAddr.ToString());
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, errMsg);
        }

    } else {
        // Get new owner address from keypool
        ownerAddr = GetNewAddressFromAccount("delegated", NullUniValue);
        if (!ownerAddr.GetKeyID(ownerKey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Unable to get spend pubkey hash from owneraddress");
    }

    // Get P2CS script for addresses
    CScript scriptPubKey = GetScriptForStakeDelegation(stakeKey, ownerKey);

    // Create the transaction
    CAmount nFeeRequired;
    if (!pwalletMain->CreateTransaction(scriptPubKey, nValue, wtxNew, reservekey, nFeeRequired, strError, NULL, ALL_COINS, /*fUseIX*/ false, (CAmount)0, fUseDelegated)) {
        if (nValue + nFeeRequired > currBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
        LogPrintf("%s : %s\n", __func__, strError);
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("owner_address", ownerAddr.ToString()));
    result.push_back(Pair("staker_address", stakeAddr.ToString()));
    return result;
}

UniValue delegatestake(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 6)
        throw std::runtime_error(
            "delegatestake \"stakingaddress\" amount ( \"owneraddress\" fExternalOwner fUseDelegated fForceNotEnabled )\n"
            "\nDelegate an amount to a given address for cold staking. The amount is a real and is rounded to the nearest 0.00000001\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"stakingaddress\"      (string, required) The btcu staking address to delegate.\n"
            "2. \"amount\"              (numeric, required) The amount in BTCU to delegate for staking. eg 100\n"
            "3. \"owneraddress\"        (string, optional) The btcu address corresponding to the key that will be able to spend the stake. \n"
            "                               If not provided, or empty string, a new wallet address is generated.\n"
            "4. \"fExternalOwner\"      (boolean, optional, default = false) use the provided 'owneraddress' anyway, even if not present in this wallet.\n"
            "                               WARNING: The owner of the keys to 'owneraddress' will be the only one allowed to spend these coins.\n"
            "5. \"fUseDelegated\"       (boolean, optional, default = false) include already delegated inputs if needed."
            "6. \"fForceNotEnabled\"    (boolean, optional, default = false) force the creation even if SPORK 17 is disabled (for tests)."

            "\nResult:\n"
            "{\n"
            "   \"owner_address\": \"xxx\"   (string) The owner (delegator) owneraddress.\n"
            "   \"staker_address\": \"xxx\"  (string) The cold staker (delegate) stakingaddress.\n"
            "   \"txid\": \"xxx\"            (string) The stake delegation transaction id.\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("delegatestake", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\" 100") +
            HelpExampleCli("delegatestake", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\" 1000 \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg34fk\"") +
            HelpExampleRpc("delegatestake", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\", 1000, \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg34fk\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CWalletTx wtx;
    CReserveKey reservekey(pwalletMain);
    UniValue ret = CreateColdStakeDelegation(params, wtx, reservekey);

    if (!pwalletMain->CommitTransaction(wtx, reservekey, "tx"))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");

    ret.push_back(Pair("txid", wtx.GetHash().GetHex()));
    return ret;
}

UniValue rawdelegatestake(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 5)
        throw std::runtime_error(
            "rawdelegatestake \"stakingaddress\" amount ( \"owneraddress\" fExternalOwner fUseDelegated )\n"
            "\nDelegate an amount to a given address for cold staking. The amount is a real and is rounded to the nearest 0.00000001\n"
            "\nDelegate transaction is returned as json object." +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"stakingaddress\"      (string, required) The btcu staking address to delegate.\n"
            "2. \"amount\"              (numeric, required) The amount in BTCU to delegate for staking. eg 100\n"
            "3. \"owneraddress\"        (string, optional) The btcu address corresponding to the key that will be able to spend the stake. \n"
            "                               If not provided, or empty string, a new wallet address is generated.\n"
            "4. \"fExternalOwner\"      (boolean, optional, default = false) use the provided 'owneraddress' anyway, even if not present in this wallet.\n"
            "                               WARNING: The owner of the keys to 'owneraddress' will be the only one allowed to spend these coins.\n"
            "5. \"fUseDelegated         (boolean, optional, default = false) include already delegated inputs if needed."

            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"id\",        (string) The transaction id (same as provided)\n"
            "  \"version\" : n,          (numeric) The version\n"
            "  \"size\" : n,             (numeric) The serialized transaction size\n"
            "  \"locktime\" : ttt,       (numeric) The lock time\n"
            "  \"vin\" : [               (array of json objects)\n"
            "     {\n"
            "       \"txid\": \"id\",    (string) The transaction id\n"
            "       \"vout\": n,         (numeric) \n"
            "       \"scriptSig\": {     (json object) The script\n"
            "         \"asm\": \"asm\",  (string) asm\n"
            "         \"hex\": \"hex\"   (string) hex\n"
            "       },\n"
            "       \"sequence\": n      (numeric) The script sequence number\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"vout\" : [              (array of json objects)\n"
            "     {\n"
            "       \"value\" : x.xxx,            (numeric) The value in btc\n"
            "       \"n\" : n,                    (numeric) index\n"
            "       \"scriptPubKey\" : {          (json object)\n"
            "         \"asm\" : \"asm\",          (string) the asm\n"
            "         \"hex\" : \"hex\",          (string) the hex\n"
            "         \"reqSigs\" : n,            (numeric) The required sigs\n"
            "         \"type\" : \"pubkeyhash\",  (string) The type, eg 'pubkeyhash'\n"
            "         \"addresses\" : [           (json array of string)\n"
            "           \"btcuaddress\"        (string) btcu address\n"
            "           ,...\n"
            "         ]\n"
            "       }\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"hex\" : \"data\",       (string) The serialized, hex-encoded data for 'txid'\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("rawdelegatestake", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\" 100") +
            HelpExampleCli("rawdelegatestake", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\" 1000 \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg34fk\"") +
            HelpExampleRpc("rawdelegatestake", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\", 1000, \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg34fk\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CWalletTx wtx;
    CReserveKey reservekey(pwalletMain);
    CreateColdStakeDelegation(params, wtx, reservekey);

    UniValue result(UniValue::VOBJ);
    TxToUniv(wtx, 0, result);

    return result;
}

UniValue CreateLeasingTransaction(const UniValue& params, CWalletTx& wtxNew, CReserveKey& reservekey)
{
    LOCK2(cs_main, pwalletMain->cs_wallet);

    // Check that Leasing has been enforced or fForceNotEnabled = true
    bool fForceNotEnabled = false;
    if (params.size() > 4 && !params[4].isNull())
        fForceNotEnabled = params[4].get_bool();

    if (!sporkManager.IsSporkActive(SPORK_1017_LEASING_ENFORCEMENT) && !fForceNotEnabled) {
        std::string errMsg = "Leasing disabled with SPORK 1017.\n"
                             "You may force the leasing setting fForceNotEnabled to true.\n"
                             "WARNING: If relayed before activation, this tx will be rejected resulting in a ban.\n";
        throw JSONRPCError(RPC_WALLET_ERROR, errMsg);
    }

    // Get Leasing Address
    CBTCUAddress leaserAddr(params[0].get_str());
    CKeyID leaserKey;
    if (!leaserAddr.IsValid() || !leaserAddr.IsLeasingAddress())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU leasing address");
    if (!leaserAddr.GetKeyID(leaserKey))
        throw JSONRPCError(RPC_WALLET_ERROR, "Unable to get leaser pubkey hash from leasingaddress");

    // Get Amount
    CAmount nValue = AmountFromValue(params[1]);
    if (nValue < Params().GetMinLeasingAmount())
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid amount (%d). Min amount: %d",
                                                            nValue, Params().GetMinLeasingAmount()));
    // Check amount
    CAmount currBalance = pwalletMain->GetBalance();
    if (nValue > currBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    std::string strError;
    EnsureWalletIsUnlocked();

    // Get Owner Address
    CBTCUAddress ownerAddr;
    CKeyID ownerKey;
    if (params.size() > 2 && !params[2].isNull() && !params[2].get_str().empty()) {
        // Address provided
        ownerAddr.SetString(params[2].get_str());
        if (!ownerAddr.IsValid() || ownerAddr.IsLeasingAddress())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU spending address");
        if (!ownerAddr.GetKeyID(ownerKey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Unable to get spend pubkey hash from owneraddress");
        // Check that the owner address belongs to this wallet, or fForceExternalAddr is true
        bool fForceExternalAddr = params.size() > 3 && !params[3].isNull() ? params[3].get_bool() : false;
        if (!fForceExternalAddr && !pwalletMain->HaveKey(ownerKey)) {
            std::string errMsg = strprintf("The provided owneraddress \"%s\" is not present in this wallet.\n"
                                           "Set 'fExternalOwner' argument to true, in order to force the leasing to an external owner address.\n"
                                           "e.g. leasetoaddress leasingaddress amount owneraddress true.\n"
                                           "WARNING: Only the owner of the key to owneraddress will be allowed to spend these coins after the leasing.",
                                           ownerAddr.ToString());
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, errMsg);
        }

    } else {
        // Get new owner address from keypool
        ownerAddr = GetNewAddressFromAccount(AddressBook::AddressBookPurpose::LEASED, NullUniValue);
        if (!ownerAddr.GetKeyID(ownerKey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Unable to get spend pubkey hash from owneraddress");
    }

    // Get P2L script for addresses
    CScript scriptPubKey = GetScriptForLeasing(leaserKey, ownerKey);

    // Create the transaction
    CAmount nFeeRequired;
    if (!pwalletMain->CreateTransaction(scriptPubKey, nValue, wtxNew, reservekey, nFeeRequired, strError, NULL, ALL_COINS, /*fUseIX*/ false, (CAmount)0)) {
        if (nValue + nFeeRequired > currBalance)
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
        LogPrintf("%s : %s\n", __func__, strError);
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("owner_address", ownerAddr.ToString()));
    result.push_back(Pair("leaser_address", leaserAddr.ToString()));
    return result;
}

UniValue leasetoaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 5)
        throw std::runtime_error(
            "leasetoaddress \"leasingaddress\" amount ( \"owneraddress\" fExternalOwner fForceNotEnabled)\n"
            "\nLease an amount to a given address for mining. The amount is a real and is rounded to the nearest 0.00000001\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"leasingaddress\"      (string, required) The btcu leasing address to lease.\n"
            "2. \"amount\"              (numeric, required) The amount in BTCU to lease for mining. eg 100\n"
            "3. \"owneraddress\"        (string, optional) The btcu address corresponding to the key that will be able to spend the rewards. \n"
            "                               If not provided, or empty string, a new wallet address is generated.\n"
            "4. \"fExternalOwner\"      (boolean, optional, default = false) use the provided 'owneraddress' anyway, even if not present in this wallet.\n"
            "                               WARNING: The owner of the keys to 'owneraddress' will be the only one allowed to spend these rewards.\n"
            "5. \"fForceNotEnabled\"    (boolean, optional, default = false) force the creation even if SPORK 1017 is disabled (for tests)."

            "\nResult:\n"
            "{\n"
            "   \"owner_address\": \"xxx\"   (string) The owner (leasee) owneraddress.\n"
            "   \"leaser_address\": \"xxx\"  (string) The miner (leaser) leasingaddress.\n"
            "   \"txid\": \"xxx\"            (string) The leasing transaction id.\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("leasetoaddress", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\" 100") +
            HelpExampleCli("leasetoaddress", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\" 1000 \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg34fk\"") +
            HelpExampleRpc("leasetoaddress", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\", 1000, \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg34fk\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CWalletTx wtx;
    CReserveKey reservekey(pwalletMain);
    UniValue ret = CreateLeasingTransaction(params, wtx, reservekey);

    if (!pwalletMain->CommitTransaction(wtx, reservekey, "tx"))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");

    ret.push_back(Pair("txid", wtx.GetHash().GetHex()));
    return ret;
}

UniValue rawleasetoaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 4)
        throw std::runtime_error(
            "rawleasetoaddress \"leasingaddress\" amount ( \"owneraddress\" fExternalOwner )\n"
            "\nLease an amount to a given address for mining. The amount is a real and is rounded to the nearest 0.00000001\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"leasingaddress\"      (string, required) The btcu leasing address to lease.\n"
            "2. \"amount\"              (numeric, required) The amount in BTCU to lease for mining. eg 100\n"
            "3. \"owneraddress\"        (string, optional) The btcu address corresponding to the key that will be able to spend the rewards. \n"
            "                               If not provided, or empty string, a new wallet address is generated.\n"
            "4. \"fExternalOwner\"      (boolean, optional, default = false) use the provided 'owneraddress' anyway, even if not present in this wallet.\n"
            "                               WARNING: The owner of the keys to 'owneraddress' will be the only one allowed to spend these rewards.\n"
            "5. \"fForceNotEnabled\"    (boolean, optional, default = false) force the creation even if SPORK 1017 is disabled (for tests)."

            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"id\",        (string) The transaction id (same as provided)\n"
            "  \"version\" : n,          (numeric) The version\n"
            "  \"size\" : n,             (numeric) The serialized transaction size\n"
            "  \"locktime\" : ttt,       (numeric) The lock time\n"
            "  \"vin\" : [               (array of json objects)\n"
            "     {\n"
            "       \"txid\": \"id\",    (string) The transaction id\n"
            "       \"vout\": n,         (numeric) \n"
            "       \"scriptSig\": {     (json object) The script\n"
            "         \"asm\": \"asm\",  (string) asm\n"
            "         \"hex\": \"hex\"   (string) hex\n"
            "       },\n"
            "       \"sequence\": n      (numeric) The script sequence number\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"vout\" : [              (array of json objects)\n"
            "     {\n"
            "       \"value\" : x.xxx,            (numeric) The value in btc\n"
            "       \"n\" : n,                    (numeric) index\n"
            "       \"scriptPubKey\" : {          (json object)\n"
            "         \"asm\" : \"asm\",          (string) the asm\n"
            "         \"hex\" : \"hex\",          (string) the hex\n"
            "         \"reqSigs\" : n,            (numeric) The required sigs\n"
            "         \"type\" : \"pubkeyhash\",  (string) The type, eg 'pubkeyhash'\n"
            "         \"addresses\" : [           (json array of string)\n"
            "           \"btcuaddress\"           (string) btcu address\n"
            "           ,...\n"
            "         ]\n"
            "       }\n"
            "     }\n"
            "     ,...\n"
            "  ],\n"
            "  \"hex\" : \"data\",       (string) The serialized, hex-encoded data for 'txid'\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("rawleasetoaddress", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\" 100") +
            HelpExampleCli("rawleasetoaddress", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\" 1000 \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg34fk\"") +
            HelpExampleRpc("rawleasetoaddress", "\"S1t2a3kab9c8c71VA78xxxy4MxZg6vgeS6\", 1000, \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg34fk\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CWalletTx wtx;
    CReserveKey reservekey(pwalletMain);
    UniValue ret = CreateLeasingTransaction(params, wtx, reservekey);

    UniValue result(UniValue::VOBJ);
    TxToUniv(wtx, 0, result);

    return ret;
}

UniValue sendtoaddressix(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 4)
        throw std::runtime_error(
            "sendtoaddressix \"btcuaddress\" amount ( \"comment\" \"comment-to\" )\n"
            "\nSend an amount to a given address. The amount is a real and is rounded to the nearest 0.00000001\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"btcuaddress\"  (string, required) The btcu address to send to.\n"
            "2. \"amount\"      (numeric, required) The amount in BTCU to send. eg 0.1\n"
            "3. \"comment\"     (string, optional) A comment used to store what the transaction is for. \n"
            "                             This is not part of the transaction, just kept in your wallet.\n"
            "4. \"comment-to\"  (string, optional) A comment to store the name of the person or organization \n"
            "                             to which you're sending the transaction. This is not part of the \n"
            "                             transaction, just kept in your wallet.\n"

            "\nResult:\n"
            "\"transactionid\"  (string) The transaction id.\n"

            "\nExamples:\n" +
            HelpExampleCli("sendtoaddressix", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" 0.1") +
            HelpExampleCli("sendtoaddressix", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" 0.1 \"donation\" \"seans outpost\"") +
            HelpExampleRpc("sendtoaddressix", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\", 0.1, \"donation\", \"seans outpost\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CBTCUAddress address(params[0].get_str());
    if (!address.IsValid() || address.IsStakingAddress())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU address");

    // Amount
    CAmount nAmount = AmountFromValue(params[1]);

    // Wallet comments
    CWalletTx wtx;
    if (params.size() > 2 && !params[2].isNull() && !params[2].get_str().empty())
        wtx.mapValue["comment"] = params[2].get_str();
    if (params.size() > 3 && !params[3].isNull() && !params[3].get_str().empty())
        wtx.mapValue["to"] = params[3].get_str();

    EnsureWalletIsUnlocked();

    SendMoney(address.Get(), nAmount, wtx, true);

    return wtx.GetHash().GetHex();
}

UniValue listaddressgroupings(const UniValue& params, bool fHelp)
{
    if (fHelp)
        throw std::runtime_error(
            "listaddressgroupings\n"
            "\nLists groups of addresses which have had their common ownership\n"
            "made public by common use as inputs or as the resulting change\n"
            "in past transactions\n"

            "\nResult:\n"
            "[\n"
            "  [\n"
            "    [\n"
            "      \"btcuaddress\",     (string) The btcu address\n"
            "      amount,                 (numeric) The amount in BTCU\n"
            "      \"account\"             (string, optional) The account (DEPRECATED)\n"
            "    ]\n"
            "    ,...\n"
            "  ]\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("listaddressgroupings", "") + HelpExampleRpc("listaddressgroupings", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    UniValue jsonGroupings(UniValue::VARR);
    std::map<CTxDestination, CAmount> balances = pwalletMain->GetAddressBalances();
    for (std::set<CTxDestination> grouping : pwalletMain->GetAddressGroupings()) {
        UniValue jsonGrouping(UniValue::VARR);
        for (CTxDestination address : grouping) {
            UniValue addressInfo(UniValue::VARR);
            addressInfo.push_back(CBTCUAddress(address).ToString());
            addressInfo.push_back(ValueFromAmount(balances[address]));
            {
                if (pwalletMain->mapAddressBook.find(CBTCUAddress(address).Get()) != pwalletMain->mapAddressBook.end())
                    addressInfo.push_back(pwalletMain->mapAddressBook.find(CBTCUAddress(address).Get())->second.name);
            }
            jsonGrouping.push_back(addressInfo);
        }
        jsonGroupings.push_back(jsonGrouping);
    }
    return jsonGroupings;
}

UniValue signmessage(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw std::runtime_error(
            "signmessage \"btcuaddress\" \"message\"\n"
            "\nSign a message with the private key of an address" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"btcuaddress\"  (string, required) The btcu address to use for the private key.\n"
            "2. \"message\"         (string, required) The message to create a signature of.\n"

            "\nResult:\n"
            "\"signature\"          (string) The signature of the message encoded in base 64\n"

            "\nExamples:\n"
            "\nUnlock the wallet for 30 seconds\n" +
            HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n" +
            HelpExampleCli("signmessage", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" \"my message\"") +
            "\nVerify the signature\n" +
            HelpExampleCli("verifymessage", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" \"signature\" \"my message\"") +
            "\nAs json rpc\n" +
            HelpExampleRpc("signmessage", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\", \"my message\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    std::string strAddress = params[0].get_str();
    std::string strMessage = params[1].get_str();

    CBTCUAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    CKey key;
    if (!pwalletMain->GetKey(keyID, key))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(&vchSig[0], vchSig.size());
}

UniValue getreceivedbyaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "getreceivedbyaddress \"btcuaddress\" ( minconf )\n"
            "\nReturns the total amount received by the given btcuaddress in transactions with at least minconf confirmations.\n"

            "\nArguments:\n"
            "1. \"btcuaddress\"  (string, required) The btcu address for transactions.\n"
            "2. minconf             (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"

            "\nResult:\n"
            "amount   (numeric) The total amount in BTCU received at this address.\n"

            "\nExamples:\n"
            "\nThe amount from transactions with at least 1 confirmation\n" +
            HelpExampleCli("getreceivedbyaddress", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\"") +
            "\nThe amount including unconfirmed transactions, zero confirmations\n" +
            HelpExampleCli("getreceivedbyaddress", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" 0") +
            "\nThe amount with at least 6 confirmation, very safe\n" +
            HelpExampleCli("getreceivedbyaddress", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" 6") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("getreceivedbyaddress", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\", 6"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    // btcu address
    CBTCUAddress address = CBTCUAddress(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU address");
    CScript scriptPubKey = GetScriptForDestination(address.Get());
    if (!IsMine(*pwalletMain, scriptPubKey))
        throw JSONRPCError(RPC_WALLET_ERROR, "Address not found in wallet");

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Tally
    CAmount nAmount = 0;
    for (std::map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it) {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsCoinBase() || !IsFinalTx(wtx))
            continue;

        for (const CTxOut& txout : wtx.vout)
            if (txout.scriptPubKey == scriptPubKey)
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
    }

    return ValueFromAmount(nAmount);
}


UniValue getreceivedbyaccount(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "getreceivedbyaccount \"account\" ( minconf )\n"
            "\nDEPRECATED. Returns the total amount received by addresses with <account> in transactions with at least [minconf] confirmations.\n"

            "\nArguments:\n"
            "1. \"account\"      (string, required) The selected account, may be the default account using \"\".\n"
            "2. minconf          (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"

            "\nResult:\n"
            "amount              (numeric) The total amount in BTCU received for this account.\n"

            "\nExamples:\n"
            "\nAmount received by the default account with at least 1 confirmation\n" +
            HelpExampleCli("getreceivedbyaccount", "\"\"") +
            "\nAmount received at the tabby account including unconfirmed amounts with zero confirmations\n" +
            HelpExampleCli("getreceivedbyaccount", "\"tabby\" 0") +
            "\nThe amount with at least 6 confirmation, very safe\n" +
            HelpExampleCli("getreceivedbyaccount", "\"tabby\" 6") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("getreceivedbyaccount", "\"tabby\", 6"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Get the set of pub keys assigned to account
    std::string strAccount = AccountFromValue(params[0]);
    std::set<CTxDestination> setAddress = pwalletMain->GetAccountAddresses(strAccount);

    // Tally
    CAmount nAmount = 0;
    for (std::map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it) {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsCoinBase() || !IsFinalTx(wtx))
            continue;

        for (const CTxOut& txout : wtx.vout) {
            CTxDestination address;
            if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*pwalletMain, address) && setAddress.count(address))
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
        }
    }

    return (double)nAmount / (double)COIN;
}


CAmount GetAccountBalance(CWalletDB& walletdb, const std::string& strAccount, int nMinDepth, const isminefilter& filter)
{
    CAmount nBalance = 0;

    // Tally wallet transactions
    for (std::map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it) {
        const CWalletTx& wtx = (*it).second;
        bool fConflicted;
        int depth = wtx.GetDepthAndMempool(fConflicted);

        if (!IsFinalTx(wtx) || wtx.GetBlocksToMaturity() > 0 || depth < 0 || fConflicted)
            continue;

        if (strAccount == "*") {
            // Calculate total balance a different way from GetBalance()
            // (GetBalance() sums up all unspent TxOuts)
            CAmount allFee;
            std::string strSentAccount;
            std::list<COutputEntry> listReceived;
            std::list<COutputEntry> listSent;
            wtx.GetAmounts(listReceived, listSent, allFee, strSentAccount, filter);
            if (wtx.GetDepthInMainChain() >= nMinDepth) {
                for (const COutputEntry& r : listReceived)
                    nBalance += r.amount;
            }
            for (const COutputEntry& s : listSent)
                nBalance -= s.amount;
            nBalance -= allFee;

        } else {

            CAmount nReceived, nSent, nFee;
            wtx.GetAccountAmounts(strAccount, nReceived, nSent, nFee, filter);

            if (nReceived != 0 && depth >= nMinDepth)
                nBalance += nReceived;
            nBalance -= nSent + nFee;
        }
    }

    // Tally internal accounting entries
    if (strAccount != "*")
        nBalance += walletdb.GetAccountCreditDebit(strAccount);

    return nBalance;
}

CAmount GetAccountBalance(const std::string& strAccount, int nMinDepth, const isminefilter& filter)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);
    return GetAccountBalance(walletdb, strAccount, nMinDepth, filter);
}


UniValue getbalance(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 4)
        throw std::runtime_error(
            "getbalance ( \"account\" minconf includeWatchonly includeDelegated )\n"
            "\nIf account is not specified, returns the server's total available balance (excluding zerocoins).\n"
            "If account is specified (DEPRECATED), returns the balance in the account.\n"
            "Note that the account \"\" is not the same as leaving the parameter out.\n"
            "The server total may be different to the balance in the default \"\" account.\n"

            "\nArguments:\n"
            "1. \"account\"      (string, optional) DEPRECATED. The selected account, or \"*\" for entire wallet. It may be the default account using \"\".\n"
            "2. minconf          (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"
            "3. includeWatchonly (bool, optional, default=false) Also include balance in watchonly addresses (see 'importaddress')\n"
            "4. includeDelegated (bool, optional, default=true) Also include balance delegated to cold stakers\n"

            "\nResult:\n"
            "amount              (numeric) The total amount in BTCU received for this account.\n"

            "\nExamples:\n"
            "\nThe total amount in the wallet\n" +
            HelpExampleCli("getbalance", "") +
            "\nThe total amount in the wallet, with at least 5 confirmations\n" +
            HelpExampleCli("getbalance", "\"*\" 6") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("getbalance", "\"*\", 6"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (params.size() == 0)
        return ValueFromAmount(pwalletMain->GetBalance());

    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();
    isminefilter filter = ISMINE_SPENDABLE;
    if ( params.size() > 2 && params[2].get_bool() )
        filter = filter | ISMINE_WATCH_ONLY;
    if ( !(params.size() > 3) || params[3].get_bool() )
        filter = filter | ISMINE_SPENDABLE_DELEGATED;

    std::string strAccount = params[0].get_str();
    return ValueFromAmount(GetAccountBalance(strAccount, nMinDepth, filter));
}

UniValue getcoldstakingbalance(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "getcoldstakingbalance ( \"account\" )\n"
            "\nIf account is not specified, returns the server's total available cold balance.\n"
            "If account is specified (DEPRECATED), returns the cold balance in the account.\n"
            "Note that the account \"\" is not the same as leaving the parameter out.\n"
            "The server total may be different to the balance in the default \"\" account.\n"

            "\nArguments:\n"
            "1. \"account\"      (string, optional) DEPRECATED. The selected account, or \"*\" for entire wallet. It may be the default account using \"\".\n"

            "\nResult:\n"
            "amount              (numeric) The total amount in BTCU received for this account in P2CS contracts.\n"

            "\nExamples:\n"
            "\nThe total amount in the wallet\n" +
            HelpExampleCli("getcoldstakingbalance", "") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("getcoldstakingbalance", "\"*\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (params.size() == 0)
        return ValueFromAmount(pwalletMain->GetColdStakingBalance());

    std::string strAccount = params[0].get_str();
    return ValueFromAmount(GetAccountBalance(strAccount, /*nMinDepth*/ 1, ISMINE_COLD));
}

UniValue getdelegatedbalance(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "getdelegatedbalance ( \"account\" )\n"
            "\nIf account is not specified, returns the server's total available delegated balance (sum of all utxos delegated\n"
            "to a cold staking address to stake on behalf of addresses of this wallet).\n"
            "If account is specified (DEPRECATED), returns the cold balance in the account.\n"
            "Note that the account \"\" is not the same as leaving the parameter out.\n"
            "The server total may be different to the balance in the default \"\" account.\n"

            "\nArguments:\n"
            "1. \"account\"      (string, optional) DEPRECATED. The selected account, or \"*\" for entire wallet. It may be the default account using \"\".\n"

            "\nResult:\n"
            "amount              (numeric) The total amount in BTCU received for this account in P2CS contracts.\n"

            "\nExamples:\n"
            "\nThe total amount in the wallet\n" +
            HelpExampleCli("getdelegatedbalance", "") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("getdelegatedbalance", "\"*\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (params.size() == 0)
        return ValueFromAmount(pwalletMain->GetDelegatedBalance());

    std::string strAccount = params[0].get_str();
    return ValueFromAmount(GetAccountBalance(strAccount, /*nMinDepth*/ 1, ISMINE_SPENDABLE_DELEGATED));
}

UniValue getunconfirmedbalance(const UniValue &params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw std::runtime_error(
            "getunconfirmedbalance\n"
            "Returns the server's total unconfirmed balance\n");

    LOCK2(cs_main, pwalletMain->cs_wallet);

    return ValueFromAmount(pwalletMain->GetUnconfirmedBalance());
}


UniValue movecmd(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw std::runtime_error(
            "move \"fromaccount\" \"toaccount\" amount ( minconf \"comment\" )\n"
            "\nDEPRECATED. Move a specified amount from one account in your wallet to another.\n"

            "\nArguments:\n"
            "1. \"fromaccount\"   (string, required) The name of the account to move funds from. May be the default account using \"\".\n"
            "2. \"toaccount\"     (string, required) The name of the account to move funds to. May be the default account using \"\".\n"
            "3. amount            (numeric, required) Quantity of BTCU to move between accounts.\n"
            "4. minconf           (numeric, optional, default=1) Only use funds with at least this many confirmations.\n"
            "5. \"comment\"       (string, optional) An optional comment, stored in the wallet only.\n"

            "\nResult:\n"
            "true|false           (boolean) true if successful.\n"

            "\nExamples:\n"
            "\nMove 0.01 BTCU from the default account to the account named tabby\n" +
            HelpExampleCli("move", "\"\" \"tabby\" 0.01") +
            "\nMove 0.01 BTCU from timotei to akiko with a comment\n" +
            HelpExampleCli("move", "\"timotei\" \"akiko\" 0.01 1 \"happy birthday!\"") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("move", "\"timotei\", \"akiko\", 0.01, 1, \"happy birthday!\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    std::string strFrom = AccountFromValue(params[0]);
    std::string strTo = AccountFromValue(params[1]);
    CAmount nAmount = AmountFromValue(params[2]);
    if (params.size() > 3)
        // unused parameter, used to be nMinDepth, keep type-checking it though
        (void)params[3].get_int();
    std::string strComment;
    if (params.size() > 4)
        strComment = params[4].get_str();

    CWalletDB walletdb(pwalletMain->strWalletFile);
    if (!walletdb.TxnBegin())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

    int64_t nNow = GetAdjustedTime();

    // Debit
    CAccountingEntry debit;
    debit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    debit.strAccount = strFrom;
    debit.nCreditDebit = -nAmount;
    debit.nTime = nNow;
    debit.strOtherAccount = strTo;
    debit.strComment = strComment;
    pwalletMain->AddAccountingEntry(debit, walletdb);

    // Credit
    CAccountingEntry credit;
    credit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    credit.strAccount = strTo;
    credit.nCreditDebit = nAmount;
    credit.nTime = nNow;
    credit.strOtherAccount = strFrom;
    credit.strComment = strComment;
    pwalletMain->AddAccountingEntry(credit, walletdb);

    if (!walletdb.TxnCommit())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

    return true;
}


UniValue sendfrom(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 7)
        throw std::runtime_error(
            "sendfrom \"fromaccount\" \"tobtcuaddress\" amount ( minconf \"comment\" \"comment-to\" includeDelegated)\n"
            "\nDEPRECATED (use sendtoaddress). Send an amount from an account to a btcu address.\n"
            "The amount is a real and is rounded to the nearest 0.00000001." +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"fromaccount\"       (string, required) The name of the account to send funds from. May be the default account using \"\".\n"
            "2. \"tobtcuaddress\"  (string, required) The btcu address to send funds to.\n"
            "3. amount                (numeric, required) The amount in BTCU. (transaction fee is added on top).\n"
            "4. minconf               (numeric, optional, default=1) Only use funds with at least this many confirmations.\n"
            "5. \"comment\"           (string, optional) A comment used to store what the transaction is for. \n"
            "                                     This is not part of the transaction, just kept in your wallet.\n"
            "6. \"comment-to\"        (string, optional) An optional comment to store the name of the person or organization \n"
            "                                     to which you're sending the transaction. This is not part of the transaction, \n"
            "                                     it is just kept in your wallet.\n"
            "7. includeDelegated     (bool, optional, default=false) Also include balance delegated to cold stakers\n"

            "\nResult:\n"
            "\"transactionid\"        (string) The transaction id.\n"

            "\nExamples:\n"
            "\nSend 0.01 BTCU from the default account to the address, must have at least 1 confirmation\n" +
            HelpExampleCli("sendfrom", "\"\" \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" 0.01") +
            "\nSend 0.01 from the tabby account to the given address, funds must have at least 6 confirmations\n" +
            HelpExampleCli("sendfrom", "\"tabby\" \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" 0.01 6 \"donation\" \"seans outpost\"") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("sendfrom", "\"tabby\", \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\", 0.01, 6, \"donation\", \"seans outpost\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    std::string strAccount = AccountFromValue(params[0]);
    CBTCUAddress address(params[1].get_str());
    if (!address.IsValid() || address.IsStakingAddress())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU address");
    CAmount nAmount = AmountFromValue(params[2]);
    int nMinDepth = 1;
    if (params.size() > 3)
        nMinDepth = params[3].get_int();

    CWalletTx wtx;
    wtx.strFromAccount = strAccount;
    if (params.size() > 4 && !params[4].isNull() && !params[4].get_str().empty())
        wtx.mapValue["comment"] = params[4].get_str();
    if (params.size() > 5 && !params[5].isNull() && !params[5].get_str().empty())
        wtx.mapValue["to"] = params[5].get_str();

    isminefilter filter = ISMINE_SPENDABLE;
    if ( params.size() > 6 && params[6].get_bool() )
        filter = filter | ISMINE_SPENDABLE_DELEGATED;

    EnsureWalletIsUnlocked();

    // Check funds
    CAmount nBalance = GetAccountBalance(strAccount, nMinDepth, filter);
    if (nAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

    SendMoney(address.Get(), nAmount, wtx);

    return wtx.GetHash().GetHex();
}


UniValue sendmany(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 5)
        throw std::runtime_error(
            "sendmany \"fromaccount\" {\"address\":amount,...} ( minconf \"comment\" includeDelegated )\n"
            "\nSend multiple times. Amounts are double-precision floating point numbers." +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"fromaccount\"         (string, required) DEPRECATED. The account to send the funds from. Should be \"\" for the default account\n"
            "2. \"amounts\"             (string, required) A json object with addresses and amounts\n"
            "    {\n"
            "      \"address\":amount   (numeric) The btcu address is the key, the numeric amount in BTCU is the value\n"
            "      ,...\n"
            "    }\n"
            "3. minconf                 (numeric, optional, default=1) Only use the balance confirmed at least this many times.\n"
            "4. \"comment\"             (string, optional) A comment\n"
            "5. includeDelegated     (bool, optional, default=false) Also include balance delegated to cold stakers\n"

            "\nResult:\n"
            "\"transactionid\"          (string) The transaction id for the send. Only 1 transaction is created regardless of \n"
            "                                    the number of addresses.\n"

            "\nExamples:\n"
            "\nSend two amounts to two different addresses:\n" +
            HelpExampleCli("sendmany", "\"\" \"{\\\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\\\":0.01,\\\"DAD3Y6ivr8nPQLT1NEPX84DxGCw9jz9Jvg\\\":0.02}\"") +
            "\nSend two amounts to two different addresses setting the confirmation and comment:\n" +
            HelpExampleCli("sendmany", "\"\" \"{\\\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\\\":0.01,\\\"DAD3Y6ivr8nPQLT1NEPX84DxGCw9jz9Jvg\\\":0.02}\" 6 \"testing\"") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("sendmany", "\"\", \"{\\\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\\\":0.01,\\\"DAD3Y6ivr8nPQLT1NEPX84DxGCw9jz9Jvg\\\":0.02}\", 6, \"testing\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    std::string strAccount = AccountFromValue(params[0]);
    UniValue sendTo = params[1].get_obj();
    int nMinDepth = 1;
    if (params.size() > 2)
        nMinDepth = params[2].get_int();

    CWalletTx wtx;
    wtx.strFromAccount = strAccount;
    if (params.size() > 3 && !params[3].isNull() && !params[3].get_str().empty())
        wtx.mapValue["comment"] = params[3].get_str();

    std::set<CBTCUAddress> setAddress;
    std::vector<std::pair<CScript, CAmount> > vecSend;

    CAmount totalAmount = 0;
    std::vector<std::string> keys = sendTo.getKeys();
    for (const std::string& name_ : keys) {
        CBTCUAddress address(name_);
        if (!address.IsValid() || address.IsStakingAddress())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid BTCU address: ")+name_);

        if (setAddress.count(address))
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated address: ")+name_);
        setAddress.insert(address);

        CScript scriptPubKey = GetScriptForDestination(address.Get());
        CAmount nAmount = AmountFromValue(sendTo[name_]);
        totalAmount += nAmount;

        vecSend.push_back(std::make_pair(scriptPubKey, nAmount));
    }

    isminefilter filter = ISMINE_SPENDABLE;
    if ( params.size() > 5 && params[5].get_bool() )
        filter = filter | ISMINE_SPENDABLE_DELEGATED;

    EnsureWalletIsUnlocked();

    // Check funds
    CAmount nBalance = GetAccountBalance(strAccount, nMinDepth, ISMINE_SPENDABLE);
    if (totalAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

    // Send
    CReserveKey keyChange(pwalletMain);
    CAmount nFeeRequired = 0;
    std::string strFailReason;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, strFailReason);
    if (!fCreated)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strFailReason);
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

// Defined in rpc/misc.cpp
extern CScript _createmultisig_redeemScript(const UniValue& params);

UniValue addmultisigaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw std::runtime_error(
            "addmultisigaddress nrequired [\"key\",...] ( \"account\" )\n"
            "\nAdd a nrequired-to-sign multisignature address to the wallet.\n"
            "Each key is a BTCU address or hex-encoded public key.\n"
            "If 'account' is specified (DEPRECATED), assign address to that account.\n"

            "\nArguments:\n"
            "1. nrequired        (numeric, required) The number of required signatures out of the n keys or addresses.\n"
            "2. \"keysobject\"   (string, required) A json array of btcu addresses or hex-encoded public keys\n"
            "     [\n"
            "       \"address\"  (string) btcu address or hex-encoded public key\n"
            "       ...,\n"
            "     ]\n"
            "3. \"account\"      (string, optional) DEPRECATED. An account to assign the addresses to.\n"

            "\nResult:\n"
            "\"btcuaddress\"  (string) A btcu address associated with the keys.\n"

            "\nExamples:\n"
            "\nAdd a multisig address from 2 addresses\n" +
            HelpExampleCli("addmultisigaddress", "2 \"[\\\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\\\",\\\"DAD3Y6ivr8nPQLT1NEPX84DxGCw9jz9Jvg\\\"]\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("addmultisigaddress", "2, \"[\\\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\\\",\\\"DAD3Y6ivr8nPQLT1NEPX84DxGCw9jz9Jvg\\\"]\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    std::string strAccount;
    if (params.size() > 2)
        strAccount = AccountFromValue(params[2]);

    // Construct using pay-to-script-hash:
    CScript inner = _createmultisig_redeemScript(params);
    CScriptID innerID(inner);
    pwalletMain->AddCScript(inner);

    pwalletMain->SetAddressBook(innerID, strAccount, AddressBook::AddressBookPurpose::SEND);
    return CBTCUAddress(innerID).ToString();
}


struct tallyitem {
    CAmount nAmount;
    int nConf;
    int nBCConf;
    std::vector<uint256> txids;
    bool fIsWatchonly;
    tallyitem()
    {
        nAmount = 0;
        nConf = std::numeric_limits<int>::max();
        nBCConf = std::numeric_limits<int>::max();
        fIsWatchonly = false;
    }
};

UniValue ListReceived(const UniValue& params, bool fByAccounts)
{
    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    // Whether to include empty accounts
    bool fIncludeEmpty = false;
    if (params.size() > 1)
        fIncludeEmpty = params[1].get_bool();

    isminefilter filter = ISMINE_SPENDABLE_ALL;
    if (params.size() > 2)
        if (params[2].get_bool())
            filter = filter | ISMINE_WATCH_ONLY;

    // Tally
    std::map<CBTCUAddress, tallyitem> mapTally;
    for (std::map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it) {
        const CWalletTx& wtx = (*it).second;

        if (wtx.IsCoinBase() || !IsFinalTx(wtx))
            continue;

        int nDepth = wtx.GetDepthInMainChain();
        int nBCDepth = wtx.GetDepthInMainChain(false);
        if (nDepth < nMinDepth)
            continue;

        for (const CTxOut& txout : wtx.vout) {
            CTxDestination address;
            if (!ExtractDestination(txout.scriptPubKey, address))
                continue;

            isminefilter mine = IsMine(*pwalletMain, address);
            if (!(mine & filter))
                continue;

            tallyitem& item = mapTally[address];
            item.nAmount += txout.nValue;
            item.nConf = std::min(item.nConf, nDepth);
            item.nBCConf = std::min(item.nBCConf, nBCDepth);
            item.txids.push_back(wtx.GetHash());
            if (mine & ISMINE_WATCH_ONLY)
                item.fIsWatchonly = true;
        }
    }

    // Reply
    UniValue ret(UniValue::VARR);
    std::map<std::string, tallyitem> mapAccountTally;
    for (const PAIRTYPE(CBTCUAddress, AddressBook::CAddressBookData) & item : pwalletMain->mapAddressBook) {
        const CBTCUAddress& address = item.first;
        const std::string& strAccount = item.second.name;
        std::map<CBTCUAddress, tallyitem>::iterator it = mapTally.find(address);
        if (it == mapTally.end() && !fIncludeEmpty)
            continue;

        CAmount nAmount = 0;
        int nConf = std::numeric_limits<int>::max();
        int nBCConf = std::numeric_limits<int>::max();
        bool fIsWatchonly = false;
        if (it != mapTally.end()) {
            nAmount = (*it).second.nAmount;
            nConf = (*it).second.nConf;
            nBCConf = (*it).second.nBCConf;
            fIsWatchonly = (*it).second.fIsWatchonly;
        }

        if (fByAccounts) {
            tallyitem& item = mapAccountTally[strAccount];
            item.nAmount += nAmount;
            item.nConf = std::min(item.nConf, nConf);
            item.nBCConf = std::min(item.nBCConf, nBCConf);
            item.fIsWatchonly = fIsWatchonly;
        } else {
            UniValue obj(UniValue::VOBJ);
            if (fIsWatchonly)
                obj.push_back(Pair("involvesWatchonly", true));
            obj.push_back(Pair("address", address.ToString()));
            obj.push_back(Pair("account", strAccount));
            obj.push_back(Pair("amount", ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            obj.push_back(Pair("bcconfirmations", (nBCConf == std::numeric_limits<int>::max() ? 0 : nBCConf)));
            UniValue transactions(UniValue::VARR);
            if (it != mapTally.end()) {
                for (const uint256& item : (*it).second.txids) {
                    transactions.push_back(item.GetHex());
                }
            }
            obj.push_back(Pair("txids", transactions));
            ret.push_back(obj);
        }
    }

    if (fByAccounts) {
        for (std::map<std::string, tallyitem>::iterator it = mapAccountTally.begin(); it != mapAccountTally.end(); ++it) {
            CAmount nAmount = (*it).second.nAmount;
            int nConf = (*it).second.nConf;
            int nBCConf = (*it).second.nBCConf;
            UniValue obj(UniValue::VOBJ);
            if ((*it).second.fIsWatchonly)
                obj.push_back(Pair("involvesWatchonly", true));
            obj.push_back(Pair("account", (*it).first));
            obj.push_back(Pair("amount", ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            obj.push_back(Pair("bcconfirmations", (nBCConf == std::numeric_limits<int>::max() ? 0 : nBCConf)));
            ret.push_back(obj);
        }
    }

    return ret;
}

UniValue listreceivedbyaddress(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 3)
        throw std::runtime_error(
            "listreceivedbyaddress ( minconf includeempty includeWatchonly)\n"
            "\nList balances by receiving address.\n"

            "\nArguments:\n"
            "1. minconf       (numeric, optional, default=1) The minimum number of confirmations before payments are included.\n"
            "2. includeempty  (numeric, optional, default=false) Whether to include addresses that haven't received any payments.\n"
            "3. includeWatchonly (bool, optional, default=false) Whether to include watchonly addresses (see 'importaddress').\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"involvesWatchonly\" : \"true\",    (bool) Only returned if imported addresses were involved in transaction\n"
            "    \"address\" : \"receivingaddress\",  (string) The receiving address\n"
            "    \"account\" : \"accountname\",       (string) DEPRECATED. The account of the receiving address. The default account is \"\".\n"
            "    \"amount\" : x.xxx,                  (numeric) The total amount in BTCU received by the address\n"
            "    \"confirmations\" : n                (numeric) The number of confirmations of the most recent transaction included\n"
            "    \"bcconfirmations\" : n              (numeric) The number of blockchain confirmations of the most recent transaction included\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("listreceivedbyaddress", "") + HelpExampleCli("listreceivedbyaddress", "6 true") + HelpExampleRpc("listreceivedbyaddress", "6, true, true"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    return ListReceived(params, false);
}

UniValue listreceivedbyaccount(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 3)
        throw std::runtime_error(
            "listreceivedbyaccount ( minconf includeempty includeWatchonly)\n"
            "\nDEPRECATED. List balances by account.\n"

            "\nArguments:\n"
            "1. minconf      (numeric, optional, default=1) The minimum number of confirmations before payments are included.\n"
            "2. includeempty (boolean, optional, default=false) Whether to include accounts that haven't received any payments.\n"
            "3. includeWatchonly (bool, optional, default=false) Whether to include watchonly addresses (see 'importaddress').\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"involvesWatchonly\" : \"true\",    (bool) Only returned if imported addresses were involved in transaction\n"
            "    \"account\" : \"accountname\",  (string) The account name of the receiving account\n"
            "    \"amount\" : x.xxx,             (numeric) The total amount received by addresses with this account\n"
            "    \"confirmations\" : n           (numeric) The number of confirmations of the most recent transaction included\n"
            "    \"bcconfirmations\" : n         (numeric) The number of blockchain confirmations of the most recent transaction included\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("listreceivedbyaccount", "") + HelpExampleCli("listreceivedbyaccount", "6 true") + HelpExampleRpc("listreceivedbyaccount", "6, true, true"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    return ListReceived(params, true);
}

UniValue listcoldutxos(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "listcoldutxos ( nonWhitelistedOnly )\n"
            "\nList P2CS unspent outputs received by this wallet as cold-staker\n"

            "\nArguments:\n"
            "1. nonWhitelistedOnly   (boolean, optional, default=false) Whether to exclude P2CS from whitelisted delegators.\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txid\" : \"true\",            (string) The transaction id of the P2CS utxo\n"
            "    \"txidn\" : \"accountname\",    (string) The output number of the P2CS utxo\n"
            "    \"amount\" : x.xxx,             (numeric) The amount of the P2CS utxo\n"
            "    \"confirmations\" : n           (numeric) The number of confirmations of the P2CS utxo\n"
            "    \"cold-staker\" : n             (string) The cold-staker address of the P2CS utxo\n"
            "    \"coin-owner\" : n              (string) The coin-owner address of the P2CS utxo\n"
            "    \"whitelisted\" : n             (string) \"true\"/\"false\" coin-owner in delegator whitelist\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("listcoldutxos", "") + HelpExampleCli("listcoldutxos", "true"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    bool fExcludeWhitelisted = false;
    if (params.size() > 0)
        fExcludeWhitelisted = params[0].get_bool();
    UniValue results(UniValue::VARR);

    for (std::map<uint256, CWalletTx>::const_iterator it =
            pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it) {
        const uint256& wtxid = it->first;
        const CWalletTx* pcoin = &(*it).second;
        if (!CheckFinalTx(*pcoin) || !pcoin->IsTrusted())
            continue;

        // if this tx has no unspent P2CS outputs for us, skip it
        if(pcoin->GetColdStakingCredit() == 0 && pcoin->GetStakeDelegationCredit() == 0)
            continue;

        for (unsigned int i = 0; i < pcoin->vout.size(); i++) {
            const CTxOut& out = pcoin->vout[i];
            isminetype mine = pwalletMain->IsMine(out);
            if (!bool(mine & ISMINE_COLD) && !bool(mine & ISMINE_SPENDABLE_DELEGATED))
                continue;
            txnouttype type;
            std::vector<CTxDestination> addresses;
            int nRequired;
            if (!ExtractDestinations(out.scriptPubKey, type, addresses, nRequired))
                continue;
            const bool fWhitelisted = pwalletMain->mapAddressBook.count(addresses[1]) > 0;
            if (fExcludeWhitelisted && fWhitelisted)
                continue;
            UniValue entry(UniValue::VOBJ);
            entry.push_back(Pair("txid", wtxid.GetHex()));
            entry.push_back(Pair("txidn", (int)i));
            entry.push_back(Pair("amount", ValueFromAmount(out.nValue)));
            entry.push_back(Pair("confirmations", pcoin->GetDepthInMainChain(false)));
            entry.push_back(Pair("cold-staker", CBTCUAddress(addresses[0], CChainParams::STAKING_ADDRESS).ToString()));
            entry.push_back(Pair("coin-owner", CBTCUAddress(addresses[1]).ToString()));
            entry.push_back(Pair("whitelisted", fWhitelisted ? "true" : "false"));
            results.push_back(entry);
        }
    }

    return results;
}

UniValue listleasingutxos(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "listleasingutxos ( nonWhitelistedOnly )\n"
            "\nList P2L unspent outputs received by this wallet as leaser\n"

            "\nArguments:\n"
            "1. nonWhitelistedOnly   (boolean, optional, default=false) Whether to exclude P2L from whitelisted leasees.\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txid\" : \"true\",            (string) The transaction id of the P2L utxo\n"
            "    \"txidn\" : \"accountname\",    (string) The output number of the P2L utxo\n"
            "    \"amount\" : x.xxx,             (numeric) The amount of the P2L utxo\n"
            "    \"confirmations\" : n,          (numeric) The number of confirmations of the P2L utxo\n"
            "    \"coin-leaser\" : n,            (string) The cold-leaser address of the P2L utxo\n"
            "    \"coin-owner\" : n,             (string) The coin-owner address of the P2L utxo\n"
            "    \"whitelisted\" : n             (string) \"true\"/\"false\" coin-owner in leasee whitelist\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("listleasingutxos", "") + HelpExampleCli("listleasingutxos", "true"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    bool fExcludeWhitelisted = false;
    if (params.size() > 0)
        fExcludeWhitelisted = params[0].get_bool();
    UniValue results(UniValue::VARR);

    for (std::map<uint256, CWalletTx>::const_iterator it =
        pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it) {
        const uint256& wtxid = it->first;
        const CWalletTx* pcoin = &(*it).second;
        if (!CheckFinalTx(*pcoin) || !pcoin->IsTrusted())
            continue;

        // if this tx has no unspent P2L outputs for us, skip it
        if(pcoin->GetLeasingCredit() == 0 && pcoin->GetLeasedCredit() == 0)
            continue;

        for (unsigned int i = 0; i < pcoin->vout.size(); i++) {
            const CTxOut& out = pcoin->vout[i];
            isminetype mine = pwalletMain->IsMine(out);
            if (!bool(mine & ISMINE_LEASING) && !bool(mine & ISMINE_LEASED))
                continue;
            txnouttype type;
            std::vector<CTxDestination> addresses;
            int nRequired;
            if (!ExtractDestinations(out.scriptPubKey, type, addresses, nRequired))
                continue;
            const bool fWhitelisted = pwalletMain->mapAddressBook.count(addresses[1]) > 0;
            if (fExcludeWhitelisted && fWhitelisted)
                continue;
            UniValue entry(UniValue::VOBJ);
            entry.push_back(Pair("txid", wtxid.GetHex()));
            entry.push_back(Pair("txidn", (int)i));
            entry.push_back(Pair("amount", ValueFromAmount(out.nValue)));
            entry.push_back(Pair("confirmations", pcoin->GetDepthInMainChain(false)));
            entry.push_back(Pair("coin-leaser", CBTCUAddress(addresses[0], CChainParams::PUBKEY_ADDRESS).ToString()));
            entry.push_back(Pair("coin-owner", CBTCUAddress(addresses[1]).ToString()));
            entry.push_back(Pair("whitelisted", fWhitelisted ? "true" : "false"));
            results.push_back(entry);
        }
    }

    return results;
}

static void MaybePushAddress(UniValue & entry, const CTxDestination &dest)
{
    CBTCUAddress addr;
    if (addr.Set(dest))
        entry.push_back(Pair("address", addr.ToString()));
}

void ListTransactions(const CWalletTx& wtx, const std::string& strAccount, int nMinDepth, bool fLong, UniValue& ret, const isminefilter& filter)
{
    CAmount nFee;
    std::string strSentAccount;
    std::list<COutputEntry> listReceived;
    std::list<COutputEntry> listSent;

    wtx.GetAmounts(listReceived, listSent, nFee, strSentAccount, filter);

    bool fAllAccounts = (strAccount == std::string("*"));
    bool involvesWatchonly = wtx.IsFromMe(ISMINE_WATCH_ONLY);

    // Sent
    if ((!listSent.empty() || nFee != 0) && (fAllAccounts || strAccount == strSentAccount)) {
        for (const COutputEntry& s : listSent) {
            UniValue entry(UniValue::VOBJ);
            if (involvesWatchonly || (::IsMine(*pwalletMain, s.destination) & ISMINE_WATCH_ONLY))
                entry.push_back(Pair("involvesWatchonly", true));
            entry.push_back(Pair("account", strSentAccount));
            MaybePushAddress(entry, s.destination);
            std::map<std::string, std::string>::const_iterator it = wtx.mapValue.find("DS");
            entry.push_back(Pair("category", (it != wtx.mapValue.end() && it->second == "1") ? "darksent" : "send"));
            entry.push_back(Pair("amount", ValueFromAmount(-s.amount)));
            entry.push_back(Pair("vout", s.vout));
            entry.push_back(Pair("fee", ValueFromAmount(-nFee)));
            if (fLong)
                WalletTxToJSON(wtx, entry);
            ret.push_back(entry);
        }
    }

    // Received
    int depth = wtx.GetDepthInMainChain();
    if (listReceived.size() > 0 && depth >= nMinDepth) {
        for (const COutputEntry& r : listReceived) {
            std::string account;
            if (pwalletMain->mapAddressBook.count(r.destination))
                account = pwalletMain->mapAddressBook[r.destination].name;
            if (fAllAccounts || (account == strAccount)) {
                UniValue entry(UniValue::VOBJ);
                if (involvesWatchonly || (::IsMine(*pwalletMain, r.destination) & ISMINE_WATCH_ONLY))
                    entry.push_back(Pair("involvesWatchonly", true));
                entry.push_back(Pair("account", account));
                MaybePushAddress(entry, r.destination);
                if (wtx.IsCoinBase()) {
                    if (depth < 1)
                        entry.push_back(Pair("category", "orphan"));
                    else if (wtx.GetBlocksToMaturity() > 0)
                        entry.push_back(Pair("category", "immature"));
                    else
                        entry.push_back(Pair("category", "generate"));
                } else {
                    entry.push_back(Pair("category", "receive"));
                }
                entry.push_back(Pair("amount", ValueFromAmount(r.amount)));
                entry.push_back(Pair("vout", r.vout));
                if (fLong)
                    WalletTxToJSON(wtx, entry);
                ret.push_back(entry);
            }
        }
    }
}

void AcentryToJSON(const CAccountingEntry& acentry, const std::string& strAccount, UniValue& ret)
{
    bool fAllAccounts = (strAccount == std::string("*"));

    if (fAllAccounts || acentry.strAccount == strAccount) {
        UniValue entry(UniValue::VOBJ);
        entry.push_back(Pair("account", acentry.strAccount));
        entry.push_back(Pair("category", "move"));
        entry.push_back(Pair("time", acentry.nTime));
        entry.push_back(Pair("amount", ValueFromAmount(acentry.nCreditDebit)));
        entry.push_back(Pair("otheraccount", acentry.strOtherAccount));
        entry.push_back(Pair("comment", acentry.strComment));
        ret.push_back(entry);
    }
}

UniValue listtransactions(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 6)
        throw std::runtime_error(
            "listtransactions ( \"account\" count from includeWatchonly includeDelegated )\n"
            "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"

            "\nArguments:\n"
            "1. \"account\"    (string, optional) DEPRECATED. The account name. Should be \"*\".\n"
            "2. count          (numeric, optional, default=10) The number of transactions to return\n"
            "3. from           (numeric, optional, default=0) The number of transactions to skip\n"
            "4. includeWatchonly (bool, optional, default=false) Include transactions to watchonly addresses (see 'importaddress')\n"
            "5. includeDelegated     (bool, optional, default=true) Also include balance delegated to cold stakers\n"
            "6. includeCold     (bool, optional, default=true) Also include delegated balance received as cold-staker by this node\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"account\":\"accountname\",       (string) DEPRECATED. The account name associated with the transaction. \n"
            "                                                It will be \"\" for the default account.\n"
            "    \"address\":\"btcuaddress\",    (string) The btcu address of the transaction. Not present for \n"
            "                                                move transactions (category = move).\n"
            "    \"category\":\"send|receive|move\", (string) The transaction category. 'move' is a local (off blockchain)\n"
            "                                                transaction between accounts, and not associated with an address,\n"
            "                                                transaction id or block. 'send' and 'receive' transactions are \n"
            "                                                associated with an address, transaction id and block details\n"
            "    \"amount\": x.xxx,          (numeric) The amount in BTCU. This is negative for the 'send' category, and for the\n"
            "                                         'move' category for moves outbound. It is positive for the 'receive' category,\n"
            "                                         and for the 'move' category for inbound funds.\n"
            "    \"vout\" : n,               (numeric) the vout value\n"
            "    \"fee\": x.xxx,             (numeric) The amount of the fee in BTCU. This is negative and only available for the \n"
            "                                         'send' category of transactions.\n"
            "    \"confirmations\": n,       (numeric) The number of confirmations for the transaction. Available for 'send' and \n"
            "                                         'receive' category of transactions.\n"
            "    \"bcconfirmations\": n,     (numeric) The number of blockchain confirmations for the transaction. Available for 'send'\n"
            "                                         'receive' category of transactions. Negative confirmations indicate the\n"
            "                                         transation conflicts with the block chain\n"
            "    \"trusted\": xxx            (bool) Whether we consider the outputs of this unconfirmed transaction safe to spend.\n"
            "                                          and 'receive' category of transactions.\n"
            "    \"blockhash\": \"hashvalue\", (string) The block hash containing the transaction. Available for 'send' and 'receive'\n"
            "                                          category of transactions.\n"
            "    \"blockindex\": n,          (numeric) The block index containing the transaction. Available for 'send' and 'receive'\n"
            "                                          category of transactions.\n"
            "    \"txid\": \"transactionid\", (string) The transaction id. Available for 'send' and 'receive' category of transactions.\n"
            "    \"time\": xxx,              (numeric) The transaction time in seconds since epoch (midnight Jan 1 1970 GMT).\n"
            "    \"timereceived\": xxx,      (numeric) The time received in seconds since epoch (midnight Jan 1 1970 GMT). Available \n"
            "                                          for 'send' and 'receive' category of transactions.\n"
            "    \"comment\": \"...\",       (string) If a comment is associated with the transaction.\n"
            "    \"otheraccount\": \"accountname\",  (string) For the 'move' category of transactions, the account the funds came \n"
            "                                          from (for receiving funds, positive amounts), or went to (for sending funds,\n"
            "                                          negative amounts).\n"
            "  }\n"
            "]\n"

            "\nExamples:\n"
            "\nList the most recent 10 transactions in the systems\n" +
            HelpExampleCli("listtransactions", "") +
            "\nList transactions 100 to 120\n" +
            HelpExampleCli("listtransactions", "\"*\" 20 100") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("listtransactions", "\"*\", 20, 100"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    std::string strAccount = "*";
    if (params.size() > 0)
        strAccount = params[0].get_str();
    int nCount = 10;
    if (params.size() > 1)
        nCount = params[1].get_int();
    int nFrom = 0;
    if (params.size() > 2)
        nFrom = params[2].get_int();
    isminefilter filter = ISMINE_SPENDABLE;
    if ( params.size() > 3 && params[3].get_bool() )
            filter = filter | ISMINE_WATCH_ONLY;
    if ( !(params.size() > 4) || params[4].get_bool() )
        filter = filter | ISMINE_SPENDABLE_DELEGATED;
    if ( !(params.size() > 5) || params[5].get_bool() )
        filter = filter | ISMINE_COLD;

    if (nCount < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    if (nFrom < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative from");

    UniValue ret(UniValue::VARR);

    const CWallet::TxItems & txOrdered = pwalletMain->wtxOrdered;

    // iterate backwards until we have nCount items to return:
    for (CWallet::TxItems::const_reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it) {
        CWalletTx* const pwtx = (*it).second.first;
        if (pwtx != 0)
            ListTransactions(*pwtx, strAccount, 0, true, ret, filter);
        CAccountingEntry* const pacentry = (*it).second.second;
        if (pacentry != 0)
            AcentryToJSON(*pacentry, strAccount, ret);

        if ((int)ret.size() >= (nCount + nFrom)) break;
    }
    // ret is newest to oldest

    if (nFrom > (int)ret.size())
        nFrom = ret.size();
    if ((nFrom + nCount) > (int)ret.size())
        nCount = ret.size() - nFrom;

    std::vector<UniValue> arrTmp = ret.getValues();

    std::vector<UniValue>::iterator first = arrTmp.begin();
    std::advance(first, nFrom);
    std::vector<UniValue>::iterator last = arrTmp.begin();
    std::advance(last, nFrom+nCount);

    if (last != arrTmp.end()) arrTmp.erase(last, arrTmp.end());
    if (first != arrTmp.begin()) arrTmp.erase(arrTmp.begin(), first);

    std::reverse(arrTmp.begin(), arrTmp.end()); // Return oldest to newest

    ret.clear();
    ret.setArray();
    ret.push_backV(arrTmp);

    return ret;
}

UniValue listaccounts(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw std::runtime_error(
            "listaccounts ( minconf includeWatchonly)\n"
            "\nDEPRECATED. Returns Object that has account names as keys, account balances as values.\n"

            "\nArguments:\n"
            "1. minconf          (numeric, optional, default=1) Only include transactions with at least this many confirmations\n"
            "2. includeWatchonly (bool, optional, default=false) Include balances in watchonly addresses (see 'importaddress')\n"

            "\nResult:\n"
            "{                      (json object where keys are account names, and values are numeric balances\n"
            "  \"account\": x.xxx,  (numeric) The property name is the account name, and the value is the total balance for the account.\n"
            "  ...\n"
            "}\n"

            "\nExamples:\n"
            "\nList account balances where there at least 1 confirmation\n" +
            HelpExampleCli("listaccounts", "") +
            "\nList account balances including zero confirmation transactions\n" +
            HelpExampleCli("listaccounts", "0") +
            "\nList account balances for 6 or more confirmations\n" +
            HelpExampleCli("listaccounts", "6") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("listaccounts", "6"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();
    isminefilter includeWatchonly = ISMINE_SPENDABLE;
    if (params.size() > 1)
        if (params[1].get_bool())
            includeWatchonly = includeWatchonly | ISMINE_WATCH_ONLY;

    std::map<std::string, CAmount> mapAccountBalances;
    for (const PAIRTYPE(CTxDestination, AddressBook::CAddressBookData) & entry : pwalletMain->mapAddressBook) {
        if (IsMine(*pwalletMain, entry.first) & includeWatchonly) // This address belongs to me
            mapAccountBalances[entry.second.name] = 0;
    }

    for (std::map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it) {
        const CWalletTx& wtx = (*it).second;
        CAmount nFee;
        std::string strSentAccount;
        std::list<COutputEntry> listReceived;
        std::list<COutputEntry> listSent;
        bool fConflicted;
        int nDepth = wtx.GetDepthAndMempool(fConflicted);
        if (wtx.GetBlocksToMaturity() > 0 || nDepth < 0 || fConflicted)
            continue;
        wtx.GetAmounts(listReceived, listSent, nFee, strSentAccount, includeWatchonly);
        mapAccountBalances[strSentAccount] -= nFee;
        for (const COutputEntry& s : listSent)
            mapAccountBalances[strSentAccount] -= s.amount;
        if (nDepth >= nMinDepth) {
            for (const COutputEntry& r : listReceived)
                if (pwalletMain->mapAddressBook.count(r.destination))
                    mapAccountBalances[pwalletMain->mapAddressBook[r.destination].name] += r.amount;
                else
                    mapAccountBalances[""] += r.amount;
        }
    }

    const std::list<CAccountingEntry> & acentries = pwalletMain->laccentries;
    for (const CAccountingEntry& entry : acentries)
        mapAccountBalances[entry.strAccount] += entry.nCreditDebit;

    UniValue ret(UniValue::VOBJ);
    for (const PAIRTYPE(std::string, CAmount) & accountBalance : mapAccountBalances) {
        ret.push_back(Pair(accountBalance.first, ValueFromAmount(accountBalance.second)));
    }
    return ret;
}

UniValue listsinceblock(const UniValue& params, bool fHelp)
{
    if (fHelp)
        throw std::runtime_error(
            "listsinceblock ( \"blockhash\" target-confirmations includeWatchonly)\n"
            "\nGet all transactions in blocks since block [blockhash], or all transactions if omitted\n"

            "\nArguments:\n"
            "1. \"blockhash\"   (string, optional) The block hash to list transactions since\n"
            "2. target-confirmations:    (numeric, optional) The confirmations required, must be 1 or more\n"
            "3. includeWatchonly:        (bool, optional, default=false) Include transactions to watchonly addresses (see 'importaddress')"

            "\nResult:\n"
            "{\n"
            "  \"transactions\": [\n"
            "    \"account\":\"accountname\",       (string) DEPRECATED. The account name associated with the transaction. Will be \"\" for the default account.\n"
            "    \"address\":\"btcuaddress\",    (string) The btcu address of the transaction. Not present for move transactions (category = move).\n"
            "    \"category\":\"send|receive\",     (string) The transaction category. 'send' has negative amounts, 'receive' has positive amounts.\n"
            "    \"amount\": x.xxx,          (numeric) The amount in BTCU. This is negative for the 'send' category, and for the 'move' category for moves \n"
            "                                          outbound. It is positive for the 'receive' category, and for the 'move' category for inbound funds.\n"
            "    \"vout\" : n,               (numeric) the vout value\n"
            "    \"fee\": x.xxx,             (numeric) The amount of the fee in BTCU. This is negative and only available for the 'send' category of transactions.\n"
            "    \"confirmations\": n,       (numeric) The number of confirmations for the transaction. Available for 'send' and 'receive' category of transactions.\n"
            "    \"bcconfirmations\" : n,    (numeric) The number of blockchain confirmations for the transaction. Available for 'send' and 'receive' category of transactions.\n"
            "    \"blockhash\": \"hashvalue\",     (string) The block hash containing the transaction. Available for 'send' and 'receive' category of transactions.\n"
            "    \"blockindex\": n,          (numeric) The block index containing the transaction. Available for 'send' and 'receive' category of transactions.\n"
            "    \"blocktime\": xxx,         (numeric) The block time in seconds since epoch (1 Jan 1970 GMT).\n"
            "    \"txid\": \"transactionid\",  (string) The transaction id. Available for 'send' and 'receive' category of transactions.\n"
            "    \"time\": xxx,              (numeric) The transaction time in seconds since epoch (Jan 1 1970 GMT).\n"
            "    \"timereceived\": xxx,      (numeric) The time received in seconds since epoch (Jan 1 1970 GMT). Available for 'send' and 'receive' category of transactions.\n"
            "    \"comment\": \"...\",       (string) If a comment is associated with the transaction.\n"
            "    \"to\": \"...\",            (string) If a comment to is associated with the transaction.\n"
            "  ],\n"
            "  \"lastblock\": \"lastblockhash\"     (string) The hash of the last block\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("listsinceblock", "") +
            HelpExampleCli("listsinceblock", "\"000000000000000bacf66f7497b7dc45ef753ee9a7d38571037cdb1a57f663ad\" 6") +
            HelpExampleRpc("listsinceblock", "\"000000000000000bacf66f7497b7dc45ef753ee9a7d38571037cdb1a57f663ad\", 6"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CBlockIndex* pindex = NULL;
    int target_confirms = 1;
    isminefilter filter = ISMINE_SPENDABLE_ALL | ISMINE_COLD | ISMINE_LEASING;

    if (params.size() > 0) {
        uint256 blockId = 0;

        blockId.SetHex(params[0].get_str());
        BlockMap::iterator it = mapBlockIndex.find(blockId);
        if (it != mapBlockIndex.end())
            pindex = it->second;
    }

    if (params.size() > 1) {
        target_confirms = params[1].get_int();

        if (target_confirms < 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter");
    }

    if (params.size() > 2)
        if (params[2].get_bool())
            filter = filter | ISMINE_WATCH_ONLY;

    int depth = pindex ? (1 + chainActive.Height() - pindex->nHeight) : -1;

    UniValue transactions(UniValue::VARR);

    for (std::map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); it++) {
        CWalletTx tx = (*it).second;

        if (depth == -1 || tx.GetDepthInMainChain(false) < depth)
            ListTransactions(tx, "*", 0, true, transactions, filter);
    }

    CBlockIndex* pblockLast = chainActive[chainActive.Height() + 1 - target_confirms];
    uint256 lastblock = pblockLast ? pblockLast->GetBlockHash() : 0;

    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("transactions", transactions));
    ret.push_back(Pair("lastblock", lastblock.GetHex()));

    return ret;
}

UniValue gettransaction(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "gettransaction \"txid\" ( includeWatchonly )\n"
            "\nGet detailed information about in-wallet transaction <txid>\n"

            "\nArguments:\n"
            "1. \"txid\"    (string, required) The transaction id\n"
            "2. \"includeWatchonly\"    (bool, optional, default=false) Whether to include watchonly addresses in balance calculation and details[]\n"

            "\nResult:\n"
            "{\n"
            "  \"amount\" : x.xxx,        (numeric) The transaction amount in BTCU\n"
            "  \"confirmations\" : n,     (numeric) The number of confirmations\n"
            "  \"bcconfirmations\" : n,   (numeric) The number of blockchain confirmations\n"
            "  \"blockhash\" : \"hash\",  (string) The block hash\n"
            "  \"blockindex\" : xx,       (numeric) The block index\n"
            "  \"blocktime\" : ttt,       (numeric) The time in seconds since epoch (1 Jan 1970 GMT)\n"
            "  \"txid\" : \"transactionid\",   (string) The transaction id.\n"
            "  \"time\" : ttt,            (numeric) The transaction time in seconds since epoch (1 Jan 1970 GMT)\n"
            "  \"timereceived\" : ttt,    (numeric) The time received in seconds since epoch (1 Jan 1970 GMT)\n"
            "  \"details\" : [\n"
            "    {\n"
            "      \"account\" : \"accountname\",  (string) DEPRECATED. The account name involved in the transaction, can be \"\" for the default account.\n"
            "      \"address\" : \"btcuaddress\",   (string) The btcu address involved in the transaction\n"
            "      \"category\" : \"send|receive\",    (string) The category, either 'send' or 'receive'\n"
            "      \"amount\" : x.xxx                  (numeric) The amount in BTCU\n"
            "      \"vout\" : n,                       (numeric) the vout value\n"
            "    }\n"
            "    ,...\n"
            "  ],\n"
            "  \"hex\" : \"data\"         (string) Raw data for transaction\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("gettransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"") +
            HelpExampleCli("gettransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\" true") +
            HelpExampleRpc("gettransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    uint256 hash;
    hash.SetHex(params[0].get_str());

    isminefilter filter = ISMINE_SPENDABLE_ALL | ISMINE_COLD | ISMINE_LEASING;
    if (params.size() > 1)
        if (params[1].get_bool())
            filter = filter | ISMINE_WATCH_ONLY;

    UniValue entry(UniValue::VOBJ);
    if (!pwalletMain->mapWallet.count(hash))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid or non-wallet transaction id");
    const CWalletTx& wtx = pwalletMain->mapWallet[hash];

    CAmount nCredit = wtx.GetCredit(filter);
    CAmount nDebit = wtx.GetDebit(filter);
    CAmount nNet = nCredit - nDebit;
    CAmount nFee = (wtx.IsFromMe(filter) ? wtx.GetValueOut() - nDebit : 0);

    entry.push_back(Pair("amount", ValueFromAmount(nNet - nFee)));
    if (wtx.IsFromMe(filter))
        entry.push_back(Pair("fee", ValueFromAmount(nFee)));

    WalletTxToJSON(wtx, entry);

    UniValue details(UniValue::VARR);
    ListTransactions(wtx, "*", 0, false, details, filter);
    entry.push_back(Pair("details", details));

    std::string strHex = EncodeHexTx(static_cast<CTransaction>(wtx));
    entry.push_back(Pair("hex", strHex));

    return entry;
}

UniValue abandontransaction(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
            "abandontransaction \"txid\"\n"
            "\nMark in-wallet transaction <txid> as abandoned\n"
            "This will mark this transaction and all its in-wallet descendants as abandoned which will allow\n"
            "for their inputs to be respent.  It can be used to replace \"stuck\" or evicted transactions.\n"
            "It only works on transactions which are not included in a block and are not currently in the mempool.\n"
            "It has no effect on transactions which are already conflicted or abandoned.\n"
            "\nArguments:\n"
            "1. \"txid\"    (string, required) The transaction id\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("abandontransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
            + HelpExampleRpc("abandontransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        );

    EnsureWalletIsUnlocked();

    LOCK2(cs_main, pwalletMain->cs_wallet);

    uint256 hash;
    hash.SetHex(params[0].get_str());

    if (!pwalletMain->mapWallet.count(hash))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid or non-wallet transaction id");
    if (!pwalletMain->AbandonTransaction(hash))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not eligible for abandonment");

    return NullUniValue;
}


UniValue backupwallet(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
            "backupwallet \"destination\"\n"
            "\nSafely copies wallet.dat to destination, which can be a directory or a path with filename.\n"

            "\nArguments:\n"
            "1. \"destination\"   (string) The destination directory or file\n"

            "\nExamples:\n" +
            HelpExampleCli("backupwallet", "\"backup.dat\"") + HelpExampleRpc("backupwallet", "\"backup.dat\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    std::string strDest = params[0].get_str();
    if (!BackupWallet(*pwalletMain, strDest))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Wallet backup failed!");

    return NullUniValue;
}


UniValue keypoolrefill(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "keypoolrefill ( newsize )\n"
            "\nFills the keypool." +
            HelpRequiringPassphrase() + "\n"

            "\nArguments\n"
            "1. newsize     (numeric, optional, default=100) The new keypool size\n"

            "\nExamples:\n" +
            HelpExampleCli("keypoolrefill", "") + HelpExampleRpc("keypoolrefill", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    // 0 is interpreted by TopUpKeyPool() as the default keypool size given by -keypool
    unsigned int kpSize = 0;
    if (params.size() > 0) {
        if (params[0].get_int() < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected valid size.");
        kpSize = (unsigned int)params[0].get_int();
    }

    EnsureWalletIsUnlocked();
    pwalletMain->TopUpKeyPool(kpSize);

    if (pwalletMain->GetKeyPoolSize() < kpSize)
        throw JSONRPCError(RPC_WALLET_ERROR, "Error refreshing keypool.");

    return NullUniValue;
}


static void LockWallet(CWallet* pWallet)
{
    LOCK(cs_nWalletUnlockTime);
    nWalletUnlockTime = 0;
    pWallet->fWalletUnlockAnonymizeOnly = false;
    pWallet->Lock();
}

UniValue walletpassphrase(const UniValue& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() < 2 || params.size() > 3))
        throw std::runtime_error(
            "walletpassphrase \"passphrase\" timeout ( anonymizeonly )\n"
            "\nStores the wallet decryption key in memory for 'timeout' seconds.\n"
            "This is needed prior to performing transactions related to private keys such as sending BTCUs\n"

            "\nArguments:\n"
            "1. \"passphrase\"     (string, required) The wallet passphrase\n"
            "2. timeout            (numeric, required) The time to keep the decryption key in seconds.\n"
            "3. anonymizeonly      (boolean, optional, default=false) If is true sending functions are disabled."

            "\nNote:\n"
            "Issuing the walletpassphrase command while the wallet is already unlocked will set a new unlock\n"
            "time that overrides the old one. A timeout of \"0\" unlocks until the wallet is closed.\n"

            "\nExamples:\n"
            "\nUnlock the wallet for 60 seconds\n" +
            HelpExampleCli("walletpassphrase", "\"my pass phrase\" 60") +
            "\nUnlock the wallet for 60 seconds but allow staking only\n" +
            HelpExampleCli("walletpassphrase", "\"my pass phrase\" 60 true") +
            "\nLock the wallet again (before 60 seconds)\n" +
            HelpExampleCli("walletlock", "") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("walletpassphrase", "\"my pass phrase\", 60"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrase was called.");

    // Note that the walletpassphrase is stored in params[0] which is not mlock()ed
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    strWalletPass = params[0].get_str().c_str();

    bool anonymizeOnly = false;
    if (params.size() == 3)
        anonymizeOnly = params[2].get_bool();

    if (!pwalletMain->IsLocked() && pwalletMain->fWalletUnlockAnonymizeOnly && anonymizeOnly)
        throw JSONRPCError(RPC_WALLET_ALREADY_UNLOCKED, "Error: Wallet is already unlocked.");

    // Get the timeout
    int64_t nSleepTime = params[1].get_int64();
    // Timeout cannot be negative, otherwise it will relock immediately
    if (nSleepTime < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Timeout cannot be negative.");
    }
    // Clamp timeout
    constexpr int64_t MAX_SLEEP_TIME = 100000000; // larger values trigger a macos/libevent bug?
    if (nSleepTime > MAX_SLEEP_TIME) {
        nSleepTime = MAX_SLEEP_TIME;
    }

    if (!pwalletMain->Unlock(strWalletPass, anonymizeOnly))
        throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");

    pwalletMain->TopUpKeyPool();

    if (nSleepTime > 0) {
        nWalletUnlockTime = GetTime () + nSleepTime;
        RPCRunLater ("lockwallet", boost::bind (LockWallet, pwalletMain), nSleepTime);
    }

    return NullUniValue;
}


UniValue walletpassphrasechange(const UniValue& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() != 2))
        throw std::runtime_error(
            "walletpassphrasechange \"oldpassphrase\" \"newpassphrase\"\n"
            "\nChanges the wallet passphrase from 'oldpassphrase' to 'newpassphrase'.\n"

            "\nArguments:\n"
            "1. \"oldpassphrase\"      (string) The current passphrase\n"
            "2. \"newpassphrase\"      (string) The new passphrase\n"

            "\nExamples:\n" +
            HelpExampleCli("walletpassphrasechange", "\"old one\" \"new one\"") + HelpExampleRpc("walletpassphrasechange", "\"old one\", \"new one\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");

    // TODO: get rid of these .c_str() calls by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strOldWalletPass;
    strOldWalletPass.reserve(100);
    strOldWalletPass = params[0].get_str().c_str();

    SecureString strNewWalletPass;
    strNewWalletPass.reserve(100);
    strNewWalletPass = params[1].get_str().c_str();

    if (strOldWalletPass.length() < 1 || strNewWalletPass.length() < 1)
        throw std::runtime_error(
            "walletpassphrasechange <oldpassphrase> <newpassphrase>\n"
            "Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");

    if (!pwalletMain->ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass))
        throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");

    return NullUniValue;
}


UniValue walletlock(const UniValue& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() != 0))
        throw std::runtime_error(
            "walletlock\n"
            "\nRemoves the wallet encryption key from memory, locking the wallet.\n"
            "After calling this method, you will need to call walletpassphrase again\n"
            "before being able to call any methods which require the wallet to be unlocked.\n"

            "\nExamples:\n"
            "\nSet the passphrase for 2 minutes to perform a transaction\n" +
            HelpExampleCli("walletpassphrase", "\"my pass phrase\" 120") +
            "\nPerform a send (requires passphrase set)\n" +
            HelpExampleCli("sendtoaddress", "\"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\" 1.0") +
            "\nClear the passphrase since we are done before 2 minutes is up\n" +
            HelpExampleCli("walletlock", "") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("walletlock", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletlock was called.");

    {
        LOCK(cs_nWalletUnlockTime);
        pwalletMain->Lock();
        nWalletUnlockTime = 0;
    }

    return NullUniValue;
}


UniValue encryptwallet(const UniValue& params, bool fHelp)
{
    if (!pwalletMain->IsCrypted() && (fHelp || params.size() != 1))
        throw std::runtime_error(
            "encryptwallet \"passphrase\"\n"
            "\nEncrypts the wallet with 'passphrase'. This is for first time encryption.\n"
            "After this, any calls that interact with private keys such as sending or signing \n"
            "will require the passphrase to be set prior the making these calls.\n"
            "Use the walletpassphrase call for this, and then walletlock call.\n"
            "If the wallet is already encrypted, use the walletpassphrasechange call.\n"
            "Note that this will shutdown the server.\n"

            "\nArguments:\n"
            "1. \"passphrase\"    (string) The pass phrase to encrypt the wallet with. It must be at least 1 character, but should be long.\n"

            "\nExamples:\n"
            "\nEncrypt you wallet\n" +
            HelpExampleCli("encryptwallet", "\"my pass phrase\"") +
            "\nNow set the passphrase to use the wallet, such as for signing or sending BTCUs\n" +
            HelpExampleCli("walletpassphrase", "\"my pass phrase\"") +
            "\nNow we can so something like sign\n" +
            HelpExampleCli("signmessage", "\"btcuaddress\" \"test message\"") +
            "\nNow lock the wallet again by removing the passphrase\n" +
            HelpExampleCli("walletlock", "") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("encryptwallet", "\"my pass phrase\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (fHelp)
        return true;
    if (pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an encrypted wallet, but encryptwallet was called.");

    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    strWalletPass = params[0].get_str().c_str();

    if (strWalletPass.length() < 1)
        throw std::runtime_error(
            "encryptwallet <passphrase>\n"
            "Encrypts the wallet with <passphrase>.");

    if (!pwalletMain->EncryptWallet(strWalletPass))
        throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Error: Failed to encrypt the wallet.");

    // BDB seems to have a bad habit of writing old data into
    // slack space in .dat files; that is bad if the old data is
    // unencrypted private keys. So:
    StartShutdown();
    return "wallet encrypted; btcu server stopping, restart to run with encrypted wallet. The keypool has been flushed, you need to make a new backup.";
}

UniValue lockunspent(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "lockunspent unlock [{\"txid\":\"txid\",\"vout\":n},...]\n"
            "\nUpdates list of temporarily unspendable outputs.\n"
            "Temporarily lock (unlock=false) or unlock (unlock=true) specified transaction outputs.\n"
            "A locked transaction output will not be chosen by automatic coin selection, when spending BTCUs.\n"
            "Locks are stored in memory only. Nodes start with zero locked outputs, and the locked output list\n"
            "is always cleared (by virtue of process exit) when a node stops or fails.\n"
            "Also see the listunspent call\n"

            "\nArguments:\n"
            "1. unlock            (boolean, required) Whether to unlock (true) or lock (false) the specified transactions\n"
            "2. \"transactions\"  (string, required) A json array of objects. Each object the txid (string) vout (numeric)\n"
            "     [           (json array of json objects)\n"
            "       {\n"
            "         \"txid\":\"id\",    (string) The transaction id\n"
            "         \"vout\": n         (numeric) The output number\n"
            "       }\n"
            "       ,...\n"
            "     ]\n"

            "\nResult:\n"
            "true|false    (boolean) Whether the command was successful or not\n"

            "\nExamples:\n"
            "\nList the unspent transactions\n" +
            HelpExampleCli("listunspent", "") +
            "\nLock an unspent transaction\n" +
            HelpExampleCli("lockunspent", "false \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
            "\nList the locked transactions\n" +
            HelpExampleCli("listlockunspent", "") +
            "\nUnlock the transaction again\n" +
            HelpExampleCli("lockunspent", "true \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("lockunspent", "false, \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (params.size() == 1)
        RPCTypeCheck(params, boost::assign::list_of(UniValue::VBOOL));
    else
        RPCTypeCheck(params, boost::assign::list_of(UniValue::VBOOL)(UniValue::VARR));

    bool fUnlock = params[0].get_bool();

    if (params.size() == 1) {
        if (fUnlock)
            pwalletMain->UnlockAllCoins();
        return true;
    }

    UniValue output_params = params[1].get_array();

    // Create and validate the COutPoints first.
    std::vector<COutPoint> outputs;
    outputs.reserve(output_params.size());

    for (unsigned int idx = 0; idx < output_params.size(); idx++) {
        const UniValue& output = output_params[idx];
        if (!output.isObject())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected object");
        const UniValue& o = output.get_obj();

        RPCTypeCheckObj(o, boost::assign::map_list_of("txid", UniValue::VSTR)("vout", UniValue::VNUM));

        const std::string& txid = find_value(o, "txid").get_str();
        if (!IsHex(txid)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected hex txid");
        }

        const int nOutput = find_value(o, "vout").get_int();
        if (nOutput < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");
        }

        const COutPoint outpt(uint256(txid), nOutput);

        const auto it = pwalletMain->mapWallet.find(outpt.hash);
        if (it == pwalletMain->mapWallet.end()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, unknown transaction");
        }

        const CWalletTx& wtx = it->second;

        if (outpt.n >= wtx.vout.size()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout index out of bounds");
        }

        if (pwalletMain->IsSpent(outpt.hash, outpt.n)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected unspent output");
        }

        const bool is_locked = pwalletMain->IsLockedCoin(outpt.hash, outpt.n);

        if (fUnlock && !is_locked) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected locked output");
        }

        if (!fUnlock && is_locked) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, output already locked");
        }

        outputs.push_back(outpt);
    }

    // Atomically set (un)locked status for the outputs.
    for (const COutPoint& outpt : outputs) {
        if (fUnlock) pwalletMain->UnlockCoin(outpt);
        else pwalletMain->LockCoin(outpt);
    }

    return true;
}

UniValue listlockunspent(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw std::runtime_error(
            "listlockunspent\n"
            "\nReturns list of temporarily unspendable outputs.\n"
            "See the lockunspent call to lock and unlock transactions for spending.\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txid\" : \"transactionid\",     (string) The transaction id locked\n"
            "    \"vout\" : n                      (numeric) The vout value\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n"
            "\nList the unspent transactions\n" +
            HelpExampleCli("listunspent", "") +
            "\nLock an unspent transaction\n" +
            HelpExampleCli("lockunspent", "false \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
            "\nList the locked transactions\n" +
            HelpExampleCli("listlockunspent", "") +
            "\nUnlock the transaction again\n" +
            HelpExampleCli("lockunspent", "true \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("listlockunspent", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    std::vector<COutPoint> vOutpts;
    pwalletMain->ListLockedCoins(vOutpts);

    UniValue ret(UniValue::VARR);

    for (COutPoint& outpt : vOutpts) {
        UniValue o(UniValue::VOBJ);

        o.push_back(Pair("txid", outpt.hash.GetHex()));
        o.push_back(Pair("vout", (int)outpt.n));
        ret.push_back(o);
    }

    return ret;
}

UniValue settxfee(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1)
        throw std::runtime_error(
            "settxfee amount\n"
            "\nSet the transaction fee per kB.\n"

            "\nArguments:\n"
            "1. amount         (numeric, required) The transaction fee in BTCU/kB rounded to the nearest 0.00000001\n"

            "\nResult\n"
            "true|false        (boolean) Returns true if successful\n"
            "\nExamples:\n" +
            HelpExampleCli("settxfee", "0.00001") + HelpExampleRpc("settxfee", "0.00001"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    // Amount
    CAmount nAmount = 0;
    if (params[0].get_real() != 0.0)
        nAmount = AmountFromValue(params[0]); // rejects 0.0 amounts

    payTxFee = CFeeRate(nAmount, 1000);
    return true;
}

UniValue getwalletinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getwalletinfo\n"
            "Returns an object containing various wallet state info.\n"

            "\nResult:\n"
            "{\n"
            "  \"walletversion\": xxxxx,                  (numeric) the wallet version\n"
            "  \"balance\": xxxxxxx,                      (numeric) the total BTCU balance of the wallet (cold balance excluded)\n"
            "  \"delegated_balance\": xxxxx,              (numeric) the BTCU balance held in P2CS (cold staking) contracts\n"
            "  \"cold_staking_balance\": xx,              (numeric) the BTCU balance held in cold staking addresses\n"
            "  \"leased_balance\": xxxxx,                 (numeric) the BTCU balance held in P2L (leasing) contracts\n"
            "  \"leasing_balance\": xxxxx,                (numeric) the BTCU balance held in leasing addresses\n"
            "  \"unconfirmed_balance\": xxx,              (numeric) the total unconfirmed balance of the wallet in BTCU\n"
            "  \"immature_delegated_balance\": xxxxxx,    (numeric) the delegated immature balance of the wallet in BTCU\n"
            "  \"immature_cold_staking_balance\": xxxxxx, (numeric) the cold-staking immature balance of the wallet in BTCU\n"
            "  \"immature_leased_balance\": xxxxxx,       (numeric) the leased immature balance of the wallet in BTCU\n"
            "  \"immature_leasing_balance\": xxxxxx,      (numeric) the leasing immature balance of the wallet in BTCU\n"
            "  \"immature_balance\": xxxxxx,              (numeric) the total immature balance of the wallet in BTCU\n"
            "  \"txcount\": xxxxxxx,                      (numeric) the total number of transactions in the wallet\n"
            "  \"keypoololdest\": xxxxxx,                 (numeric) the timestamp (seconds since GMT epoch) of the oldest pre-generated key in the key pool\n"
            "  \"keypoolsize\": xxxx,                     (numeric) how many new keys are pre-generated\n"
            "  \"unlocked_until\": ttt,                   (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is locked\n"
            "  \"paytxfee\": x.xxxx                       (numeric) the transaction fee configuration, set in BTCU/kB\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("getwalletinfo", "") + HelpExampleRpc("getwalletinfo", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    UniValue obj(UniValue::VOBJ);
    obj.push_back(Pair("walletversion", pwalletMain->GetVersion()));
    obj.push_back(Pair("balance", ValueFromAmount(pwalletMain->GetBalance())));
    obj.push_back(Pair("delegated_balance", ValueFromAmount(pwalletMain->GetDelegatedBalance())));
    obj.push_back(Pair("cold_staking_balance", ValueFromAmount(pwalletMain->GetColdStakingBalance())));
    obj.push_back(Pair("leased_balance", ValueFromAmount(pwalletMain->GetLeasedBalance())));
    obj.push_back(Pair("leasing_balance", ValueFromAmount(pwalletMain->GetLeasingBalance())));
    obj.push_back(Pair("unconfirmed_balance", ValueFromAmount(pwalletMain->GetUnconfirmedBalance())));
    obj.push_back(Pair("immature_balance",    ValueFromAmount(pwalletMain->GetImmatureBalance())));
    obj.push_back(Pair("immature_delegated_balance",    ValueFromAmount(pwalletMain->GetImmatureDelegatedBalance())));
    obj.push_back(Pair("immature_cold_staking_balance",    ValueFromAmount(pwalletMain->GetImmatureColdStakingBalance())));
    obj.push_back(Pair("immature_leased_balance", ValueFromAmount(pwalletMain->GetImmatureLeasedBalance())));
    obj.push_back(Pair("immature_leasing_balance", ValueFromAmount(pwalletMain->GetImmatureLeasingBalance())));
    obj.push_back(Pair("txcount", (int)pwalletMain->mapWallet.size()));
    obj.push_back(Pair("keypoololdest", pwalletMain->GetOldestKeyPoolTime()));
    obj.push_back(Pair("keypoolsize", (int)pwalletMain->GetKeyPoolSize()));
    if (pwalletMain->IsCrypted())
        obj.push_back(Pair("unlocked_until", nWalletUnlockTime));
    obj.push_back(Pair("paytxfee",      ValueFromAmount(payTxFee.GetFeePerK())));
    return obj;
}

// ppcoin: reserve balance from being staked for network protection
UniValue reservebalance(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw std::runtime_error(
            "reservebalance ( reserve amount )\n"
            "\nShow or set the reserve amount not participating in network protection\n"
            "If no parameters provided current setting is printed.\n"

            "\nArguments:\n"
            "1. reserve     (boolean, optional) is true or false to turn balance reserve on or off.\n"
            "2. amount      (numeric, optional) is a real and rounded to cent.\n"

            "\nResult:\n"
            "{\n"
            "  \"reserve\": true|false,     (boolean) Status of the reserve balance\n"
            "  \"amount\": x.xxxx       (numeric) Amount reserved\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("reservebalance", "true 5000") + HelpExampleRpc("reservebalance", "true 5000"));

    if (params.size() > 0) {
        bool fReserve = params[0].get_bool();
        if (fReserve) {
            if (params.size() == 1)
                throw std::runtime_error("must provide amount to reserve balance.\n");
            CAmount nAmount = AmountFromValue(params[1]);
            nAmount = (nAmount / CENT) * CENT; // round to cent
            if (nAmount < 0)
                throw std::runtime_error("amount cannot be negative.\n");
            nReserveBalance = nAmount;
        } else {
            if (params.size() > 1)
                throw std::runtime_error("cannot specify amount to turn off reserve.\n");
            nReserveBalance = 0;
        }
    }

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("reserve", (nReserveBalance > 0)));
    result.push_back(Pair("amount", ValueFromAmount(nReserveBalance)));
    return result;
}

// presstab HyperStake
UniValue setstakesplitthreshold(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw std::runtime_error(
            "setstakesplitthreshold value\n"
            "\nThis will set the output size of your stakes to never be below this number\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. value   (numeric, required) Threshold value between 1 and 999999\n"

            "\nResult:\n"
            "{\n"
            "  \"threshold\": n,    (numeric) Threshold value set\n"
            "  \"saved\": true|false    (boolean) 'true' if successfully saved to the wallet file\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("setstakesplitthreshold", "5000") + HelpExampleRpc("setstakesplitthreshold", "5000"));

    EnsureWalletIsUnlocked();

    uint64_t nStakeSplitThreshold = params[0].get_int();

    if (nStakeSplitThreshold > 999999)
        throw std::runtime_error("Value out of range, max allowed is 999999");

    CWalletDB walletdb(pwalletMain->strWalletFile);
    LOCK(pwalletMain->cs_wallet);
    {
        bool fFileBacked = pwalletMain->fFileBacked;

        UniValue result(UniValue::VOBJ);
        pwalletMain->nStakeSplitThreshold = nStakeSplitThreshold;
        result.push_back(Pair("threshold", int(pwalletMain->nStakeSplitThreshold)));
        if (fFileBacked) {
            walletdb.WriteStakeSplitThreshold(nStakeSplitThreshold);
            result.push_back(Pair("saved", "true"));
        } else
            result.push_back(Pair("saved", "false"));

        return result;
    }
}

// presstab HyperStake
UniValue getstakesplitthreshold(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getstakesplitthreshold\n"
            "Returns the threshold for stake splitting\n"

            "\nResult:\n"
            "n      (numeric) Threshold value\n"

            "\nExamples:\n" +
            HelpExampleCli("getstakesplitthreshold", "") + HelpExampleRpc("getstakesplitthreshold", ""));

    return int(pwalletMain->nStakeSplitThreshold);
}

UniValue autocombinerewards(const UniValue& params, bool fHelp)
{
    bool fEnable;
    if (params.size() >= 1)
        fEnable = params[0].get_bool();

    if (fHelp || params.size() < 1 || (fEnable && params.size() != 2) || params.size() > 2)
        throw std::runtime_error(
            "autocombinerewards enable ( threshold )\n"
            "\nWallet will automatically monitor for any coins with value below the threshold amount, and combine them if they reside with the same BTCU address\n"
            "When autocombinerewards runs it will create a transaction, and therefore will be subject to transaction fees.\n"

            "\nArguments:\n"
            "1. enable          (boolean, required) Enable auto combine (true) or disable (false)\n"
            "2. threshold       (numeric, optional) Threshold amount (default: 0)\n"

            "\nExamples:\n" +
            HelpExampleCli("autocombinerewards", "true 500") + HelpExampleRpc("autocombinerewards", "true 500"));

    CWalletDB walletdb(pwalletMain->strWalletFile);
    CAmount nThreshold = 0;

    if (fEnable)
        nThreshold = params[1].get_int();

    pwalletMain->fCombineDust = fEnable;
    pwalletMain->nAutoCombineThreshold = nThreshold;

    if (!walletdb.WriteAutoCombineSettings(fEnable, nThreshold))
        throw std::runtime_error("Changed settings in wallet but failed to save to database\n");

    return NullUniValue;
}

UniValue printMultiSend()
{
    UniValue ret(UniValue::VARR);
    UniValue act(UniValue::VOBJ);
    act.push_back(Pair("MultiSendStake Activated?", pwalletMain->fMultiSendStake));
    act.push_back(Pair("MultiSendMasternode Activated?", pwalletMain->fMultiSendMasternodeReward));
    ret.push_back(act);

    if (pwalletMain->vDisabledAddresses.size() >= 1) {
        UniValue disAdd(UniValue::VOBJ);
        for (unsigned int i = 0; i < pwalletMain->vDisabledAddresses.size(); i++) {
            disAdd.push_back(Pair("Disabled From Sending", pwalletMain->vDisabledAddresses[i]));
        }
        ret.push_back(disAdd);
    }

    ret.push_back("MultiSend Addresses to Send To:");

    UniValue vMS(UniValue::VOBJ);
    for (unsigned int i = 0; i < pwalletMain->vMultiSend.size(); i++) {
        vMS.push_back(Pair("Address " + std::to_string(i), pwalletMain->vMultiSend[i].first));
        vMS.push_back(Pair("Percent", pwalletMain->vMultiSend[i].second));
    }

    ret.push_back(vMS);
    return ret;
}

UniValue printAddresses()
{
    std::vector<COutput> vCoins;
    pwalletMain->AvailableCoins(&vCoins);
    std::map<std::string, double> mapAddresses;
    for (const COutput& out : vCoins) {
        CTxDestination utxoAddress;
        ExtractDestination(out.tx->vout[out.i].scriptPubKey, utxoAddress);
        std::string strAdd = CBTCUAddress(utxoAddress).ToString();

        if (mapAddresses.find(strAdd) == mapAddresses.end()) //if strAdd is not already part of the map
            mapAddresses[strAdd] = (double)out.tx->vout[out.i].nValue / (double)COIN;
        else
            mapAddresses[strAdd] += (double)out.tx->vout[out.i].nValue / (double)COIN;
    }

    UniValue ret(UniValue::VARR);
    for (std::map<std::string, double>::const_iterator it = mapAddresses.begin(); it != mapAddresses.end(); ++it) {
        UniValue obj(UniValue::VOBJ);
        const std::string* strAdd = &(*it).first;
        const double* nBalance = &(*it).second;
        obj.push_back(Pair("Address ", *strAdd));
        obj.push_back(Pair("Balance ", *nBalance));
        ret.push_back(obj);
    }

    return ret;
}

unsigned int sumMultiSend()
{
    unsigned int sum = 0;
    for (unsigned int i = 0; i < pwalletMain->vMultiSend.size(); i++)
        sum += pwalletMain->vMultiSend[i].second;
    return sum;
}

UniValue multisend(const UniValue& params, bool fHelp)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);
    bool fFileBacked;
    //MultiSend Commands
    if (params.size() == 1) {
        std::string strCommand = params[0].get_str();
        UniValue ret(UniValue::VOBJ);
        if (strCommand == "print") {
            return printMultiSend();
        } else if (strCommand == "printaddress" || strCommand == "printaddresses") {
            return printAddresses();
        } else if (strCommand == "clear") {
            LOCK(pwalletMain->cs_wallet);
            {
                bool erased = false;
                if (pwalletMain->fFileBacked) {
                    if (walletdb.EraseMultiSend(pwalletMain->vMultiSend))
                        erased = true;
                }

                pwalletMain->vMultiSend.clear();
                pwalletMain->setMultiSendDisabled();

                UniValue obj(UniValue::VOBJ);
                obj.push_back(Pair("Erased from database", erased));
                obj.push_back(Pair("Erased from RAM", true));

                return obj;
            }
        } else if (strCommand == "enablestake" || strCommand == "activatestake") {
            if (pwalletMain->vMultiSend.size() < 1)
                throw JSONRPCError(RPC_INVALID_REQUEST, "Unable to activate MultiSend, check MultiSend vector");

            if (CBTCUAddress(pwalletMain->vMultiSend[0].first).IsValid()) {
                pwalletMain->fMultiSendStake = true;
                if (!walletdb.WriteMSettings(true, pwalletMain->fMultiSendMasternodeReward, pwalletMain->nLastMultiSendHeight)) {
                    UniValue obj(UniValue::VOBJ);
                    obj.push_back(Pair("error", "MultiSend activated but writing settings to DB failed"));
                    UniValue arr(UniValue::VARR);
                    arr.push_back(obj);
                    arr.push_back(printMultiSend());
                    return arr;
                } else
                    return printMultiSend();
            }

            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to activate MultiSend, check MultiSend vector");
        } else if (strCommand == "enablemasternode" || strCommand == "activatemasternode") {
            if (pwalletMain->vMultiSend.size() < 1)
                throw JSONRPCError(RPC_INVALID_REQUEST, "Unable to activate MultiSend, check MultiSend vector");

            if (CBTCUAddress(pwalletMain->vMultiSend[0].first).IsValid()) {
                pwalletMain->fMultiSendMasternodeReward = true;

                if (!walletdb.WriteMSettings(pwalletMain->fMultiSendStake, true, pwalletMain->nLastMultiSendHeight)) {
                    UniValue obj(UniValue::VOBJ);
                    obj.push_back(Pair("error", "MultiSend activated but writing settings to DB failed"));
                    UniValue arr(UniValue::VARR);
                    arr.push_back(obj);
                    arr.push_back(printMultiSend());
                    return arr;
                } else
                    return printMultiSend();
            }

            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to activate MultiSend, check MultiSend vector");
        } else if (strCommand == "disable" || strCommand == "deactivate") {
            pwalletMain->setMultiSendDisabled();
            if (!walletdb.WriteMSettings(false, false, pwalletMain->nLastMultiSendHeight))
                throw JSONRPCError(RPC_DATABASE_ERROR, "MultiSend deactivated but writing settings to DB failed");

            return printMultiSend();
        } else if (strCommand == "enableall") {
            if (!walletdb.EraseMSDisabledAddresses(pwalletMain->vDisabledAddresses))
                return "failed to clear old vector from walletDB";
            else {
                pwalletMain->vDisabledAddresses.clear();
                return printMultiSend();
            }
        }
    }
    if (params.size() == 2 && params[0].get_str() == "delete") {
        int del = std::stoi(params[1].get_str().c_str());
        if (!walletdb.EraseMultiSend(pwalletMain->vMultiSend))
            throw JSONRPCError(RPC_DATABASE_ERROR, "failed to delete old MultiSend vector from database");

        pwalletMain->vMultiSend.erase(pwalletMain->vMultiSend.begin() + del);
        if (!walletdb.WriteMultiSend(pwalletMain->vMultiSend))
            throw JSONRPCError(RPC_DATABASE_ERROR, "walletdb WriteMultiSend failed!");

        return printMultiSend();
    }
    if (params.size() == 2 && params[0].get_str() == "disable") {
        std::string disAddress = params[1].get_str();
        if (!CBTCUAddress(disAddress).IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "address you want to disable is not valid");
        else {
            pwalletMain->vDisabledAddresses.push_back(disAddress);
            if (!walletdb.EraseMSDisabledAddresses(pwalletMain->vDisabledAddresses))
                throw JSONRPCError(RPC_DATABASE_ERROR, "disabled address from sending, but failed to clear old vector from walletDB");

            if (!walletdb.WriteMSDisabledAddresses(pwalletMain->vDisabledAddresses))
                throw JSONRPCError(RPC_DATABASE_ERROR, "disabled address from sending, but failed to store it to walletDB");
            else
                return printMultiSend();
        }
    }

    //if no commands are used
    if (fHelp || params.size() != 2)
        throw std::runtime_error(
            "multisend <command>\n"
            "****************************************************************\n"
            "WHAT IS MULTISEND?\n"
            "MultiSend allows a user to automatically send a percent of their stake reward to as many addresses as you would like\n"
            "The MultiSend transaction is sent when the staked coins mature (100 confirmations)\n"
            "****************************************************************\n"
            "TO CREATE OR ADD TO THE MULTISEND VECTOR:\n"
            "multisend <BTCU Address> <percent>\n"
            "This will add a new address to the MultiSend vector\n"
            "Percent is a whole number 1 to 100.\n"
            "****************************************************************\n"
            "MULTISEND COMMANDS (usage: multisend <command>)\n"
            " print - displays the current MultiSend vector \n"
            " clear - deletes the current MultiSend vector \n"
            " enablestake/activatestake - activates the current MultiSend vector to be activated on stake rewards\n"
            " enablemasternode/activatemasternode - activates the current MultiSend vector to be activated on masternode rewards\n"
            " disable/deactivate - disables the current MultiSend vector \n"
            " delete <Address #> - deletes an address from the MultiSend vector \n"
            " disable <address> - prevents a specific address from sending MultiSend transactions\n"
            " enableall - enables all addresses to be eligible to send MultiSend transactions\n"
            "****************************************************************\n");

    //if the user is entering a new MultiSend item
    std::string strAddress = params[0].get_str();
    CBTCUAddress address(strAddress);
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU address");
    if (std::stoi(params[1].get_str().c_str()) < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected valid percentage");
    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    unsigned int nPercent = (unsigned int) std::stoul(params[1].get_str().c_str());

    LOCK(pwalletMain->cs_wallet);
    {
        fFileBacked = pwalletMain->fFileBacked;
        //Error if 0 is entered
        if (nPercent == 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Sending 0% of stake is not valid");
        }

        //MultiSend can only send 100% of your stake
        if (nPercent + sumMultiSend() > 100)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to add to MultiSend vector, the sum of your MultiSend is greater than 100%");

        for (unsigned int i = 0; i < pwalletMain->vMultiSend.size(); i++) {
            if (pwalletMain->vMultiSend[i].first == strAddress)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to add to MultiSend vector, cannot use the same address twice");
        }

        if (fFileBacked)
            walletdb.EraseMultiSend(pwalletMain->vMultiSend);

        std::pair<std::string, int> newMultiSend;
        newMultiSend.first = strAddress;
        newMultiSend.second = nPercent;
        pwalletMain->vMultiSend.push_back(newMultiSend);
        if (fFileBacked) {
            if (!walletdb.WriteMultiSend(pwalletMain->vMultiSend))
                throw JSONRPCError(RPC_DATABASE_ERROR, "walletdb WriteMultiSend failed!");
        }
    }
    return printMultiSend();
}

UniValue getzerocoinbalance(const UniValue& params, bool fHelp)
{

    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getzerocoinbalance\n"
            "\nReturn the wallet's total zBTCU balance.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nResult:\n"
            "amount         (numeric) Total zBTCU balance.\n"

            "\nExamples:\n" +
            HelpExampleCli("getzerocoinbalance", "") + HelpExampleRpc("getzerocoinbalance", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked(true);

        UniValue ret(UniValue::VOBJ);
        ret.push_back(Pair("Total", ValueFromAmount(pwalletMain->GetZerocoinBalance(false))));
        ret.push_back(Pair("Mature", ValueFromAmount(pwalletMain->GetZerocoinBalance(true))));
        ret.push_back(Pair("Unconfirmed", ValueFromAmount(pwalletMain->GetUnconfirmedZerocoinBalance())));
        ret.push_back(Pair("Immature", ValueFromAmount(pwalletMain->GetImmatureZerocoinBalance())));
        return ret;

}

UniValue listmintedzerocoins(const UniValue& params, bool fHelp)
{

    if (fHelp || params.size() > 2)
        throw std::runtime_error(
            "listmintedzerocoins (fVerbose) (fMatureOnly)\n"
            "\nList all zBTCU mints in the wallet.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. fVerbose      (boolean, optional, default=false) Output mints metadata.\n"
            "2. fMatureOnly   (boolean, optional, default=false) List only mature mints.\n"
            "                 Set only if fVerbose is specified\n"

            "\nResult (with fVerbose=false):\n"
            "[\n"
            "  \"xxx\"      (string) Pubcoin in hex format.\n"
            "  ,...\n"
            "]\n"

            "\nResult (with fVerbose=true):\n"
            "[\n"
            "  {\n"
            "    \"serial hash\": \"xxx\",   (string) Mint serial hash in hex format.\n"
            "    \"version\": n,   (numeric) Zerocoin version number.\n"
            "    \"zBTCU ID\": \"xxx\",   (string) Pubcoin in hex format.\n"
            "    \"denomination\": n,   (numeric) Coin denomination.\n"
            "    \"mint height\": n     (numeric) Height of the block containing this mint.\n"
            "    \"confirmations\": n   (numeric) Number of confirmations.\n"
            "    \"hash stake\": \"xxx\",   (string) Mint serialstake hash in hex format.\n"
            "  }\n"
            "  ,..."
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("listmintedzerocoins", "") + HelpExampleRpc("listmintedzerocoins", "") +
            HelpExampleCli("listmintedzerocoins", "true") + HelpExampleRpc("listmintedzerocoins", "true") +
            HelpExampleCli("listmintedzerocoins", "true true") + HelpExampleRpc("listmintedzerocoins", "true, true"));

    bool fVerbose = (params.size() > 0) ? params[0].get_bool() : false;
    bool fMatureOnly = (params.size() > 1) ? params[1].get_bool() : false;

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked(true);

    CWalletDB walletdb(pwalletMain->strWalletFile);
    std::set<CMintMeta> setMints = pwalletMain->zbtcuTracker->ListMints(true, fMatureOnly, true);

    int nBestHeight = chainActive.Height();

    UniValue jsonList(UniValue::VARR);
    if (fVerbose) {
        for (auto m : setMints) {
            // Construct mint object
            UniValue objMint(UniValue::VOBJ);
            objMint.push_back(Pair("serial hash", m.hashSerial.GetHex()));  // Serial hash
            objMint.push_back(Pair("version", m.nVersion));                 // Zerocoin version
            objMint.push_back(Pair("zBTCU ID", m.hashPubcoin.GetHex()));     // PubCoin
            int denom = libzerocoin::ZerocoinDenominationToInt(m.denom);
            objMint.push_back(Pair("denomination", denom));                 // Denomination
            objMint.push_back(Pair("mint height", m.nHeight));              // Mint Height
            int nConfirmations = (m.nHeight && nBestHeight > m.nHeight) ? nBestHeight - m.nHeight : 0;
            objMint.push_back(Pair("confirmations", nConfirmations));       // Confirmations
            if (m.hashStake == 0) {
                CZerocoinMint mint;
                if (pwalletMain->GetMint(m.hashSerial, mint)) {
                    uint256 hashStake = mint.GetSerialNumber().getuint256();
                    hashStake = Hash(hashStake.begin(), hashStake.end());
                    m.hashStake = hashStake;
                    pwalletMain->zbtcuTracker->UpdateState(m);
                }
            }
            objMint.push_back(Pair("hash stake", m.hashStake.GetHex()));    // hashStake
            // Push back mint object
            jsonList.push_back(objMint);
        }
    } else {
        for (const CMintMeta& m : setMints)
            // Push back PubCoin
            jsonList.push_back(m.hashPubcoin.GetHex());
    }
    return jsonList;
}

UniValue listzerocoinamounts(const UniValue& params, bool fHelp)
{

    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "listzerocoinamounts\n"
            "\nGet information about your zerocoin amounts.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"denomination\": n,   (numeric) Denomination Value.\n"
            "    \"mints\": n           (numeric) Number of mints.\n"
            "  }\n"
            "  ,..."
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("listzerocoinamounts", "") + HelpExampleRpc("listzerocoinamounts", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked(true);

    CWalletDB walletdb(pwalletMain->strWalletFile);
    std::set<CMintMeta> setMints = pwalletMain->zbtcuTracker->ListMints(true, true, true);

    std::map<libzerocoin::CoinDenomination, CAmount> spread;
    for (const auto& denom : libzerocoin::zerocoinDenomList)
        spread.insert(std::pair<libzerocoin::CoinDenomination, CAmount>(denom, 0));
    for (auto& meta : setMints) spread.at(meta.denom)++;


    UniValue ret(UniValue::VARR);
    for (const auto& m : libzerocoin::zerocoinDenomList) {
        UniValue val(UniValue::VOBJ);
        val.push_back(Pair("denomination", libzerocoin::ZerocoinDenominationToInt(m)));
        val.push_back(Pair("mints", (int64_t)spread.at(m)));
        ret.push_back(val);
    }
    return ret;
}

UniValue listspentzerocoins(const UniValue& params, bool fHelp)
{

    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "listspentzerocoins\n"
            "\nList all the spent zBTCU mints in the wallet.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nResult:\n"
            "[\n"
            "  \"xxx\"      (string) Pubcoin in hex format.\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("listspentzerocoins", "") + HelpExampleRpc("listspentzerocoins", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked(true);

    CWalletDB walletdb(pwalletMain->strWalletFile);
    std::list<CBigNum> listPubCoin = walletdb.ListSpentCoinsSerial();

    UniValue jsonList(UniValue::VARR);
    for (const CBigNum& pubCoinItem : listPubCoin) {
        jsonList.push_back(pubCoinItem.GetHex());
    }

    return jsonList;
}

UniValue mintzerocoin(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "mintzerocoin amount ( utxos )\n"
            "\nMint the specified zBTCU amount\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. amount      (numeric, required) Enter an amount of Piv to convert to zBTCU\n"
            "2. utxos       (string, optional) A json array of objects.\n"
            "                   Each object needs the txid (string) and vout (numeric)\n"
            "  [\n"
            "    {\n"
            "      \"txid\":\"txid\",    (string) The transaction id\n"
            "      \"vout\": n         (numeric) The output number\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"

            "\nResult:\n"
            "{\n"
            "   \"txid\": \"xxx\",       (string) Transaction ID.\n"
            "   \"time\": nnn            (numeric) Time to mint this transaction.\n"
            "   \"mints\":\n"
            "   [\n"
            "      {\n"
            "         \"denomination\": nnn,     (numeric) Minted denomination.\n"
            "         \"pubcoin\": \"xxx\",      (string) Pubcoin in hex format.\n"
            "         \"randomness\": \"xxx\",   (string) Hex encoded randomness.\n"
            "         \"serial\": \"xxx\",       (string) Serial in hex format.\n"
            "      },\n"
            "      ...\n"
            "   ]\n"
            "}\n"

            "\nExamples:\n"
            "\nMint 50 from anywhere\n" +
            HelpExampleCli("mintzerocoin", "50") +
            "\nMint 13 from a specific output\n" +
            HelpExampleCli("mintzerocoin", "13 \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
            "\nAs a json rpc call\n" +
            HelpExampleRpc("mintzerocoin", "13, \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\""));


    if (!Params().IsRegTestNet())
        throw JSONRPCError(RPC_WALLET_ERROR, "zBTCU minting is DISABLED");

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (params.size() == 1)
    {
        RPCTypeCheck(params, boost::assign::list_of(UniValue::VNUM));
    } else
    {
        RPCTypeCheck(params, boost::assign::list_of(UniValue::VNUM)(UniValue::VARR));
    }

    int64_t nTime = GetTimeMillis();
    if(sporkManager.IsSporkActive(SPORK_16_ZEROCOIN_MAINTENANCE_MODE))
        throw JSONRPCError(RPC_WALLET_ERROR, "zBTCU is currently disabled due to maintenance.");

    EnsureWalletIsUnlocked(true);

    CAmount nAmount = params[0].get_int() * COIN;

    CWalletTx wtx;
    std::vector<CDeterministicMint> vDMints;
    std::string strError;
    std::vector<COutPoint> vOutpts;

    if (params.size() == 2)
    {
        UniValue outputs = params[1].get_array();
        for (unsigned int idx = 0; idx < outputs.size(); idx++) {
            const UniValue& output = outputs[idx];
            if (!output.isObject())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected object");
            const UniValue& o = output.get_obj();

            RPCTypeCheckObj(o, boost::assign::map_list_of("txid", UniValue::VSTR)("vout", UniValue::VNUM));

            std::string txid = find_value(o, "txid").get_str();
            if (!IsHex(txid))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected hex txid");

            int nOutput = find_value(o, "vout").get_int();
            if (nOutput < 0)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");

            COutPoint outpt(uint256(txid), nOutput);
            vOutpts.push_back(outpt);
        }
        strError = pwalletMain->MintZerocoinFromOutPoint(nAmount, wtx, vDMints, vOutpts);
    } else
    {
        strError = pwalletMain->MintZerocoin(nAmount, wtx, vDMints);
    }

    if (strError != "")
        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    UniValue retObj(UniValue::VOBJ);
    retObj.push_back(Pair("txid", wtx.GetHash().ToString()));
    retObj.push_back(Pair("time", GetTimeMillis() - nTime));
    UniValue arrMints(UniValue::VARR);
    for (CDeterministicMint dMint : vDMints) {
        UniValue m(UniValue::VOBJ);
        m.push_back(Pair("denomination", ValueFromAmount(libzerocoin::ZerocoinDenominationToAmount(dMint.GetDenomination()))));
        m.push_back(Pair("pubcoinhash", dMint.GetPubcoinHash().GetHex()));
        m.push_back(Pair("serialhash", dMint.GetSerialHash().GetHex()));
        m.push_back(Pair("seedhash", dMint.GetSeedHash().GetHex()));
        m.push_back(Pair("count", (int64_t)dMint.GetCount()));
        arrMints.push_back(m);
    }
    retObj.push_back(Pair("mints", arrMints));

    return retObj;
}

UniValue spendzerocoin(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 2 || params.size() < 1)
        throw std::runtime_error(
            "spendzerocoin amount ( \"address\" )\n"
            "\nSpend zBTCU to a BTCU address.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. amount          (numeric, required) Amount to spend.\n"
            "2. \"address\"     (string, optional, default=change) Send to specified address or to a new change address.\n"
            "                       If there is change then an address is required\n"

            "\nResult:\n"
            "{\n"
            "  \"txid\": \"xxx\",             (string) Transaction hash.\n"
            "  \"bytes\": nnn,              (numeric) Transaction size.\n"
            "  \"fee\": amount,             (numeric) Transaction fee (if any).\n"
            "  \"spends\": [                (array) JSON array of input objects.\n"
            "    {\n"
            "      \"denomination\": nnn,   (numeric) Denomination value.\n"
            "      \"pubcoin\": \"xxx\",      (string) Pubcoin in hex format.\n"
            "      \"serial\": \"xxx\",       (string) Serial number in hex format.\n"
            "      \"acc_checksum\": \"xxx\", (string) Accumulator checksum in hex format.\n"
            "    }\n"
            "    ,...\n"
            "  ],\n"
            "  \"outputs\": [                 (array) JSON array of output objects.\n"
            "    {\n"
            "      \"value\": amount,         (numeric) Value in BTCU.\n"
            "      \"address\": \"xxx\"         (string) BTCU address or \"zerocoinmint\" for reminted change.\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"

            "\nExamples\n" +
            HelpExampleCli("spendzerocoin", "5000 \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\"") +
            HelpExampleRpc("spendzerocoin", "5000 \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if(sporkManager.IsSporkActive(SPORK_16_ZEROCOIN_MAINTENANCE_MODE))
        throw JSONRPCError(RPC_WALLET_ERROR, "zBTCU is currently disabled due to maintenance.");

    CAmount nAmount = AmountFromValue(params[0]);        // Spending amount
    const std::string address_str = (params.size() > 1 ? params[1].get_str() : "");

    std::vector<CZerocoinMint> vMintsSelected;
    return DoZbtcuSpend(nAmount, vMintsSelected, address_str);
}


UniValue spendzerocoinmints(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw std::runtime_error(
            "spendzerocoinmints mints_list ( \"address\" ) \n"
            "\nSpend zBTCU mints to a BTCU address.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. mints_list     (string, required) A json array of zerocoin mints serial hashes\n"
            "2. \"address\"     (string, optional, default=change) Send to specified address or to a new change address.\n"

            "\nResult:\n"
            "{\n"
            "  \"txid\": \"xxx\",             (string) Transaction hash.\n"
            "  \"bytes\": nnn,              (numeric) Transaction size.\n"
            "  \"fee\": amount,             (numeric) Transaction fee (if any).\n"
            "  \"spends\": [                (array) JSON array of input objects.\n"
            "    {\n"
            "      \"denomination\": nnn,   (numeric) Denomination value.\n"
            "      \"pubcoin\": \"xxx\",      (string) Pubcoin in hex format.\n"
            "      \"serial\": \"xxx\",       (string) Serial number in hex format.\n"
            "      \"acc_checksum\": \"xxx\", (string) Accumulator checksum in hex format.\n"
            "    }\n"
            "    ,...\n"
            "  ],\n"
            "  \"outputs\": [                 (array) JSON array of output objects.\n"
            "    {\n"
            "      \"value\": amount,         (numeric) Value in BTCU.\n"
            "      \"address\": \"xxx\"         (string) BTCU address or \"zerocoinmint\" for reminted change.\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"

            "\nExamples\n" +
            HelpExampleCli("spendzerocoinmints", "'[\"0d8c16eee7737e3cc1e4e70dc006634182b175e039700931283b202715a0818f\", \"dfe585659e265e6a509d93effb906d3d2a0ac2fe3464b2c3b6d71a3ef34c8ad7\"]' \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\"") +
            HelpExampleRpc("spendzerocoinmints", "[\"0d8c16eee7737e3cc1e4e70dc006634182b175e039700931283b202715a0818f\", \"dfe585659e265e6a509d93effb906d3d2a0ac2fe3464b2c3b6d71a3ef34c8ad7\"], \"DMJRSsuU9zfyrvxVaAEFQqK4MxZg6vgeS6\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if(sporkManager.IsSporkActive(SPORK_16_ZEROCOIN_MAINTENANCE_MODE))
        throw JSONRPCError(RPC_WALLET_ERROR, "zBTCU is currently disabled due to maintenance.");

    UniValue arrMints = params[0].get_array();
    const std::string address_str = (params.size() > 1 ? params[1].get_str() : "");

    if (arrMints.size() == 0)
        throw JSONRPCError(RPC_WALLET_ERROR, "No zerocoin selected");

    // check mints supplied and save serial hash (do this here so we don't fetch if any is wrong)
    std::vector<uint256> vSerialHashes;
    for(unsigned int i = 0; i < arrMints.size(); i++) {
        std::string serialHashStr = arrMints[i].get_str();
        if (!IsHex(serialHashStr))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected hex serial hash");
        vSerialHashes.push_back(uint256(serialHashStr));
    }

    // fetch mints and update nAmount
    CAmount nAmount(0);
    std::vector<CZerocoinMint> vMintsSelected;
    for(const uint256& serialHash : vSerialHashes) {
        CZerocoinMint mint;
        if (!pwalletMain->GetMint(serialHash, mint)) {
            std::string strErr = "Failed to fetch mint associated with serial hash " + serialHash.GetHex();
            throw JSONRPCError(RPC_WALLET_ERROR, strErr);
        }
        vMintsSelected.emplace_back(mint);
        nAmount += mint.GetDenominationAsAmount();
    }

    return DoZbtcuSpend(nAmount, vMintsSelected, address_str);
}


extern UniValue DoZbtcuSpend(const CAmount nAmount, std::vector<CZerocoinMint>& vMintsSelected, std::string address_str)
{
    int64_t nTimeStart = GetTimeMillis();
    CBTCUAddress address = CBTCUAddress(); // Optional sending address. Dummy initialization here.
    CWalletTx wtx;
    CZerocoinSpendReceipt receipt;
    bool fSuccess;

    std::list<std::pair<CBTCUAddress*, CAmount>> outputs;
    if(address_str != "") { // Spend to supplied destination address
        address = CBTCUAddress(address_str);
        if(!address.IsValid() || address.IsStakingAddress())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid BTCU address");
        outputs.push_back(std::pair<CBTCUAddress*, CAmount>(&address, nAmount));
    }

    EnsureWalletIsUnlocked();
    fSuccess = pwalletMain->SpendZerocoin(nAmount, wtx, receipt, vMintsSelected, outputs, nullptr);

    if (!fSuccess)
        throw JSONRPCError(RPC_WALLET_ERROR, receipt.GetStatusMessage());

    CAmount nValueIn = 0;
    UniValue arrSpends(UniValue::VARR);
    for (CZerocoinSpend spend : receipt.GetSpends()) {
        UniValue obj(UniValue::VOBJ);
        obj.push_back(Pair("denomination", spend.GetDenomination()));
        obj.push_back(Pair("pubcoin", spend.GetPubCoin().GetHex()));
        obj.push_back(Pair("serial", spend.GetSerial().GetHex()));
        uint32_t nChecksum = spend.GetAccumulatorChecksum();
        obj.push_back(Pair("acc_checksum", HexStr(BEGIN(nChecksum), END(nChecksum))));
        arrSpends.push_back(obj);
        nValueIn += libzerocoin::ZerocoinDenominationToAmount(spend.GetDenomination());
    }

    CAmount nValueOut = 0;
    UniValue vout(UniValue::VARR);
    for (unsigned int i = 0; i < wtx.vout.size(); i++) {
        const CTxOut& txout = wtx.vout[i];
        UniValue out(UniValue::VOBJ);
        out.push_back(Pair("value", ValueFromAmount(txout.nValue)));
        nValueOut += txout.nValue;

        CTxDestination dest;
        if(txout.IsZerocoinMint())
            out.push_back(Pair("address", "zerocoinmint"));
        else if(ExtractDestination(txout.scriptPubKey, dest))
            out.push_back(Pair("address", CBTCUAddress(dest).ToString()));
        vout.push_back(out);
    }

    //construct JSON to return
    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("txid", wtx.GetHash().ToString()));
    ret.push_back(Pair("bytes", (int64_t)wtx.GetSerializeSize(SER_NETWORK, CTransaction::CURRENT_VERSION)));
    ret.push_back(Pair("fee", ValueFromAmount(nValueIn - nValueOut)));
    ret.push_back(Pair("duration_millis", (GetTimeMillis() - nTimeStart)));
    ret.push_back(Pair("spends", arrSpends));
    ret.push_back(Pair("outputs", vout));

    return ret;
}


UniValue resetmintzerocoin(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw std::runtime_error(
            "resetmintzerocoin ( fullscan )\n"
            "\nScan the blockchain for all of the zerocoins that are held in the wallet.dat.\n"
            "Update any meta-data that is incorrect. Archive any mints that are not able to be found.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. fullscan          (boolean, optional) Rescan each block of the blockchain.\n"
            "                               WARNING - may take 30+ minutes!\n"

            "\nResult:\n"
            "{\n"
            "  \"updated\": [       (array) JSON array of updated mints.\n"
            "    \"xxx\"            (string) Hex encoded mint.\n"
            "    ,...\n"
            "  ],\n"
            "  \"archived\": [      (array) JSON array of archived mints.\n"
            "    \"xxx\"            (string) Hex encoded mint.\n"
            "    ,...\n"
            "  ]\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("resetmintzerocoin", "true") + HelpExampleRpc("resetmintzerocoin", "true"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CWalletDB walletdb(pwalletMain->strWalletFile);
    CzBTCUTracker* zbtcuTracker = pwalletMain->zbtcuTracker.get();
    std::set<CMintMeta> setMints = zbtcuTracker->ListMints(false, false, true);
    std::vector<CMintMeta> vMintsToFind(setMints.begin(), setMints.end());
    std::vector<CMintMeta> vMintsMissing;
    std::vector<CMintMeta> vMintsToUpdate;

    // search all of our available data for these mints
    FindMints(vMintsToFind, vMintsToUpdate, vMintsMissing);

    // update the meta data of mints that were marked for updating
    UniValue arrUpdated(UniValue::VARR);
    for (CMintMeta meta : vMintsToUpdate) {
        zbtcuTracker->UpdateState(meta);
        arrUpdated.push_back(meta.hashPubcoin.GetHex());
    }

    // delete any mints that were unable to be located on the blockchain
    UniValue arrDeleted(UniValue::VARR);
    for (CMintMeta mint : vMintsMissing) {
        zbtcuTracker->Archive(mint);
        arrDeleted.push_back(mint.hashPubcoin.GetHex());
    }

    UniValue obj(UniValue::VOBJ);
    obj.push_back(Pair("updated", arrUpdated));
    obj.push_back(Pair("archived", arrDeleted));
    return obj;
}

UniValue resetspentzerocoin(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "resetspentzerocoin\n"
            "\nScan the blockchain for all of the zerocoins that are held in the wallet.dat.\n"
            "Reset mints that are considered spent that did not make it into the blockchain.\n"

            "\nResult:\n"
            "{\n"
            "  \"restored\": [        (array) JSON array of restored objects.\n"
            "    {\n"
            "      \"serial\": \"xxx\"  (string) Serial in hex format.\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"

            "\nExamples:\n" +
            HelpExampleCli("resetspentzerocoin", "") + HelpExampleRpc("resetspentzerocoin", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    CWalletDB walletdb(pwalletMain->strWalletFile);
    CzBTCUTracker* zbtcuTracker = pwalletMain->zbtcuTracker.get();
    std::set<CMintMeta> setMints = zbtcuTracker->ListMints(false, false, false);
    std::list<CZerocoinSpend> listSpends = walletdb.ListSpentCoins();
    std::list<CZerocoinSpend> listUnconfirmedSpends;

    for (CZerocoinSpend spend : listSpends) {
        CTransaction tx;
        uint256 hashBlock = 0;
        if (!GetTransaction(spend.GetTxHash(), tx, hashBlock)) {
            listUnconfirmedSpends.push_back(spend);
            continue;
        }

        //no confirmations
        if (hashBlock == 0)
            listUnconfirmedSpends.push_back(spend);
    }

    UniValue objRet(UniValue::VOBJ);
    UniValue arrRestored(UniValue::VARR);
    for (CZerocoinSpend spend : listUnconfirmedSpends) {
        for (auto& meta : setMints) {
            if (meta.hashSerial == GetSerialHash(spend.GetSerial())) {
                zbtcuTracker->SetPubcoinNotUsed(meta.hashPubcoin);
                walletdb.EraseZerocoinSpendSerialEntry(spend.GetSerial());
                RemoveSerialFromDB(spend.GetSerial());
                UniValue obj(UniValue::VOBJ);
                obj.push_back(Pair("serial", spend.GetSerial().GetHex()));
                arrRestored.push_back(obj);
                continue;
            }
        }
    }

    objRet.push_back(Pair("restored", arrRestored));
    return objRet;
}

UniValue getarchivedzerocoin(const UniValue& params, bool fHelp)
{
    if(fHelp || params.size() != 0)
        throw std::runtime_error(
            "getarchivedzerocoin\n"
            "\nDisplay zerocoins that were archived because they were believed to be orphans.\n"
            "Provides enough information to recover mint if it was incorrectly archived.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txid\": \"xxx\",           (string) Transaction ID for archived mint.\n"
            "    \"denomination\": amount,  (numeric) Denomination value.\n"
            "    \"serial\": \"xxx\",         (string) Serial number in hex format.\n"
            "    \"randomness\": \"xxx\",     (string) Hex encoded randomness.\n"
            "    \"pubcoin\": \"xxx\"         (string) Pubcoin in hex format.\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("getarchivedzerocoin", "") + HelpExampleRpc("getarchivedzerocoin", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    CWalletDB walletdb(pwalletMain->strWalletFile);
    std::list<CZerocoinMint> listMints = walletdb.ListArchivedZerocoins();
    std::list<CDeterministicMint> listDMints = walletdb.ListArchivedDeterministicMints();

    UniValue arrRet(UniValue::VARR);
    for (const CZerocoinMint& mint : listMints) {
        UniValue objMint(UniValue::VOBJ);
        objMint.push_back(Pair("txid", mint.GetTxHash().GetHex()));
        objMint.push_back(Pair("denomination", ValueFromAmount(mint.GetDenominationAsAmount())));
        objMint.push_back(Pair("serial", mint.GetSerialNumber().GetHex()));
        objMint.push_back(Pair("randomness", mint.GetRandomness().GetHex()));
        objMint.push_back(Pair("pubcoin", mint.GetValue().GetHex()));
        arrRet.push_back(objMint);
    }

    for (const CDeterministicMint& dMint : listDMints) {
        UniValue objDMint(UniValue::VOBJ);
        objDMint.push_back(Pair("txid", dMint.GetTxHash().GetHex()));
        objDMint.push_back(Pair("denomination", ValueFromAmount(libzerocoin::ZerocoinDenominationToAmount(dMint.GetDenomination()))));
        objDMint.push_back(Pair("serialhash", dMint.GetSerialHash().GetHex()));
        objDMint.push_back(Pair("pubcoinhash", dMint.GetPubcoinHash().GetHex()));
        objDMint.push_back(Pair("seedhash", dMint.GetSeedHash().GetHex()));
        objDMint.push_back(Pair("count", (int64_t)dMint.GetCount()));
        arrRet.push_back(objDMint);
    }

    return arrRet;
}

UniValue exportzerocoins(const UniValue& params, bool fHelp)
{
    if(fHelp || params.empty() || params.size() > 2)
        throw std::runtime_error(
            "exportzerocoins include_spent ( denomination )\n"
            "\nExports zerocoin mints that are held by this wallet.dat\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"include_spent\"        (bool, required) Include mints that have already been spent\n"
            "2. \"denomination\"         (integer, optional) Export a specific denomination of zBTCU\n"

            "\nResult:\n"
            "[                   (array of json object)\n"
            "  {\n"
            "    \"id\": \"serial hash\",  (string) the mint's zBTCU serial hash \n"
            "    \"d\": n,         (numeric) the mint's zerocoin denomination \n"
            "    \"p\": \"pubcoin\", (string) The public coin\n"
            "    \"s\": \"serial\",  (string) The secret serial number\n"
            "    \"r\": \"random\",  (string) The secret random number\n"
            "    \"t\": \"txid\",    (string) The txid that the coin was minted in\n"
            "    \"h\": n,         (numeric) The height the tx was added to the blockchain\n"
            "    \"u\": used,      (boolean) Whether the mint has been spent\n"
            "    \"v\": version,   (numeric) The version of the zBTCU\n"
            "    \"k\": \"privkey\"  (string) The zBTCU private key (V2+ zBTCU only)\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n" +
            HelpExampleCli("exportzerocoins", "false 5") + HelpExampleRpc("exportzerocoins", "false 5"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    CWalletDB walletdb(pwalletMain->strWalletFile);

    bool fIncludeSpent = params[0].get_bool();
    libzerocoin::CoinDenomination denomination = libzerocoin::ZQ_ERROR;
    if (params.size() == 2)
        denomination = libzerocoin::IntToZerocoinDenomination(params[1].get_int());

    CzBTCUTracker* zbtcuTracker = pwalletMain->zbtcuTracker.get();
    std::set<CMintMeta> setMints = zbtcuTracker->ListMints(!fIncludeSpent, false, false);

    UniValue jsonList(UniValue::VARR);
    for (const CMintMeta& meta : setMints) {
        if (denomination != libzerocoin::ZQ_ERROR && denomination != meta.denom)
            continue;

        CZerocoinMint mint;
        if (!pwalletMain->GetMint(meta.hashSerial, mint))
            continue;

        UniValue objMint(UniValue::VOBJ);
        objMint.push_back(Pair("id", meta.hashSerial.GetHex()));
        objMint.push_back(Pair("d", mint.GetDenomination()));
        objMint.push_back(Pair("p", mint.GetValue().GetHex()));
        objMint.push_back(Pair("s", mint.GetSerialNumber().GetHex()));
        objMint.push_back(Pair("r", mint.GetRandomness().GetHex()));
        objMint.push_back(Pair("t", mint.GetTxHash().GetHex()));
        objMint.push_back(Pair("h", mint.GetHeight()));
        objMint.push_back(Pair("u", mint.IsUsed()));
        objMint.push_back(Pair("v", mint.GetVersion()));
        if (mint.GetVersion() >= 2) {
            CKey key;
            key.SetPrivKey(mint.GetPrivKey(), true);
            CBTCUSecret CBTCUSecret;
            CBTCUSecret.SetKey(key);
            objMint.push_back(Pair("k", CBTCUSecret.ToString()));
        }
        jsonList.push_back(objMint);
    }

    return jsonList;
}

UniValue importzerocoins(const UniValue& params, bool fHelp)
{
    if(fHelp || params.size() == 0)
        throw std::runtime_error(
            "importzerocoins importdata \n"
            "\n[{\"d\":denomination,\"p\":\"pubcoin_hex\",\"s\":\"serial_hex\",\"r\":\"randomness_hex\",\"t\":\"txid\",\"h\":height, \"u\":used},{\"d\":...}]\n"
            "\nImport zerocoin mints.\n"
            "Adds raw zerocoin mints to the wallet.dat\n"
            "Note it is recommended to use the json export created from the exportzerocoins RPC call\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"importdata\"    (string, required) A json array of json objects containing zerocoin mints\n"

            "\nResult:\n"
            "{\n"
            "  \"added\": n,        (numeric) The quantity of zerocoin mints that were added\n"
            "  \"value\": amount    (numeric) The total zBTCU value of zerocoin mints that were added\n"
            "}\n"

            "\nExamples\n" +
            HelpExampleCli("importzerocoins", "\'[{\"d\":100,\"p\":\"mypubcoin\",\"s\":\"myserial\",\"r\":\"randomness_hex\",\"t\":\"mytxid\",\"h\":104923, \"u\":false},{\"d\":5,...}]\'") +
            HelpExampleRpc("importzerocoins", "[{\"d\":100,\"p\":\"mypubcoin\",\"s\":\"myserial\",\"r\":\"randomness_hex\",\"t\":\"mytxid\",\"h\":104923, \"u\":false},{\"d\":5,...}]"));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    RPCTypeCheck(params, boost::assign::list_of(UniValue::VARR)(UniValue::VOBJ));
    UniValue arrMints = params[0].get_array();
    CWalletDB walletdb(pwalletMain->strWalletFile);

    int count = 0;
    CAmount nValue = 0;
    for (unsigned int idx = 0; idx < arrMints.size(); idx++) {
        const UniValue &val = arrMints[idx];
        const UniValue &o = val.get_obj();

        const UniValue& vDenom = find_value(o, "d");
        if (!vDenom.isNum())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing d key");
        int d = vDenom.get_int();
        if (d < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, d must be positive");

        libzerocoin::CoinDenomination denom = libzerocoin::IntToZerocoinDenomination(d);
        CBigNum bnValue = 0;
        bnValue.SetHex(find_value(o, "p").get_str());
        CBigNum bnSerial = 0;
        bnSerial.SetHex(find_value(o, "s").get_str());
        CBigNum bnRandom = 0;
        bnRandom.SetHex(find_value(o, "r").get_str());
        uint256 txid(find_value(o, "t").get_str());

        int nHeight = find_value(o, "h").get_int();
        if (nHeight < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, h must be positive");

        bool fUsed = find_value(o, "u").get_bool();

        //Assume coin is version 1 unless it has the version actually set
        uint8_t nVersion = 1;
        const UniValue& vVersion = find_value(o, "v");
        if (vVersion.isNum())
            nVersion = static_cast<uint8_t>(vVersion.get_int());

        //Set the privkey if applicable
        CPrivKey privkey;
        if (nVersion >= libzerocoin::PrivateCoin::PUBKEY_VERSION) {
            std::string strPrivkey = find_value(o, "k").get_str();
            CBTCUSecret vchSecret;
            bool fGood = vchSecret.SetString(strPrivkey);
            CKey key = vchSecret.GetKey();
            if (!key.IsValid() && fGood)
                return JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "privkey is not valid");
            privkey = key.GetPrivKey();
        }

        CZerocoinMint mint(denom, bnValue, bnRandom, bnSerial, fUsed, nVersion, &privkey);
        mint.SetTxHash(txid);
        mint.SetHeight(nHeight);
        pwalletMain->zbtcuTracker->Add(mint, true);
        count++;
        nValue += libzerocoin::ZerocoinDenominationToAmount(denom);
    }

    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("added", count));
    ret.push_back(Pair("value", ValueFromAmount(nValue)));
    return ret;
}

UniValue reconsiderzerocoins(const UniValue& params, bool fHelp)
{
    if(fHelp || !params.empty())
        throw std::runtime_error(
            "reconsiderzerocoins\n"
            "\nCheck archived zBTCU list to see if any mints were added to the blockchain.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txid\" : \"xxx\",           (string) the mint's zerocoin denomination \n"
            "    \"denomination\" : amount,  (numeric) the mint's zerocoin denomination\n"
            "    \"pubcoin\" : \"xxx\",        (string) The mint's public identifier\n"
            "    \"height\" : n              (numeric) The height the tx was added to the blockchain\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples\n" +
            HelpExampleCli("reconsiderzerocoins", "") + HelpExampleRpc("reconsiderzerocoins", ""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    EnsureWalletIsUnlocked(true);

    std::list<CZerocoinMint> listMints;
    std::list<CDeterministicMint> listDMints;
    pwalletMain->ReconsiderZerocoins(listMints, listDMints);

    UniValue arrRet(UniValue::VARR);
    for (const CZerocoinMint& mint : listMints) {
        UniValue objMint(UniValue::VOBJ);
        objMint.push_back(Pair("txid", mint.GetTxHash().GetHex()));
        objMint.push_back(Pair("denomination", ValueFromAmount(mint.GetDenominationAsAmount())));
        objMint.push_back(Pair("pubcoin", mint.GetValue().GetHex()));
        objMint.push_back(Pair("height", mint.GetHeight()));
        arrRet.push_back(objMint);
    }
    for (const CDeterministicMint& dMint : listDMints) {
        UniValue objMint(UniValue::VOBJ);
        objMint.push_back(Pair("txid", dMint.GetTxHash().GetHex()));
        objMint.push_back(Pair("denomination", FormatMoney(libzerocoin::ZerocoinDenominationToAmount(dMint.GetDenomination()))));
        objMint.push_back(Pair("pubcoinhash", dMint.GetPubcoinHash().GetHex()));
        objMint.push_back(Pair("height", dMint.GetHeight()));
        arrRet.push_back(objMint);
    }

    return arrRet;
}

UniValue setzbtcuseed(const UniValue& params, bool fHelp)
{
    if(fHelp || params.size() != 1)
        throw std::runtime_error(
            "setzbtcuseed \"seed\"\n"
            "\nSet the wallet's deterministic zbtcu seed to a specific value.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments:\n"
            "1. \"seed\"        (string, required) The deterministic zbtcu seed.\n"

            "\nResult\n"
            "\"success\" : b,  (boolean) Whether the seed was successfully set.\n"

            "\nExamples\n" +
            HelpExampleCli("setzbtcuseed", "63f793e7895dd30d99187b35fbfb314a5f91af0add9e0a4e5877036d1e392dd5") +
            HelpExampleRpc("setzbtcuseed", "63f793e7895dd30d99187b35fbfb314a5f91af0add9e0a4e5877036d1e392dd5"));

    EnsureWalletIsUnlocked();

    uint256 seed;
    seed.SetHex(params[0].get_str());

    CzBTCUWallet* zwallet = pwalletMain->getZWallet();
    bool fSuccess = zwallet->SetMasterSeed(seed, true);
    if (fSuccess)
        zwallet->SyncWithChain();

    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("success", fSuccess));

    return ret;
}

UniValue getzbtcuseed(const UniValue& params, bool fHelp)
{
    if(fHelp || !params.empty())
        throw std::runtime_error(
            "getzbtcuseed\n"
            "\nCheck archived zBTCU list to see if any mints were added to the blockchain.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nResult\n"
            "\"seed\" : s,  (string) The deterministic zBTCU seed.\n"

            "\nExamples\n" +
            HelpExampleCli("getzbtcuseed", "") + HelpExampleRpc("getzbtcuseed", ""));

    EnsureWalletIsUnlocked();

    CzBTCUWallet* zwallet = pwalletMain->getZWallet();
    uint256 seed = zwallet->GetMasterSeed();

    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("seed", seed.GetHex()));

    return ret;
}

UniValue generatemintlist(const UniValue& params, bool fHelp)
{
    if(fHelp || params.size() != 2)
        throw std::runtime_error(
            "generatemintlist\n"
            "\nShow mints that are derived from the deterministic zBTCU seed.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments\n"
            "1. \"count\"  : n,  (numeric) Which sequential zBTCU to start with.\n"
            "2. \"range\"  : n,  (numeric) How many zBTCU to generate.\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"count\": n,          (numeric) Deterministic Count.\n"
            "    \"value\": \"xxx\",    (string) Hex encoded pubcoin value.\n"
            "    \"randomness\": \"xxx\",   (string) Hex encoded randomness.\n"
            "    \"serial\": \"xxx\"        (string) Hex encoded Serial.\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples\n" +
            HelpExampleCli("generatemintlist", "1, 100") + HelpExampleRpc("generatemintlist", "1, 100"));

    EnsureWalletIsUnlocked();

    int nCount = params[0].get_int();
    int nRange = params[1].get_int();
    CzBTCUWallet* zwallet = pwalletMain->zwalletMain;

    UniValue arrRet(UniValue::VARR);
    for (int i = nCount; i < nCount + nRange; i++) {
        libzerocoin::CoinDenomination denom = libzerocoin::CoinDenomination::ZQ_ONE;
        libzerocoin::PrivateCoin coin(Params().Zerocoin_Params(false), denom, false);
        CDeterministicMint dMint;
        zwallet->GenerateMint(i, denom, coin, dMint);
        UniValue obj(UniValue::VOBJ);
        obj.push_back(Pair("count", i));
        obj.push_back(Pair("value", coin.getPublicCoin().getValue().GetHex()));
        obj.push_back(Pair("randomness", coin.getRandomness().GetHex()));
        obj.push_back(Pair("serial", coin.getSerialNumber().GetHex()));
        arrRet.push_back(obj);
    }

    return arrRet;
}

UniValue dzbtcustate(const UniValue& params, bool fHelp) {
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
                "dzbtcustate\n"
                        "\nThe current state of the mintpool of the deterministic zBTCU wallet.\n" +
                HelpRequiringPassphrase() + "\n"

                        "\nExamples\n" +
                HelpExampleCli("mintpoolstatus", "") + HelpExampleRpc("mintpoolstatus", ""));

    CzBTCUWallet* zwallet = pwalletMain->zwalletMain;
    UniValue obj(UniValue::VOBJ);
    int nCount, nCountLastUsed;
    zwallet->GetState(nCount, nCountLastUsed);
    obj.push_back(Pair("dzbtcu_count", nCount));
    obj.push_back(Pair("mintpool_count", nCountLastUsed));

    return obj;
}


void static SearchThread(CzBTCUWallet* zwallet, int nCountStart, int nCountEnd)
{
    LogPrintf("%s: start=%d end=%d\n", __func__, nCountStart, nCountEnd);
    CWalletDB walletDB(pwalletMain->strWalletFile);
    try {
        uint256 seedMaster = zwallet->GetMasterSeed();
        uint256 hashSeed = Hash(seedMaster.begin(), seedMaster.end());
        for(int i = nCountStart; i < nCountEnd; i++) {
            boost::this_thread::interruption_point();
            CDataStream ss(SER_GETHASH, 0);
            ss << seedMaster << i;
            uint512 zerocoinSeed = Hash512(ss.begin(), ss.end());

            CBigNum bnValue;
            CBigNum bnSerial;
            CBigNum bnRandomness;
            CKey key;
            zwallet->SeedToZBTCU(zerocoinSeed, bnValue, bnSerial, bnRandomness, key);

            uint256 hashPubcoin = GetPubCoinHash(bnValue);
            zwallet->AddToMintPool(std::make_pair(hashPubcoin, i), true);
            walletDB.WriteMintPoolPair(hashSeed, hashPubcoin, i);
        }
    } catch (const std::exception& e) {
        LogPrintf("SearchThread() exception");
    } catch (...) {
        LogPrintf("SearchThread() exception");
    }
}

UniValue searchdzbtcu(const UniValue& params, bool fHelp)
{
    if(fHelp || params.size() != 3)
        throw std::runtime_error(
            "searchdzbtcu\n"
            "\nMake an extended search for deterministically generated zBTCU that have not yet been recognized by the wallet.\n" +
            HelpRequiringPassphrase() + "\n"

            "\nArguments\n"
            "1. \"count\"       (numeric) Which sequential zBTCU to start with.\n"
            "2. \"range\"       (numeric) How many zBTCU to generate.\n"
            "3. \"threads\"     (numeric) How many threads should this operation consume.\n"

            "\nExamples\n" +
            HelpExampleCli("searchdzbtcu", "1, 100, 2") + HelpExampleRpc("searchdzbtcu", "1, 100, 2"));

    EnsureWalletIsUnlocked();

    int nCount = params[0].get_int();
    if (nCount < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Count cannot be less than 0");

    int nRange = params[1].get_int();
    if (nRange < 1)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Range has to be at least 1");

    int nThreads = params[2].get_int();

    CzBTCUWallet* zwallet = pwalletMain->zwalletMain;

    boost::thread_group* dzbtcuThreads = new boost::thread_group();
    int nRangePerThread = nRange / nThreads;

    int nPrevThreadEnd = nCount - 1;
    for (int i = 0; i < nThreads; i++) {
        int nStart = nPrevThreadEnd + 1;;
        int nEnd = nStart + nRangePerThread;
        nPrevThreadEnd = nEnd;
        dzbtcuThreads->create_thread(boost::bind(&SearchThread, zwallet, nStart, nEnd));
    }

    dzbtcuThreads->join_all();

    zwallet->RemoveMintsFromPool(pwalletMain->zbtcuTracker->GetSerialHashes());
    zwallet->SyncWithChain(false);

    //todo: better response
    return "done";
}

UniValue spendrawzerocoin(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 6)
        throw std::runtime_error(
            "spendrawzerocoin \"serialHex\" denom \"randomnessHex\" \"priv key\" ( \"address\" \"mintTxId\" )\n"
            "\nCreate and broadcast a TX spending the provided zericoin.\n"

            "\nArguments:\n"
            "1. \"serialHex\"        (string, required) A zerocoin serial number (hex)\n"
            "2. \"randomnessHex\"    (string, required) A zerocoin randomness value (hex)\n"
            "3. denom                (numeric, required) A zerocoin denomination (decimal)\n"
            "4. \"priv key\"         (string, required) The private key associated with this coin (hex)\n"
            "5. \"address\"          (string, optional) BTCU address to spend to. If not specified, "
            "                        or empty string, spend to change address.\n"
            "6. \"mintTxId\"         (string, optional) txid of the transaction containing the mint. If not"
            "                        specified, or empty string, the blockchain will be scanned (could take a while)"

            "\nResult:\n"
                "\"txid\"             (string) The transaction txid in hex\n"

            "\nExamples\n" +
            HelpExampleCli("spendrawzerocoin", "\"f80892e78c30a393ef4ab4d5a9d5a2989de6ebc7b976b241948c7f489ad716a2\" \"a4fd4d7248e6a51f1d877ddd2a4965996154acc6b8de5aa6c83d4775b283b600\" 100 \"xxx\"") +
            HelpExampleRpc("spendrawzerocoin", "\"f80892e78c30a393ef4ab4d5a9d5a2989de6ebc7b976b241948c7f489ad716a2\", \"a4fd4d7248e6a51f1d877ddd2a4965996154acc6b8de5aa6c83d4775b283b600\", 100, \"xxx\""));

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (sporkManager.IsSporkActive(SPORK_16_ZEROCOIN_MAINTENANCE_MODE))
            throw JSONRPCError(RPC_WALLET_ERROR, "zBTCU is currently disabled due to maintenance.");

    CBigNum serial;
    serial.SetHex(params[0].get_str());

    CBigNum randomness;
    randomness.SetHex(params[1].get_str());

    const int denom_int = params[2].get_int();
    libzerocoin::CoinDenomination denom = libzerocoin::IntToZerocoinDenomination(denom_int);

    std::string priv_key_str = params[3].get_str();
    CPrivKey privkey;
    CBTCUSecret vchSecret;
    bool fGood = vchSecret.SetString(priv_key_str);
    CKey key = vchSecret.GetKey();
    if (!key.IsValid() && fGood)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "privkey is not valid");
    privkey = key.GetPrivKey();

    // Create the coin associated with these secrets
    libzerocoin::PrivateCoin coin(Params().Zerocoin_Params(false), denom, serial, randomness);
    coin.setPrivKey(privkey);
    coin.setVersion(libzerocoin::PrivateCoin::CURRENT_VERSION);

    // Create the mint associated with this coin
    CZerocoinMint mint(denom, coin.getPublicCoin().getValue(), randomness, serial, false, CZerocoinMint::CURRENT_VERSION, &privkey);

    std::string address_str = "";
    if (params.size() > 4)
        address_str = params[4].get_str();

    if (params.size() > 5) {
        // update mint txid
        mint.SetTxHash(ParseHashV(params[5], "parameter 5"));
    } else {
        // If the mint tx is not provided, look for it
        const CBigNum& mintValue = mint.GetValue();
        bool found = false;
        {
            CBlockIndex* pindex = chainActive.Tip();
            while (!found && pindex && pindex->nHeight >= Params().Zerocoin_StartHeight()) {
                LogPrintf("%s : Checking block %d...\n", __func__, pindex->nHeight);
                if (pindex->MintedDenomination(denom)) {
                    CBlock block;
                    if (!ReadBlockFromDisk(block, pindex))
                        throw JSONRPCError(RPC_INTERNAL_ERROR, "Unable to read block from disk");
                    std::list<CZerocoinMint> listMints;
                    BlockToZerocoinMintList(block, listMints, true);
                    for (const CZerocoinMint& m : listMints) {
                        if (m.GetValue() == mintValue && m.GetDenomination() == denom) {
                            // mint found. update txid
                            mint.SetTxHash(m.GetTxHash());
                            found = true;
                            break;
                        }
                    }
                }
                pindex = pindex->pprev;
            }
        }
        if (!found)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Mint tx not found");
    }

    std::vector<CZerocoinMint> vMintsSelected = {mint};
    return DoZbtcuSpend(mint.GetDenominationAsAmount(), vMintsSelected, address_str);
}

