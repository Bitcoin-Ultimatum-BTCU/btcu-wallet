// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "contract.h"
#include "univalue.h"
#include "main.h"
#include "coins.h"
#include "chain.h"

#include "libdevcore/ABI.h"

std::unique_ptr<QtumState> globalState;
std::shared_ptr<dev::eth::SealEngineFace> globalSealEngine;
bool fRecordLogOpcodes = false;
bool fIsVMlogFile = false;
bool fGettingValuesDGP = false;

void ContractStateInit()
{
    namespace fs = boost::filesystem;
    const CChainParams& chainparams = Params();
    dev::eth::NoProof::init();
    fs::path qtumStateDir = GetDataDir() / "stateQtum";
    bool fStatus = fs::exists(qtumStateDir);
    const std::string dirQtum(qtumStateDir.string());
    const dev::h256 hashDB(dev::sha3(dev::rlp("")));
    dev::eth::BaseState existsQtumstate = fStatus ? dev::eth::BaseState::PreExisting : dev::eth::BaseState::Empty;
    globalState = std::unique_ptr<QtumState>(new QtumState(dev::u256(0), QtumState::openDB(dirQtum, hashDB, dev::WithExisting::Trust), dirQtum, existsQtumstate));
    auto geni = chainparams.EVMGenesisInfo(dev::eth::Network::qtumMainNetwork);
    dev::eth::ChainParams cp((geni));
    globalSealEngine = std::unique_ptr<dev::eth::SealEngineFace>(cp.createSealEngine());

    if(chainActive.Tip() != nullptr){
        auto hash = uintToh256(chainActive.Tip()->hashStateRoot);
        globalState->setRoot(uintToh256(chainActive.Tip()->hashStateRoot));
        globalState->setRootUTXO(uintToh256(chainActive.Tip()->hashUTXORoot));
    } else {
        globalState->setRoot(dev::sha3(dev::rlp("")));
        globalState->setRootUTXO(uintToh256(chainparams.GenesisBlock().hashUTXORoot));
        globalState->populateFrom(cp.genesisState);
    }

    globalState->db().commit();
    globalState->dbUtxo().commit();
}

void ContractStateShutdown()
{
    globalState.reset();
}

bool CheckOpSender(const CTransaction& tx, const CChainParams& chainparams, int nHeight){
    if(!tx.HasOpSender())
        return true;

    ///TODO:resolve this const. Does it needed after transplantation?
    //if(!(nHeight >= chainparams.GetConsensus().QIP5Height))
    //   return false;

    // Check that the sender address inside the output is only valid for contract outputs
    for (const CTxOut& txout : tx.vout)
    {
        bool hashOpSender = txout.scriptPubKey.HasOpSender();
        if(hashOpSender &&
           !(txout.scriptPubKey.HasOpCreate() ||
             txout.scriptPubKey.HasOpCall()))
        {
            return false;
        }

        // Solve the script that match the sender templates
        if(hashOpSender && !ExtractSenderData(txout.scriptPubKey, nullptr, nullptr))
            return false;
    }

    return true;
}

bool CheckSenderScript(const CCoinsViewCache& view, const CTransaction& tx){
    // Check for the sender that pays the coins
    CScript script = view.AccessCoins(tx.vin[0].prevout.hash)->vout[tx.vin[0].prevout.n].scriptPubKey;
    if(!script.IsPayToPubkeyHash() && !script.IsPayToPubkey()){
        return false;
    }

    // Check for additional VM sender
    if(!tx.HasOpSender())
        return true;

    // Check for the VM sender that is encoded into the output
    for (const CTxOut& txout : tx.vout)
    {
        if(txout.scriptPubKey.HasOpSender())
        {
            // Extract the sender data
            CScript senderPubKey, senderSig;
            if(!ExtractSenderData(txout.scriptPubKey, &senderPubKey, &senderSig))
                return false;

            // Check that the pub key is valid sender that can be used in the VM
            if(!senderPubKey.IsPayToPubkeyHash() && !senderPubKey.IsPayToPubkey())
                return false;

            // Get the signature stack
            std::vector <std::vector<unsigned char> > stack;
            if (!EvalScript(stack, senderSig, SCRIPT_VERIFY_NONE, BaseSignatureChecker()))
                return false;

            // Check that the signature script contains only signature and public key (2 items)
            if(stack.size() != STANDARD_SENDER_STACK_ITEMS)
                return false;

            // Check that the items size is no more than 80 bytes
            for(size_t i=0; i < stack.size(); i++)
            {
                if(stack[i].size() > MAX_STANDARD_SENDER_STACK_ITEM_SIZE)
                    return false;
            }
        }
    }

    return true;
}

