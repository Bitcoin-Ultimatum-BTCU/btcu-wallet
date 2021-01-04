// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCU_CONTRACT_H
#define BTCU_CONTRACT_H

#include "libethereum/ChainParams.h"
#include "libethereum/LastBlockHashesFace.h"
#include "libethashseal/Ethash.h"
#include "libethashseal/GenesisInfo.h"

#include "qtum/qtumstate.h"
#include "qtum/qtumDGP.h"

#include "script/interpreter.h"

class CCoinsViewCache;
class CBlockIndex;

extern std::unique_ptr<QtumState> globalState;
extern std::shared_ptr<dev::eth::SealEngineFace> globalSealEngine;
extern bool fRecordLogOpcodes;
extern bool fIsVMlogFile;
extern bool fGettingValuesDGP;

struct EthTransactionParams;
using valtype = std::vector<unsigned char>;
using ExtractQtumTX = std::pair<std::vector<QtumTransaction>, std::vector<EthTransactionParams>>;


void ContractStateInit();
void ContractStateShutdown();

//unsigned int GetContractScriptFlags(int nHeight, const CChainParams& consensusparams);

std::vector<ResultExecute> CallContract(const dev::Address& addrContract, std::vector<unsigned char> opcode, const dev::Address& sender = dev::Address(), uint64_t gasLimit=0);

//bool CheckOpSender(const CTransaction& tx, const CChainParams& chainparams, int nHeight);
//
//bool CheckSenderScript(const CCoinsViewCache& view, const CTransaction& tx);
//
//bool CheckMinGasPrice(std::vector<EthTransactionParams>& etps, const uint64_t& minGasPrice);

void writeVMlog(const std::vector<ResultExecute>& res, const CTransaction& tx = CTransaction(), const CBlock& block = CBlock());

std::string exceptedMessage(const dev::eth::TransactionException& excepted, const dev::bytes& output);

struct EthTransactionParams{
    VersionVM version;
    dev::u256 gasLimit;
    dev::u256 gasPrice;
    valtype code;
    dev::Address receiveAddress;

    bool operator!=(EthTransactionParams etp){
        if(this->version.toRaw() != etp.version.toRaw() || this->gasLimit != etp.gasLimit ||
           this->gasPrice != etp.gasPrice || this->code != etp.code ||
           this->receiveAddress != etp.receiveAddress)
            return true;
        return false;
    }
};

struct ByteCodeExecResult{
    uint64_t usedGas = 0;
    CAmount refundSender = 0;
    std::vector<CTxOut> refundOutputs;
    std::vector<CTransaction> valueTransfers;
};

class QtumTxConverter{

public:

    QtumTxConverter(CTransaction tx, CCoinsViewCache* v = NULL, const std::vector<CTransactionRef>* blockTxs = NULL, unsigned int flags = SCRIPT_EXEC_BYTE_CODE) : txBit(tx), view(v), blockTransactions(blockTxs), sender(false), nFlags(flags){}

    bool extractionQtumTransactions(ExtractQtumTX& qtumTx);

private:

    bool receiveStack(const CScript& scriptPubKey);

    bool parseEthTXParams(EthTransactionParams& params);

    QtumTransaction createEthTX(const EthTransactionParams& etp, const uint32_t nOut);

    size_t correctedStackSize(size_t size);

    const CTransaction txBit;
    const CCoinsViewCache* view;
    std::vector<valtype> stack;
    opcodetype opcode;
    const std::vector<CTransactionRef> *blockTransactions;
    bool sender;
    dev::Address refundSender;
    unsigned int nFlags;
};

class LastHashes: public dev::eth::LastBlockHashesFace
{
public:
    explicit LastHashes();

    void set(CBlockIndex const* tip);

    dev::h256s precedingHashes(dev::h256 const&) const;

    void clear();

private:
    dev::h256s m_lastHashes;
};

class ByteCodeExec {

public:

    ByteCodeExec(const CBlock& _block, std::vector<QtumTransaction> _txs, const uint64_t _blockGasLimit, CBlockIndex* _pindex) : txs(_txs), block(_block), blockGasLimit(_blockGasLimit), pindex(_pindex) {}

    bool performByteCode(dev::eth::Permanence type = dev::eth::Permanence::Committed);

    bool processingResults(ByteCodeExecResult& result);

    std::vector<ResultExecute>& getResult(){ return result; }

private:

    dev::eth::EnvInfo BuildEVMEnvironment();

    dev::Address EthAddrFromScript(const CScript& scriptIn);

    std::vector<QtumTransaction> txs;

    std::vector<ResultExecute> result;

    const CBlock& block;

    const uint64_t blockGasLimit;

    CBlockIndex* pindex;

    LastHashes lastHashes;
};

#endif // BBTCU_CONTRACT_H