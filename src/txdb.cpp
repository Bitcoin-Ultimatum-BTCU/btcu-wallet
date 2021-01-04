// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2016-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"

#include "main.h"
#include "pow.h"
#include "uint256.h"
#include "base58.h"
#include "guiinterface.h"

#include <stdint.h>

#include <boost/thread.hpp>

static constexpr char cBTCU = 'c';
static constexpr char cBitcoin = 'C';

bool ShutdownRequested();

void static BatchWriteCoins(CLevelDBBatch& batch, const uint256& hash, const CCoins& coins)
{
    if (coins.IsPruned())
        batch.Erase(std::make_pair(cBTCU, hash));
    else
        batch.Write(std::make_pair(cBTCU, hash), coins);
}

void static BatchWriteHashBestChain(CLevelDBBatch& batch, const uint256& hash)
{
    batch.Write('B', hash);
}


bool CCoinsViewDBIterator::GetTrxHash(uint256& trxHash, bool fThrow) const {
    std::pair<char, uint256> key;
    if (pCursor->GetKey(key, fThrow)) {
        trxHash = key.second;
        return true;
    }
    return false;
}

bool CCoinsViewDBIterator::GetCoins(CCoins& coins, bool fThrow) const {
    return pCursor->GetValue(coins, true);
}

bool CCoinsViewDBIterator::Valid() const {
    char chType;
    return pCursor->Valid() && pCursor->GetKey(chType, false) && (chType == cBTCU);
}

void CCoinsViewDBIterator::Next() const {
    pCursor->Next();
}

CCoinsViewDB::CCoinsViewDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "chainstate", nCacheSize, fMemory, fWipe, true)
{
}

bool CCoinsViewDB::GetCoins(const uint256& txid, CCoins& coins) const
{
    return db.Read(std::make_pair(cBTCU, txid), coins);
}

bool CCoinsViewDB::HaveCoins(const uint256& txid) const
{
    return db.Exists(std::make_pair(cBTCU, txid));
}

uint256 CCoinsViewDB::GetBestBlock() const
{
    uint256 hashBestChain;
    if (!db.Read('B', hashBestChain))
        return uint256(0);
    return hashBestChain;
}

bool CCoinsViewDB::BatchWrite(CCoinsMap& mapCoins, const uint256& hashBlock)
{
    CLevelDBBatch batch(db);
    size_t count = 0;
    size_t changed = 0;
    for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end();) {
        if (it->second.flags & CCoinsCacheEntry::DIRTY) {
            BatchWriteCoins(batch, it->first, it->second.coins);
            changed++;
        }
        count++;
        CCoinsMap::iterator itOld = it++;
        mapCoins.erase(itOld);
    }
    if (hashBlock != uint256(0))
        BatchWriteHashBestChain(batch, hashBlock);

    LogPrint("coindb", "Committing %u changed transactions (out of %u) to coin database...\n", (unsigned int)changed, (unsigned int)count);
    return db.WriteBatch(batch);
}

CBlockTreeDB::CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe) : CLevelDBWrapper(GetDataDir() / "blocks" / "index", nCacheSize, fMemory, fWipe)
{
}

bool CBlockTreeDB::WriteBlockIndex(const CDiskBlockIndex& blockindex)
{
    return Write(std::make_pair('b', blockindex.GetBlockHash()), blockindex);
}

bool CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo& info)
{
    return Read(std::make_pair('f', nFile), info);
}

bool CBlockTreeDB::WriteReindexing(bool fReindexing)
{
    if (fReindexing)
        return Write('R', '1');
    else
        return Erase('R');
}

bool CBlockTreeDB::ReadReindexing(bool& fReindexing)
{
    fReindexing = Exists('R');
    return true;
}

bool CBlockTreeDB::ReadLastBlockFile(int& nFile)
{
    return Read('l', nFile);
}