std::vector<ResultExecute> CallContract(const dev::Address& addrContract, std::vector<unsigned char> opcode, const dev::Address& sender, uint64_t gasLimit){
    CBlock block;
    CMutableTransaction tx;

    //CBlockIndex* pblockindex = ::BlockIndex()[::ChainActive().Tip()->GetBlockHash()];
    CBlockIndex* pblockindex = mapBlockIndex[chainActive.Tip()->GetBlockHash()];

    ReadBlockFromDisk(block, pblockindex);
    block.nTime = GetAdjustedTime();

    if(block.IsProofOfStake())
        block.vtx.erase(block.vtx.begin()+2,block.vtx.end());
    else
        block.vtx.erase(block.vtx.begin()+1,block.vtx.end());

    QtumDGP qtumDGP(globalState.get(), fGettingValuesDGP);
    uint64_t blockGasLimit = qtumDGP.getBlockGasLimit(chainActive.Tip()->nHeight + 1);

    if(gasLimit == 0){
        gasLimit = blockGasLimit - 1;
    }
    dev::Address senderAddress = sender == dev::Address() ? dev::Address("ffffffffffffffffffffffffffffffffffffffff") : sender;
    tx.vout.push_back(CTxOut(0, CScript() << OP_DUP << OP_HASH160 << senderAddress.asBytes() << OP_EQUALVERIFY << OP_CHECKSIG));
    //block.vtx.push_back(MakeTransactionRef(CTransaction(tx)));
    block.vtx.push_back(CTransaction(tx));

    QtumTransaction callTransaction(0, 1, dev::u256(gasLimit), addrContract, opcode, dev::u256(0));
    callTransaction.forceSender(senderAddress);
    callTransaction.setVersion(VersionVM::GetEVMDefault());


    ByteCodeExec exec(block, std::vector<QtumTransaction>(1, callTransaction), blockGasLimit, pblockindex);
    exec.performByteCode(dev::eth::Permanence::Reverted);
    return exec.getResult();
}

bool CheckMinGasPrice(std::vector<EthTransactionParams>& etps, const uint64_t& minGasPrice){
    for(EthTransactionParams& etp : etps){
        if(etp.gasPrice < dev::u256(minGasPrice))
            return false;
    }
    return true;
}