bool CCoinsViewDB::GetStats(CCoinsStats& stats) const
{
    /* It seems that there are no "const iterators" for LevelDB.  Since we
       only need read operations on it, use a const-cast to get around
       that restriction.  */
    auto pcursor = db.NewIterator();
    pcursor->Seek(std::make_pair(cBTCU, uint256()));

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    stats.hashBlock = GetBestBlock();
    ss << stats.hashBlock;
    CAmount nTotalAmount = 0;
    for (; pcursor->Valid(); pcursor->Next()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pcursor->GetKey();
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);
            char chType;
            ssKey >> chType;
            if (chType != cBTCU)
                break;

            CCoins coins;
            pcursor->GetValue(coins, true);
            uint256 txhash;
            ssKey >> txhash;
            ss << txhash;
            ss << VARINT(coins.nVersion);
            ss << (coins.fCoinBase ? 'c' : 'n');
            ss << VARINT(coins.nHeight);
            stats.nTransactions++;
            for (unsigned int i = 0; i < coins.vout.size(); i++) {
                const CTxOut& out = coins.vout[i];
                if (!out.IsNull()) {
                    stats.nTransactionOutputs++;
                    ss << VARINT(i + 1);
                    ss << out;
                    nTotalAmount += out.nValue;
                }
            }
            stats.nSerializedSize += 32 + pcursor->GetValueSize();
            ss << VARINT(0);
        } catch (const std::exception& e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    stats.nHeight = mapBlockIndex.find(GetBestBlock())->second->nHeight;
    stats.hashSerialized = ss.GetHash();
    stats.nTotalAmount = nTotalAmount;
    return true;
}

std::unique_ptr<CCoinsViewIterator> CCoinsViewDB::SeekToFirst() const {
    auto pCursor = db.NewIterator();
    pCursor->Seek(std::make_pair(cBTCU, uint256()));
    return std::unique_ptr<CCoinsViewIterator>(new CCoinsViewDBIterator(std::move(pCursor)));
}

namespace {
    /**
     * A Bitcoin UTXO key.
     */
    struct CoinKey {
        char type = cBitcoin;
        uint256 trxHash = uint256();
        int n = 0;

        ADD_SERIALIZE_METHODS;

        template <typename Stream, typename Operation>
        inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
            READWRITE(this->type);
            READWRITE(this->trxHash);
            READWRITE(VARINT(this->n));
        }
    };

    /**
     * A Bitcoin UTXO entry.
     *
     * Serialized format:
     * - VARINT((coinbase ? 1 : 0) | (height << 1))
     * - the non-spent CTxOut (via TxOutCompression)
     */
    class Coin
    {
    public:
        //! unspent transaction output
        CTxOut out;

        //! whether containing transaction was a coinbase
        unsigned int fCoinBase : 1;

        //! at which height this containing transaction was included in the active block chain
        uint32_t nHeight : 31;

        //! construct a Coin from a CTxOut and height/coinbase information.
        Coin(CTxOut&& outIn, int nHeightIn, bool fCoinBaseIn) : out(std::move(outIn)), fCoinBase(fCoinBaseIn), nHeight(nHeightIn) {}
        Coin(const CTxOut& outIn, int nHeightIn, bool fCoinBaseIn) : out(outIn), fCoinBase(fCoinBaseIn),nHeight(nHeightIn) {}

        void Clear() {
            out.SetNull();
            fCoinBase = false;
            nHeight = 0;
        }

        //! empty constructor
        Coin() : fCoinBase(false), nHeight(0) { }

        bool IsCoinBase() const {
            return fCoinBase;
        }

        template <typename Stream>
        void Serialize(Stream& s, int nType, int nVersion) const {
            assert(!IsSpent());
            uint32_t code = nHeight * uint32_t{2} + fCoinBase;
            ::Serialize(s, VARINT(code), nType, nVersion);
            //::Serialize(s, Using<TxOutCompression>(out));
            ::Serialize(s, CTxOutCompressor(REF(out)), nType, nVersion);

        }

        template <typename Stream>
        void Unserialize(Stream& s, int nType, int nVersion) {
            uint32_t code = 0;
            ::Unserialize(s, VARINT(code), nType, nVersion);
            nHeight = code >> 1;
            fCoinBase = code & 1;
            //::Unserialize(s, Using<TxOutCompression>(out));
            ::Unserialize(s, REF(CTxOutCompressor(out)), nType, nVersion);
        }

        bool IsSpent() const {
            return out.IsNull();
        }

    };
}

/** Upgrade the database from bitcoin format to btcu format. */
bool CCoinsViewDB::Upgrade(const uint256& hashBestBlock) {

    auto pCursor = db.NewIterator();
    pCursor->Seek(CoinKey());
    if (!pCursor->Valid()) {
        return true;
    }

    char chType;
    if (!pCursor->GetKey(chType, false) || chType != cBitcoin) {
        return true;
    }

    int64_t count = 0;
    size_t batch_size = 1 << 24;
    CLevelDBBatch batch(db);
    int reportDone = 0;
    CCoins btcu_coins;
    Coin bitcoin_coin;
    std::pair<char, uint256> btcu_key;
    CoinKey key, prev_key, range_key;

    LogPrintf("Upgrading utxo-set database...\n");
    LogPrintf("[0%%]..."); /* Continued */
    uiInterface.ShowProgress(_("Upgrading UTXO database"), 0);

    btcu_key.first = cBTCU;

    for (; pCursor->Valid(); pCursor->Next()) {
        if (ShutdownRequested()) {
            LogPrintf("[CANCELED].\n");
            return false;
        }
        boost::this_thread::interruption_point();
        try {
            if (!pCursor->GetKey(key, false) || key.type != cBitcoin) {
                continue;
            }

            if (count++ % 256 == 0) {
                uint32_t high = 0x100 * *key.trxHash.begin() + *(key.trxHash.begin() + 1);
                int percentageDone = (int) (high * 100.0 / 65536.0 + 0.5);
                uiInterface.ShowProgress(_("Upgrading UTXO database"), percentageDone);
                if (reportDone < percentageDone / 10) {
                    // report max. every 10% step
                    LogPrintf("[%d%%]...", percentageDone); /* Continued */
                    reportDone = percentageDone / 10;
                }
            }

            pCursor->GetValue(bitcoin_coin, true);

            if (prev_key.trxHash != key.trxHash) {
                if (!btcu_coins.vout.empty()) {

                    batch.Write(btcu_key, btcu_coins);

                    if (batch.SizeEstimate() > batch_size) {
                        db.WriteBatch(batch);
                        db.CompactRange(range_key, prev_key);
                        batch.Clear();
                        range_key = prev_key;
                    }
                }
                prev_key.trxHash = key.trxHash;
                prev_key.n = 0;

                btcu_coins.fCoinStake = false;
                btcu_coins.fCoinBase = bitcoin_coin.fCoinBase;
                btcu_coins.nHeight = bitcoin_coin.nHeight;
                btcu_coins.nVersion = CTransaction::BITCOIN_VERSION;
                btcu_coins.vout.clear();

                btcu_key.second = key.trxHash;
            }
            for (; prev_key.n < key.n; ++prev_key.n) {
                btcu_coins.vout.push_back(CTxOut());
            }

            btcu_coins.vout.push_back(bitcoin_coin.out);
            batch.Erase(pCursor->GetKey());

        } catch (const std::exception& e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }

    batch.Write(btcu_key, btcu_coins);
    batch.Write('B', hashBestBlock);
    db.WriteBatch(batch);
    db.CompactRange(range_key, key);

    uiInterface.ShowProgress("", 100);
    LogPrintf("[DONE].\n");
    return true;
}

bool CBlockTreeDB::WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*> >& fileInfo, int nLastFile, const std::vector<const CBlockIndex*>& blockinfo) {
    CLevelDBBatch batch(*this);
    for (std::vector<std::pair<int, const CBlockFileInfo*> >::const_iterator it=fileInfo.begin(); it != fileInfo.end(); it++) {
        batch.Write(std::make_pair('f', it->first), *it->second);
    }
    batch.Write('l', nLastFile);
    for (std::vector<const CBlockIndex*>::const_iterator it=blockinfo.begin(); it != blockinfo.end(); it++) {
        batch.Write(std::make_pair('b', (*it)->GetBlockHash()), CDiskBlockIndex(*it));
    }
    return WriteBatch(batch, true);
}

bool CBlockTreeDB::ReadTxIndex(const uint256& txid, CDiskTxPos& pos)
{
    return Read(std::make_pair('t', txid), pos);
}

bool CBlockTreeDB::WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> >& vect)
{
    CLevelDBBatch batch(*this);
    for (std::vector<std::pair<uint256, CDiskTxPos> >::const_iterator it = vect.begin(); it != vect.end(); it++)
        batch.Write(std::make_pair('t', it->first), it->second);
    return WriteBatch(batch);
}

bool CBlockTreeDB::WriteFlag(const std::string& name, bool fValue)
{
    return Write(std::make_pair('F', name), fValue ? '1' : '0');
}

bool CBlockTreeDB::ReadFlag(const std::string& name, bool& fValue)
{
    char ch;
    if (!Read(std::make_pair('F', name), ch))
        return false;
    fValue = ch == '1';
    return true;
}

bool CBlockTreeDB::WriteInt(const std::string& name, int nValue)
{
    return Write(std::make_pair('I', name), nValue);
}