/*
bool CheckReward(const CBlock& block, CValidationState& state, int nHeight, const CChainParams& consensusParams, CAmount nFees, CAmount gasRefunds, CAmount nActualStakeReward, const std::vector<CTxOut>& vouts)
{
   size_t offset = block.IsProofOfStake() ? 1 : 0;
   std::vector<CTxOut> vTempVouts=block.vtx[offset].vout;
   std::vector<CTxOut>::iterator it;
   for(size_t i = 0; i < vouts.size(); i++){
      it=std::find(vTempVouts.begin(), vTempVouts.end(), vouts[i]);
      if(it==vTempVouts.end()){
         return state.Invalid(error("CheckReward(): Gas refund missing"), REJECT_INVALID, "bad-gas-refund-missing");
      }else{
         vTempVouts.erase(it);
      }
   }

   // Check block reward
   if (block.IsProofOfWork())
   {
      // Check proof-of-work reward
      //CAmount blockReward = nFees + GetBlockSubsidy(nHeight, consensusParams);
      CAmount blockReward = nFees + GetBlockValue(nHeight);
      if (block.vtx[offset].GetValueOut() > blockReward)
         return state.Invalid(error("CheckReward(): coinbase pays too much (actual=%d vs limit=%d)",
                                                                        block.vtx[offset].GetValueOut(), blockReward), REJECT_INVALID, "bad-cb-amount");
   }
   else
   {
      // Check full reward
      //CAmount blockReward = nFees + GetBlockSubsidy(nHeight, consensusParams);
      CAmount blockReward = nFees + GetBlockValue(nHeight);
      if (nActualStakeReward > blockReward)
         return state.Invalid(error("CheckReward(): coinstake pays too much (actual=%d vs limit=%d)",
                                                                        nActualStakeReward, blockReward), REJECT_INVALID, "bad-cs-amount");

      // The first proof-of-stake blocks get full reward, the rest of them are split between recipients
      int rewardRecipients = 1;
      int nPrevHeight = nHeight -1;
      if(nPrevHeight >= consensusParams.LAST_POW_BLOCK())
         rewardRecipients = consensusParams.MPoSRewardRecipients();

      // Check reward recipients number
      if(rewardRecipients < 1)
         return error("CheckReward(): invalid reward recipients");

      //if only 1 then no MPoS logic required
      if(rewardRecipients == 1){
         return true;
      }
      if(blockReward < gasRefunds){
         return state.Invalid(error("CheckReward(): Block Reward is less than total gas refunds"), REJECT_INVALID, "bad-cs-gas-greater-than-reward");

      }
      CAmount splitReward = (blockReward - gasRefunds) / rewardRecipients;

      // Generate the list of script recipients including all of their parameters
      std::vector<CScript> mposScriptList;
      if(!GetMPoSOutputScripts(mposScriptList, nPrevHeight, consensusParams))
         return error("CheckReward(): cannot create the list of MPoS output scripts");

      for(size_t i = 0; i < mposScriptList.size(); i++){
         it=std::find(vTempVouts.begin(), vTempVouts.end(), CTxOut(splitReward,mposScriptList[i]));
         if(it==vTempVouts.end()){
            return state.Invalid(error("CheckReward(): An MPoS participant was not properly paid"), REJECT_INVALID, "bad-cs-mpos-missing");
         }else{
            vTempVouts.erase(it);
         }
      }

      vTempVouts.clear();
   }

   return true;
}
*/
valtype GetSenderAddress(const CTransaction& tx, const CCoinsViewCache* coinsView, const std::vector<CTransactionRef>* blockTxs, int nOut = -1){
    CScript script;
    bool scriptFilled=false; //can't use script.empty() because an empty script is technically valid

    // Try get the sender script from the output script
    if(nOut > -1)
        scriptFilled = ExtractSenderData(tx.vout[nOut].scriptPubKey, &script, nullptr);

    // Check the current (or in-progress) block for zero-confirmation change spending that won't yet be in txindex
    if(!scriptFilled && blockTxs){
        for(auto btx : *blockTxs){
            if(btx->GetHash() == tx.vin[0].prevout.hash){
                script = btx->vout[tx.vin[0].prevout.n].scriptPubKey;
                scriptFilled=true;
                break;
            }
        }
    }
    if(!scriptFilled && coinsView){
        script = coinsView->AccessCoins(tx.vin[0].prevout.hash)->vout[tx.vin[0].prevout.n].scriptPubKey;
        //script = coinsView->AccessCoin(tx.vin[0].prevout).out.scriptPubKey;
        scriptFilled = true;
    }
    if(!scriptFilled)
    {
        //CTransactionRef txPrevout;
        CTransaction txPrevout;
        uint256 hashBlock;
        if(GetTransaction(tx.vin[0].prevout.hash, txPrevout, hashBlock, true, nullptr)){
            script = txPrevout.vout[tx.vin[0].prevout.n].scriptPubKey;
        } else {
            LogPrintf("Error fetching transaction details of tx %s. This will probably cause more errors", tx.vin[0].prevout.hash.ToString());
            return valtype();
        }
    }

    CTxDestination addressBit;
    txnouttype txType=TX_NONSTANDARD;
    if(ExtractDestination(script, addressBit, &txType)){
        if ((txType == TX_PUBKEY || txType == TX_PUBKEYHASH) &&
            addressBit.type() == typeid(CKeyID)){
            CKeyID senderAddress(boost::get<CKeyID>(addressBit));
            return valtype(senderAddress.begin(), senderAddress.end());
        }
    }
    //prevout is not a standard transaction format, so just return 0
    return valtype();
}