bool CBlockTreeDB::ReadInt(const std::string& name, int& nValue)
{
    return Read(std::make_pair('I', name), nValue);
}

bool CBlockTreeDB::LoadBlockIndexGuts()
{
    auto pcursor = NewIterator();

    pcursor->Seek(std::make_pair('b', uint256(0)));

    // Load mapBlockIndex
    uint256 nPreviousCheckpoint;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            char chType;
            pcursor->GetKey(chType, true);
            if (chType == 'b') {
                CDiskBlockIndex diskindex;
                pcursor->GetValue(diskindex, true);

                // Construct block index object
                CBlockIndex* pindexNew = InsertBlockIndex(diskindex.GetBlockHash());
                pindexNew->pprev = InsertBlockIndex(diskindex.hashPrev);
                pindexNew->pnext = InsertBlockIndex(diskindex.hashNext);
                pindexNew->nHeight = diskindex.nHeight;
                pindexNew->nFile = diskindex.nFile;
                pindexNew->nDataPos = diskindex.nDataPos;
                pindexNew->nUndoPos = diskindex.nUndoPos;
                pindexNew->nVersion = diskindex.nVersion;
                pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
                pindexNew->nTime = diskindex.nTime;
                pindexNew->nBits = diskindex.nBits;
                pindexNew->nNonce = diskindex.nNonce;
                pindexNew->nStatus = diskindex.nStatus;
                pindexNew->nTx = diskindex.nTx;
                pindexNew->hashChainstate = diskindex.hashChainstate;
                pindexNew->hashStateRoot  = diskindex.hashStateRoot; // qtum
                pindexNew->hashUTXORoot   = diskindex.hashUTXORoot; // qtum

                //zerocoin
                pindexNew->nAccumulatorCheckpoint = diskindex.nAccumulatorCheckpoint;
                pindexNew->mapZerocoinSupply = diskindex.mapZerocoinSupply;
                pindexNew->vMintDenominationsInBlock = diskindex.vMintDenominationsInBlock;

                //Proof Of Stake
                pindexNew->nMint = diskindex.nMint;
                pindexNew->nMoneySupply = diskindex.nMoneySupply;
                pindexNew->nFlags = diskindex.nFlags;
                if (!Params().IsStakeModifierV2(pindexNew->nHeight)) {
                    pindexNew->nStakeModifier = diskindex.nStakeModifier;
                } else {
                    pindexNew->nStakeModifierV2 = diskindex.nStakeModifierV2;
                }
                pindexNew->prevoutStake = diskindex.prevoutStake;
                pindexNew->nStakeTime = diskindex.nStakeTime;
                pindexNew->hashProofOfStake = diskindex.hashProofOfStake;

                if (pindexNew->nHeight <= Params().LAST_POW_BLOCK()) {
                    if (!CheckProofOfWork(pindexNew->GetBlockHash(), pindexNew->nBits))
                        return error("LoadBlockIndex() : CheckProofOfWork failed: %s", pindexNew->ToString());
                }

                pcursor->Next();
            } else {
                break; // if shutdown requested or finished loading block index
            }
        } catch (const std::exception& e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }

    return true;
}

CZerocoinDB::CZerocoinDB(size_t nCacheSize, bool fMemory, bool fWipe) : CLevelDBWrapper(GetDataDir() / "zerocoin", nCacheSize, fMemory, fWipe)
{
}

bool CZerocoinDB::WriteCoinMintBatch(const std::vector<std::pair<libzerocoin::PublicCoin, uint256> >& mintInfo)
{
    CLevelDBBatch batch(*this);
    size_t count = 0;
    for (std::vector<std::pair<libzerocoin::PublicCoin, uint256> >::const_iterator it=mintInfo.begin(); it != mintInfo.end(); it++) {
        libzerocoin::PublicCoin pubCoin = it->first;
        uint256 hash = GetPubCoinHash(pubCoin.getValue());
        batch.Write(std::make_pair('m', hash), it->second);
        ++count;
    }

    LogPrint("zero", "Writing %u coin mints to db.\n", (unsigned int)count);
    return WriteBatch(batch, true);
}

bool CZerocoinDB::ReadCoinMint(const CBigNum& bnPubcoin, uint256& hashTx)
{
    return ReadCoinMint(GetPubCoinHash(bnPubcoin), hashTx);
}

bool CZerocoinDB::ReadCoinMint(const uint256& hashPubcoin, uint256& hashTx)
{
    return Read(std::make_pair('m', hashPubcoin), hashTx);
}

bool CZerocoinDB::EraseCoinMint(const CBigNum& bnPubcoin)
{
    uint256 hash = GetPubCoinHash(bnPubcoin);
    return Erase(std::make_pair('m', hash));
}

bool CZerocoinDB::WriteCoinSpendBatch(const std::vector<std::pair<libzerocoin::CoinSpend, uint256> >& spendInfo)
{
    CLevelDBBatch batch(*this);
    size_t count = 0;
    for (std::vector<std::pair<libzerocoin::CoinSpend, uint256> >::const_iterator it=spendInfo.begin(); it != spendInfo.end(); it++) {
        CBigNum bnSerial = it->first.getCoinSerialNumber();
        CDataStream ss(SER_GETHASH, 0);
        ss << bnSerial;
        uint256 hash = Hash(ss.begin(), ss.end());
        batch.Write(std::make_pair('s', hash), it->second);
        ++count;
    }

    LogPrint("zero", "Writing %u coin spends to db.\n", (unsigned int)count);
    return WriteBatch(batch, true);
}

bool CZerocoinDB::ReadCoinSpend(const CBigNum& bnSerial, uint256& txHash)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << bnSerial;
    uint256 hash = Hash(ss.begin(), ss.end());

    return Read(std::make_pair('s', hash), txHash);
}

bool CZerocoinDB::ReadCoinSpend(const uint256& hashSerial, uint256 &txHash)
{
    return Read(std::make_pair('s', hashSerial), txHash);
}

bool CZerocoinDB::EraseCoinSpend(const CBigNum& bnSerial)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << bnSerial;
    uint256 hash = Hash(ss.begin(), ss.end());

    return Erase(std::make_pair('s', hash));
}

bool CZerocoinDB::WipeCoins(std::string strType)
{
    if (strType != "spends" && strType != "mints")
        return error("%s: did not recognize type %s", __func__, strType);

    auto pcursor = NewIterator();

    char type = (strType == "spends" ? 's' : 'm');
    pcursor->Seek(std::make_pair(type, uint256()));
    // Load mapBlockIndex
    std::set<uint256> setDelete;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            char chType;
            pcursor->GetKey(chType, true);
            if (chType == type) {
                uint256 hash;
                pcursor->GetValue(hash, true);
                setDelete.insert(hash);
                pcursor->Next();
            } else {
                break; // if shutdown requested or finished loading block index
            }
        } catch (const std::exception& e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }

    for (auto& hash : setDelete) {
        if (!Erase(std::make_pair(type, hash)))
            LogPrintf("%s: error failed to delete %s\n", __func__, hash.GetHex());
    }

    return true;
}


// Legacy Zerocoin Database
static const char LZC_ACCUMCS = 'A';

bool CZerocoinDB::WriteAccChecksum(const uint32_t& nChecksum, const libzerocoin::CoinDenomination denom, const int nHeight)
{
    return Write(std::make_pair(LZC_ACCUMCS, std::make_pair(nChecksum, denom)), nHeight);
}

bool CZerocoinDB::ReadAccChecksum(const uint32_t& nChecksum, const libzerocoin::CoinDenomination denom, int& nHeightRet)
{
    return Read(std::make_pair(LZC_ACCUMCS, std::make_pair(nChecksum, denom)), nHeightRet);
}

bool CZerocoinDB::EraseAccChecksum(const uint32_t& nChecksum, const libzerocoin::CoinDenomination denom)
{
    return Erase(std::make_pair(LZC_ACCUMCS, std::make_pair(nChecksum, denom)));
}

bool CZerocoinDB::WipeAccChecksums()
{
    auto pcursor = NewIterator();
    pcursor->Seek(std::make_pair(LZC_ACCUMCS, (uint32_t) 0));
    std::set<uint32_t> setDelete;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            char chType;
            pcursor->GetKey(chType, true);
            if (chType == LZC_ACCUMCS) {
                uint32_t acs;
                pcursor->GetValue(acs, true);
                setDelete.insert(acs);
                pcursor->Next();
            } else break;
        } catch (const std::exception& e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }

    for (auto& acs : setDelete) {
        if (!Erase(std::make_pair(LZC_ACCUMCS, acs)))
            LogPrintf("%s: error failed to acc checksum %s\n", __func__, acs);
    }

    LogPrintf("%s: AccChecksum database removed.\n", __func__);
    return true;
}