UniValue vmLogToJSON(const ResultExecute& execRes, const CTransaction& tx, const CBlock& block){
    UniValue result(UniValue::VOBJ);
    if(tx != CTransaction())
        result.pushKV("txid", tx.GetHash().GetHex());
    result.pushKV("address", execRes.execRes.newAddress.hex());
    if(block.GetHash() != CBlock().GetHash()){
        result.pushKV("time", block.GetBlockTime());
        result.pushKV("blockhash", block.GetHash().GetHex());
        result.pushKV("blockheight", chainActive.Tip()->nHeight + 1);
    } else {
        result.pushKV("time", GetAdjustedTime());
        result.pushKV("blockheight", chainActive.Tip()->nHeight);
    }
    UniValue logEntries(UniValue::VARR);
    dev::eth::LogEntries logs = execRes.txRec.log();
    for(dev::eth::LogEntry log : logs){
        UniValue logEntrie(UniValue::VOBJ);
        logEntrie.pushKV("address", log.address.hex());
        UniValue topics(UniValue::VARR);
        for(dev::h256 l : log.topics){
            UniValue topicPair(UniValue::VOBJ);
            topicPair.pushKV("raw", l.hex());
            topics.push_back(topicPair);
            //TODO add "pretty" field for human readable data
        }
        UniValue dataPair(UniValue::VOBJ);
        dataPair.pushKV("raw", HexStr(log.data));
        logEntrie.pushKV("data", dataPair);
        logEntrie.pushKV("topics", topics);
        logEntries.push_back(logEntrie);
    }
    result.pushKV("entries", logEntries);
    return result;
}

void writeVMlog(const std::vector<ResultExecute>& res, const CTransaction& tx, const CBlock& block){
    boost::filesystem::path qtumDir = GetDataDir() / "vmExecLogs.json";
    std::stringstream ss;
    if(fIsVMlogFile){
        ss << ",";
    } else {
        std::ofstream file(qtumDir.string(), std::ios::out | std::ios::app);
        file << "{\"logs\":[]}";
        file.close();
    }

    for(size_t i = 0; i < res.size(); i++){
        ss << vmLogToJSON(res[i], tx, block).write();
        if(i != res.size() - 1){
            ss << ",";
        } else {
            ss << "]}";
        }
    }

    std::ofstream file(qtumDir.string(), std::ios::in | std::ios::out);
    file.seekp(-2, std::ios::end);
    file << ss.str();
    file.close();
    fIsVMlogFile = true;
}

LastHashes::LastHashes()
{}

void LastHashes::set(const CBlockIndex *tip)
{
    clear();

    m_lastHashes.resize(256);
    for(int i=0;i<256;i++){
        if(!tip)
            break;
        m_lastHashes[i]= uintToh256(*tip->phashBlock);
        tip = tip->pprev;
    }
}

dev::h256s LastHashes::precedingHashes(const dev::h256 &) const
{
    return m_lastHashes;
}

void LastHashes::clear()
{
    m_lastHashes.clear();
}

bool ByteCodeExec::performByteCode(dev::eth::Permanence type){
    for(QtumTransaction& tx : txs){
        //validate VM version
        if(tx.getVersion().toRaw() != VersionVM::GetEVMDefault().toRaw()){
            return false;
        }
        dev::eth::EnvInfo envInfo(BuildEVMEnvironment());
        if(!tx.isCreation() && !globalState->addressInUse(tx.receiveAddress())){
            dev::eth::ExecutionResult execRes;
            execRes.excepted = dev::eth::TransactionException::Unknown;
            result.push_back(ResultExecute{execRes, QtumTransactionReceipt(dev::h256(), dev::h256(), dev::u256(), dev::eth::LogEntries()), CTransaction()});
            continue;
        }
        result.push_back(globalState->execute(envInfo, *globalSealEngine.get(), tx, type, OnOpFunc()));
    }
    globalState->db().commit();
    globalState->dbUtxo().commit();
    globalSealEngine.get()->deleteAddresses.clear();
    return true;
}

bool ByteCodeExec::processingResults(ByteCodeExecResult& resultBCE){
    //const CChainParams& consensusParams = Params();
    for(size_t i = 0; i < result.size(); i++){
        uint64_t gasUsed = (uint64_t) result[i].execRes.gasUsed;

        if(result[i].execRes.excepted != dev::eth::TransactionException::None){
            // refund coins sent to the contract to the sender
            if(txs[i].value() > 0){
                CMutableTransaction tx;
                tx.vin.push_back(CTxIn(h256Touint(txs[i].getHashWith()), txs[i].getNVout(), CScript() << OP_SPEND));
                CScript script(CScript() << OP_DUP << OP_HASH160 << txs[i].sender().asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
                tx.vout.push_back(CTxOut(CAmount(txs[i].value()), script));
                resultBCE.valueTransfers.push_back(CTransaction(tx));
            }
            if(!(/*chainActive.Height() >= consensusParams.QIP7Height && */result[i].execRes.excepted == dev::eth::TransactionException::RevertInstruction)){
                resultBCE.usedGas += gasUsed;
            }
        }

        if(result[i].execRes.excepted == dev::eth::TransactionException::None || (/*chainActive.Height() >= consensusParams.QIP7Height && */result[i].execRes.excepted == dev::eth::TransactionException::RevertInstruction)){
            if(txs[i].gas() > UINT64_MAX ||
               result[i].execRes.gasUsed > UINT64_MAX ||
               txs[i].gasPrice() > UINT64_MAX){
                return false;
            }
            uint64_t gas = (uint64_t) txs[i].gas();
            uint64_t gasPrice = (uint64_t) txs[i].gasPrice();

            resultBCE.usedGas += gasUsed;
            int64_t amount = (gas - gasUsed) * gasPrice;
            if(amount < 0){
                return false;
            }
            if(amount > 0){
                // Refund the rest of the amount to the sender that provide the coins for the contract
                CScript script(CScript() << OP_DUP << OP_HASH160 << txs[i].getRefundSender().asBytes() << OP_EQUALVERIFY << OP_CHECKSIG);
                resultBCE.refundOutputs.push_back(CTxOut(amount, script));
                resultBCE.refundSender += amount;
            }
        }

        if(result[i].tx != CTransaction()){
            resultBCE.valueTransfers.push_back(result[i].tx);
        }
    }
    return true;
}

dev::eth::EnvInfo ByteCodeExec::BuildEVMEnvironment(){
    CBlockIndex* tip = pindex;
    dev::eth::BlockHeader header;
    header.setNumber(tip->nHeight + 1);
    header.setTimestamp(block.nTime);
    header.setDifficulty(dev::u256(block.nBits));
    header.setGasLimit(blockGasLimit);

    lastHashes.set(tip);

    if(block.IsProofOfStake()){
        header.setAuthor(EthAddrFromScript(block.vtx[1].vout[1].scriptPubKey));
    }else {
        header.setAuthor(EthAddrFromScript(block.vtx[0].vout[0].scriptPubKey));
    }
    dev::u256 gasUsed;
    dev::eth::EnvInfo env(header, lastHashes, gasUsed);
    return env;
}

dev::Address ByteCodeExec::EthAddrFromScript(const CScript& script){
    CTxDestination addressBit;
    txnouttype txType=TX_NONSTANDARD;
    if(ExtractDestination(script, addressBit, &txType)){
        if ((txType == TX_PUBKEY || txType == TX_PUBKEYHASH) &&
            addressBit.type() == typeid(CKeyID)){
            CKeyID addressKey(boost::get<CKeyID>(addressBit));
            std::vector<unsigned char> addr(addressKey.begin(), addressKey.end());
            return dev::Address(addr);
        }
    }
    //if not standard or not a pubkey or pubkeyhash output, then return 0
    return dev::Address();
}

bool QtumTxConverter::extractionQtumTransactions(ExtractQtumTX& qtumtx){
    // Get the address of the sender that pay the coins for the contract transactions
    refundSender = dev::Address(GetSenderAddress(txBit, view, blockTransactions));

    // Extract contract transactions
    std::vector<QtumTransaction> resultTX;
    std::vector<EthTransactionParams> resultETP;
    for(size_t i = 0; i < txBit.vout.size(); i++){
        if(txBit.vout[i].scriptPubKey.HasOpCreate() || txBit.vout[i].scriptPubKey.HasOpCall()){
            if(receiveStack(txBit.vout[i].scriptPubKey)){
                EthTransactionParams params;
                if(parseEthTXParams(params)){
                    resultTX.push_back(createEthTX(params, i));
                    resultETP.push_back(params);
                }else{
                    return false;
                }
            }else{
                return false;
            }
        }
    }
    qtumtx = std::make_pair(resultTX, resultETP);
    return true;
}

bool QtumTxConverter::receiveStack(const CScript& scriptPubKey){
    sender = false;
    // FIX: hardcoded flag value
    EvalScript(stack, scriptPubKey, 1610612736, BaseSignatureChecker(), nullptr);
    if (stack.empty())
        return false;

    std::vector<unsigned char> vec(stack.back().begin(),stack.back().end());

    opcode = (opcodetype)(*vec.begin());


    stack.pop_back();
    sender = scriptPubKey.HasOpSender();

    if((opcode == OP_CREATE && stack.size() < correctedStackSize(4)) || (opcode == OP_CALL && stack.size() < correctedStackSize(5))){
        stack.clear();
        sender = false;
        return false;
    }

    return true;
}

bool QtumTxConverter::parseEthTXParams(EthTransactionParams& params){
    try{
        dev::Address receiveAddress;
        valtype vecAddr;
        if (opcode == OP_CALL)
        {
            vecAddr = stack.back();
            stack.pop_back();
            receiveAddress = dev::Address(vecAddr);
        }
        if(stack.size() < correctedStackSize(4))
            return false;

        if(stack.back().size() < 1){
            return false;
        }
        valtype code(stack.back());
        stack.pop_back();
        uint64_t gasPrice = CScriptNum::vch_to_uint64(stack.back());
        stack.pop_back();
        uint64_t gasLimit = CScriptNum::vch_to_uint64(stack.back());
        stack.pop_back();
        if(gasPrice > INT64_MAX || gasLimit > INT64_MAX){
            return false;
        }
        //we track this as CAmount in some places, which is an int64_t, so constrain to INT64_MAX
        if(gasPrice !=0 && gasLimit > INT64_MAX / gasPrice){
            //overflows past 64bits, reject this tx
            return false;
        }
        if(stack.back().size() > 4){
            return false;
        }
        VersionVM version = VersionVM::fromRaw((uint32_t)CScriptNum::vch_to_uint64(stack.back()));
        stack.pop_back();
        params.version = version;
        params.gasPrice = dev::u256(gasPrice);
        params.receiveAddress = receiveAddress;
        params.code = code;
        params.gasLimit = dev::u256(gasLimit);
        return true;
    }
    catch(const scriptnum_error& err){
        LogPrintf("Incorrect parameters to VM.");
        return false;
    }
}

QtumTransaction QtumTxConverter::createEthTX(const EthTransactionParams& etp, uint32_t nOut){
    QtumTransaction txEth;
    if (etp.receiveAddress == dev::Address() && opcode != OP_CALL){
        txEth = QtumTransaction(txBit.vout[nOut].nValue, etp.gasPrice, etp.gasLimit, etp.code, dev::u256(0));
    }
    else{
        txEth = QtumTransaction(txBit.vout[nOut].nValue, etp.gasPrice, etp.gasLimit, etp.receiveAddress, etp.code, dev::u256(0));
    }
    dev::Address sender(GetSenderAddress(txBit, view, blockTransactions, (int)nOut));
    txEth.forceSender(sender);
    txEth.setHashWith(uintToh256(txBit.GetHash()));
    txEth.setNVout(nOut);
    txEth.setVersion(etp.version);
    txEth.setRefundSender(refundSender);

    return txEth;
}
size_t QtumTxConverter::correctedStackSize(size_t size){
    // OP_SENDER add 3 more parameters in stack besides those for OP_CREATE or OP_CALL
    return sender ? size + 3 : size;
}

std::string exceptedMessage(const dev::eth::TransactionException& excepted, const dev::bytes& output)
{
    std::string message;
    try
    {
        // Process the revert message from the output
        if(excepted == dev::eth::TransactionException::RevertInstruction)
        {
            // Get function: Error(string)
            dev::bytesConstRef oRawData(&output);
            dev::bytes errorFunc = oRawData.cropped(0, 4).toBytes();
            if(dev::toHex(errorFunc) == "08c379a0")
            {
                dev::bytesConstRef oData = oRawData.cropped(4);
                message = dev::eth::ABIDeserialiser<std::string>::deserialise(oData);
            }
        }
    }
    catch(...)
    {}

    return message;
}