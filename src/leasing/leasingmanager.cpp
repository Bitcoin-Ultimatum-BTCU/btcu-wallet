// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "leasing/leasingmanager.h"

#ifdef ENABLE_LEASING_MANAGER

#include <limits>
#include <tuple>

#include "script/standard.h"

#include "util.h"
#include "chain.h"
#include "leveldbwrapper.h"
#include "serialize.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/composite_key.hpp>

#ifdef  _WIN32
#include <boost/thread/interruption.hpp>
#endif // _WIN32



namespace bmi = boost::multi_index;

namespace {
   // 1.00% -> precision 2 symbols
   static constexpr int64_t _1pct = 100;
   // 100.00%
   static constexpr int64_t _100pct = 100 * _1pct;

   static constexpr int _nonRewardHeight = std::numeric_limits<int>::max();
}



struct CLeasingOutput {
   uint256 nTrxHash;
   uint32_t nPosition = -1;
   CAmount nValue;
   int nInitHeight = _nonRewardHeight;
   CScript scriptPubKey;
   CKeyID kOwnerID;
   CKeyID kLeaserID;
   int nNextRewardHeight = _nonRewardHeight;
   int nLastRewardHeight = _nonRewardHeight;
   int nSpendingHeight = _nonRewardHeight;

   ADD_SERIALIZE_METHODS;

   template <typename Stream, typename Operation>
   inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
      READWRITE(VARINT(nValue));
      READWRITE(VARINT(nInitHeight));
      READWRITE(scriptPubKey);
      READWRITE(kOwnerID);
      READWRITE(kLeaserID);
      READWRITE(VARINT(nLastRewardHeight));
      READWRITE(VARINT(nNextRewardHeight));
      READWRITE(VARINT(nSpendingHeight));
   }
};



struct byTrxHash;
struct byLeasingReward;
struct byOwner;

using CLeasingOutputMap = bmi::multi_index_container<
    CLeasingOutput,
    bmi::indexed_by<
        bmi::ordered_unique<
            bmi::tag<byTrxHash>,
            bmi::composite_key<
                CLeasingOutput,
                    bmi::member<CLeasingOutput, uint256,  &CLeasingOutput::nTrxHash>,
                    bmi::member<CLeasingOutput, uint32_t, &CLeasingOutput::nPosition>>>,
        bmi::ordered_non_unique<
            bmi::tag<byLeasingReward>,
            bmi::composite_key<
                CLeasingOutput,
                bmi::member<CLeasingOutput, CKeyID, &CLeasingOutput::kLeaserID>,
                bmi::member<CLeasingOutput, int, &CLeasingOutput::nNextRewardHeight>>>,
                bmi::ordered_non_unique<bmi::tag<byOwner>, bmi::member<CLeasingOutput, CKeyID, &CLeasingOutput::kOwnerID>>
    >>;



struct CLeasingReward {
   uint256 nTrxHash;
   uint32_t nPosition = -1;

   uint256 nLeasingHash;
   uint32_t nLeasingPosition = -1;

   int nLastRewardHeight = _nonRewardHeight;
   int nNextRewardHeight = _nonRewardHeight;

   ADD_SERIALIZE_METHODS;

   template <typename Stream, typename Operation>
   inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
      READWRITE(nLeasingHash);
      READWRITE(VARINT(nLeasingPosition));
      READWRITE(VARINT(nLastRewardHeight));
      READWRITE(VARINT(nNextRewardHeight));
   }
};



struct CLeasingSpend {
   uint256 nTrxHash;
   uint32_t nPosition = -1;

   uint256 nLeasingHash;
   uint32_t nLeasingPosition = -1;

   ADD_SERIALIZE_METHODS;

   template <typename Stream, typename Operation>
   inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
      READWRITE(nLeasingHash);
      READWRITE(VARINT(nLeasingPosition));
   }
};



enum class LeasingDBType: char {
   Output = 'O',
   Reward = 'R',
   Spend = 'S',
   Flag = 'F'
};

namespace {
   static const std::string _shutdown = "shutdown";
}

class CLeasingDB : public CLevelDBWrapper {
   CLeasingDB(const CLeasingDB&) = delete;
   void operator=(const CLeasingDB&) = delete;

public:
   CLeasingDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false):
      CLevelDBWrapper(GetDataDir() / "leasing", nCacheSize, fMemory, fWipe)
   { }

   bool WriteLeasingOutput(const CLeasingOutput& leasingOut) {
      return Write(GetKey(LeasingDBType::Output, leasingOut), leasingOut);
   }

   bool EraseLeasingOutput(const CLeasingOutput& leasingOut) {
      return Erase(GetKey(LeasingDBType::Output, leasingOut));
   }

   bool ReadLeasingOutput(CLeasingOutput& leasingOut) {
      return Read(GetKey(LeasingDBType::Output, leasingOut), leasingOut);
   }

   bool WriteLeasingReward(const CLeasingReward& leasingReward) {
      return Write(GetKey(LeasingDBType::Reward, leasingReward), leasingReward);
   }

   bool EraseLeasingReward(const CLeasingReward& leasingReward) {
      return Erase(GetKey(LeasingDBType::Reward, leasingReward));
   }

   bool ReadLeasingReward(CLeasingReward& leasingReward) {
      return Read(GetKey(LeasingDBType::Reward, leasingReward), leasingReward);
   }

   bool WriteLeasingSpend(const CLeasingSpend& leasingSpend) {
      return Write(GetKey(LeasingDBType::Spend, leasingSpend), leasingSpend);
   }

   bool EraseLeasingSpend(const CLeasingSpend& leasingSpend) {
      return Erase(GetKey(LeasingDBType::Spend, leasingSpend));
   }

   bool ReadLeasingSpend(CLeasingSpend& leasingSpend) {
      return Read(GetKey(LeasingDBType::Spend, leasingSpend), leasingSpend);
   }

   bool WriteShutdown(bool fValue) {
      return Write(std::make_pair(char(LeasingDBType::Flag), _shutdown), fValue ? '1' : '0');
   }

   bool ReadShutdown(bool& fValue) {
      char ch;
      if (!Read(std::make_pair(char(LeasingDBType::Flag), _shutdown), ch))
         return false;
      fValue = (ch == '1');
      return true;
   }

private:
   template <typename T>
   std::tuple<char, uint256, uint32_t> GetKey(const LeasingDBType type, const T& v) const {
      return std::make_tuple(char(type), v.nTrxHash, v.nPosition);
   }
};



class CLeasingManager::CImpl {
public:
   CImpl(): leasingDB(1 << 20) {
      OpenDB();
   }

   ~CImpl() {
      leasingDB.WriteShutdown(true);
      leasingDB.Flush();
   }

   bool AddLeasingOutput(const CTransaction& tx, const CTxOut& txOut, const uint32_t n) {
      LOCK(cs_leasing);

      auto add = Params().GetLeasingRewardMaturity();

      CLeasingOutput leasingOutput;
      leasingOutput.nTrxHash = tx.GetHash();
      leasingOutput.nPosition = n;
      leasingOutput.nValue = txOut.nValue;
      leasingOutput.nInitHeight = nBlockHeight;
      leasingOutput.scriptPubKey = txOut.scriptPubKey;
      leasingOutput.nNextRewardHeight = CalcNextRewardHeight() + add;
      leasingOutput.nLastRewardHeight = nBlockHeight + add;

      LeasingLogPrint("new next reward height %d for %s:%d",
        leasingOutput.nNextRewardHeight, leasingOutput.nTrxHash.ToString(), leasingOutput.nPosition);

      std::vector<valtype> vSolutions;
      txnouttype whichType;
      if (Solver(txOut.scriptPubKey, whichType, vSolutions) && TX_LEASE == whichType) {
         leasingOutput.kLeaserID = CKeyID(uint160(vSolutions[0]));
         leasingOutput.kOwnerID  = CKeyID(uint160(vSolutions[1]));
      }

      if (leasingOutput.kOwnerID.IsNull()) {
         return LeasingError("fail to get the owner address from the leasing script from %s:%d",
            txOut.GetHash().ToString(), n);
      }

      if (leasingOutput.kLeaserID.IsNull()) {
         return LeasingError("%s : fail to get the leaser address from the leasing script from %s:%d",
            txOut.GetHash().ToString(), n);
      }

      if (!mapOutputs.insert(leasingOutput).second) {
         return LeasingError("duplicate of leasing %s:%d", txOut.GetHash().ToString(), n);
      }

      leasingDB.WriteLeasingOutput(leasingOutput);

      IncLeasingSupply(leasingOutput.kLeaserID, txOut.nValue);

      return true;
   }

   void RemoveLeasingOutput(const CTransaction& tx, const uint32_t n) {
      LOCK(cs_leasing);

      auto& idxTrxHash = mapOutputs.get<byTrxHash>();
      auto itr = idxTrxHash.find(std::make_tuple(tx.GetHash(), n));

      if (idxTrxHash.end() == itr) {
         return;
      }

      leasingDB.EraseLeasingOutput(*itr);

      DecLeasingSupply((*itr).kLeaserID, (*itr).nValue);

      idxTrxHash.erase(itr);
   }

   void AddLeasingSpend(const CTransaction& tx, const COutPoint& txPoint, const uint32_t n) {
      LOCK(cs_leasing);

      auto& idxTrxHash = mapOutputs.get<byTrxHash>();
      auto itr = idxTrxHash.find(std::make_tuple(txPoint.hash, txPoint.n));

      if (idxTrxHash.end() == itr) {
         return;
      }

      CLeasingSpend leasingSpend;
      leasingSpend.nTrxHash = tx.GetHash();
      leasingSpend.nPosition = n;
      leasingSpend.nLeasingHash = (*itr).nTrxHash;
      leasingSpend.nLeasingPosition = (*itr).nPosition;

      leasingDB.WriteLeasingSpend(leasingSpend);

      idxTrxHash.modify(itr, [&](CLeasingOutput& v) {
         v.nSpendingHeight = nBlockHeight;

         DecLeasingSupply(v.kLeaserID, v.nValue);
         CalcLeasingSupplyPct();

         leasingDB.WriteLeasingOutput(v);
      });

      idxTrxHash.erase(itr);
   }

   bool RemoveLeasingSpend(const CTransaction& tx, const COutPoint& txPoint, const uint32_t n) {
      LOCK(cs_leasing);

      CLeasingSpend leasingSpend;
      leasingSpend.nTrxHash = tx.GetHash();
      leasingSpend.nPosition = n;

      if (!leasingDB.ReadLeasingSpend(leasingSpend)) {
         return true;
      }

      CLeasingOutput leasingOutput;
      leasingOutput.nTrxHash = leasingSpend.nLeasingHash;
      leasingOutput.nPosition = leasingSpend.nLeasingPosition;

      if (!leasingDB.ReadLeasingOutput(leasingOutput)) {
         return LeasingError("fail to read leasing from DB for %s:%d",
            leasingOutput.nTrxHash.ToString(), leasingOutput.nPosition);
      }

      leasingOutput.nSpendingHeight = _nonRewardHeight;

      if (!mapOutputs.insert(leasingOutput).second) {
         return LeasingError("duplicate leasing on reading from DB for %s:%d", leasingOutput.nTrxHash.ToString(), leasingOutput.nPosition);
      }

      IncLeasingSupply(leasingOutput.kLeaserID, leasingOutput.nValue);

      return true;
   }

   bool AddLeasingReward(const CTransaction& tx, const CTxOut& txout, const uint32_t n) {
      LOCK(cs_leasing);

      COutPoint point;

      if (txout.IsEmpty()) {
         return true;
      }

      if (!ExtractLeasingPoint(txout, point)) {
         return LeasingError("can't extract leasing reward point from %s:%d", txout.GetHash().ToString(), n);
      }

      ModifyLeasingOutput(point.hash, point.n, [&](CLeasingOutput& v){
         CLeasingReward leasingReward;
         leasingReward.nTrxHash = tx.GetHash();
         leasingReward.nPosition = n;
         leasingReward.nLeasingHash = v.nTrxHash;
         leasingReward.nLeasingPosition = v.nPosition;
         leasingReward.nLastRewardHeight = v.nLastRewardHeight;
         leasingReward.nNextRewardHeight = v.nNextRewardHeight;

         leasingDB.WriteLeasingReward(leasingReward);

         v.nLastRewardHeight = this->nBlockHeight;
         v.nNextRewardHeight = CalcNextRewardHeight();

         LeasingLogPrint("set next reward height %d for %s:%d",
            v.nNextRewardHeight, v.nTrxHash.ToString(), v.nPosition);

         leasingDB.WriteLeasingOutput(v);
      });
      return true;
   }

   bool RemoveLeasingReward(const CTransaction& tx, const CTxOut& txout, const uint32_t n) {
      LOCK(cs_leasing);

      COutPoint point;

      if (!ExtractLeasingPoint(txout, point)) {
         return LeasingError("can't extract leasing reward point for %s:%d", txout.GetHash().ToString(), n);
      }

      CLeasingReward leasingReward;
      leasingReward.nTrxHash = tx.GetHash();
      leasingReward.nPosition = n;

      if (!leasingDB.ReadLeasingReward(leasingReward)) {
         return LeasingError("can't read leasing reward from DB for %s:%d", tx.GetHash().ToString(), n);
      }
      leasingDB.EraseLeasingReward(leasingReward);

      ModifyLeasingOutput(point.hash, point.n, [&](CLeasingOutput& v){
         v.nNextRewardHeight = leasingReward.nNextRewardHeight;
         v.nLastRewardHeight = leasingReward.nLastRewardHeight;

         LeasingLogPrint("restore next reward height %d for %s:%d",
            v.nNextRewardHeight, v.nTrxHash.ToString(), v.nPosition);

         leasingDB.WriteLeasingOutput(v);
      });
      return true;
   }

   void SetBlock(const int height, const uint256& hash) {
      LOCK(cs_leasing);

      nBlockHeight = height;
      nBlockHash = hash;
      CalcLeasingHeightPct();
      leasingDB.Flush();
   }

   void GetAllAmountsLeasedTo(CPubKey &pubKey, CAmount &amount) const {
      CKeyID ownerID = CPubKey(pubKey).GetID();
      LOCK(cs_leasing);
      auto &idxLeasedTo = mapOutputs.get<byOwner>();
      auto itr = idxLeasedTo.lower_bound(ownerID);
      amount = 0;
      for (; itr != idxLeasedTo.end() && itr->kLeaserID == ownerID; ++itr)
         amount += itr->nValue;
   }

   bool GetLeasingRewards(
       const LeaserType type,
       const CKeyID& leaserID,
       const size_t nLimit,
       std::vector<CTxOut>& vRewards
   ) {
      LOCK(cs_leasing);

      auto& idxRewards = mapOutputs.get<byLeasingReward>();
      auto itr = idxRewards.lower_bound(leaserID);
      size_t i = 0;

      vRewards.reserve(vRewards.size() + nLimit + 1);
      for (; itr != idxRewards.end() && itr->kLeaserID == leaserID && itr->nNextRewardHeight <= nBlockHeight; ++itr) {
         auto txOut = CalcLeasingReward(*itr);
         if (!txOut.IsEmpty()) {
            vRewards.emplace_back(std::move(txOut));
            if (++i >= nLimit)
               break;
         }
      }

      // don't pay if no leasees
      if (!i) return false;

      auto btr = mapBalance.find(leaserID);
      if (mapBalance.end() != btr) {
         auto txOut = CalcLeasingReward(type, leaserID, (*btr).second);
         if (!txOut.IsEmpty())
            vRewards.emplace_back(std::move(txOut));
      }
      return true;
   }

   int64_t GetLeasingPct() const {
      return nHeightLeasingPct * nSupplyLeasingPct / _100pct;
   }

   int GetBlockHeight() const {
      return nBlockHeight;
   }

   const uint256& GetBlockHash() const {
      return nBlockHash;
   }

   CTxOut CalcLeasingReward(const COutPoint& point, const CKeyID& keyID) const {
      if (point.hash.IsNull()) {
         const auto leaserType = static_cast<LeaserType>(point.n);

         switch (leaserType) {
            case LeaserType::MasterNode:
            case LeaserType::ValidatorNode:
               break;
            default:
               return CTxOut(); // empty
         }

         auto itr = mapBalance.find(keyID);
         if (mapBalance.end() == itr)
            return CTxOut(); // empty

         return CalcLeasingReward(leaserType, keyID, (*itr).second);
      }

      auto& idxTrxHash = mapOutputs.get<byTrxHash>();
      auto itr = idxTrxHash.find(std::make_tuple(point.hash, point.n));

      if (idxTrxHash.end() == itr)
         return CTxOut(); // empty

      return CalcLeasingReward(*itr);
   }

private:
   void OpenDB() {
      bool fLastShutdown = false;
      bool wasShutdown = (!leasingDB.ReadShutdown(fLastShutdown) || !fLastShutdown);
      LogPrintf("%s: Last shutdown was prepared: %s\n", __func__, wasShutdown);

      leasingDB.WriteShutdown(false);
      leasingDB.Flush();

      auto pCursor = leasingDB.NewIterator();
      while (pCursor->Valid()) {
         boost::this_thread::interruption_point();
         try {
            leveldb::Slice slKey = pCursor->GetKey();
            CDataStream ssKey(slKey.data(), slKey.data() + slKey.size(), SER_DISK, CLIENT_VERSION);
            char chType;
            ssKey >> chType;
            switch (static_cast<LeasingDBType>(chType)) {
               case LeasingDBType::Output: {
                  CLeasingOutput leasingOutput;
                  pCursor->GetValue(leasingOutput, true);

                  ssKey >> leasingOutput.nTrxHash;
                  ssKey >> leasingOutput.nPosition;
                  if (leasingOutput.nSpendingHeight != _nonRewardHeight) {
                     if (!mapOutputs.insert(leasingOutput).second) {
                        LeasingError("duplicate of leasing for %s:%d",
                           leasingOutput.nTrxHash.ToString(), leasingOutput.nPosition);
                     } else {
                        nLeasingSupply += leasingOutput.nValue;
                        auto itr = mapBalance.emplace(leasingOutput.kLeaserID, 0).first;
                        (*itr).second += leasingOutput.nValue;
                     }
                  }
                  break;
               }
               case LeasingDBType::Reward:
               case LeasingDBType::Spend:
               case LeasingDBType::Flag:
               default:
                  break;
            }
            pCursor->Next();
         } catch (const std::exception& e) {
            LeasingError("deserialize or I/O error - %s", e.what());
            break;
         }
      }

      CalcLeasingSupplyPct();
   }

   void IncLeasingSupply(const CKeyID& kLeaserID, const CAmount aAmount) {
      nLeasingSupply += aAmount;
      CalcLeasingSupplyPct();

      auto itr = mapBalance.emplace(kLeaserID, 0).first;
      (*itr).second += aAmount;
   }

   void DecLeasingSupply(const CKeyID& kLeaserID, const CAmount aAmount) {
      auto btr = mapBalance.find(kLeaserID);
      if (mapBalance.end() != btr) {
         if (aAmount == (*btr).second) {
            mapBalance.erase(btr);
         } else {
            (*btr).second -= aAmount;
         }
      }

      nLeasingSupply -= aAmount;
      CalcLeasingSupplyPct();
   }

   template <typename TModifyAction>
   void ModifyLeasingOutput(const uint256& hash, int n, TModifyAction&& modify) {
      auto& idxTrxHash = mapOutputs.get<byTrxHash>();
      auto itr = idxTrxHash.find(std::make_tuple(hash, n));

      if (idxTrxHash.end() != itr) {
         idxTrxHash.modify(itr, std::forward<TModifyAction>(modify));
      }
   }

   int CalcNextRewardHeight() const {
      return nBlockHeight + Params().GetLeasingRewardPeriod();
   }

   void CalcLeasingHeightPct() {
      nHeightLeasingPct = 15;

      if (nBlockHeight  < 500000)
         nHeightLeasingPct = 15;
      else if (nBlockHeight  < 1000000)
         nHeightLeasingPct = 14;
      else if (nBlockHeight  < 1500000)
         nHeightLeasingPct = 13;
      else if (nBlockHeight  < 2000000)
         nHeightLeasingPct = 12;
      else if (nBlockHeight  < 2500000)
         nHeightLeasingPct = 11;
      else if (nBlockHeight  < 3000000)
         nHeightLeasingPct = 10;
      else if (nBlockHeight  < 3500000)
         nHeightLeasingPct = 9;
      else if (nBlockHeight  < 4000000)
         nHeightLeasingPct = 8;
      else if (nBlockHeight  < 4500000)
         nHeightLeasingPct = 7;
      else if (nBlockHeight  < 5000000)
         nHeightLeasingPct = 6;
      else if (nBlockHeight  < 5500000)
         nHeightLeasingPct = 5;
      else if (nBlockHeight  < 6000000)
         nHeightLeasingPct = 4;
      else if (nBlockHeight  < 6500000)
         nHeightLeasingPct = 3;
      else if (nBlockHeight  < 7000000)
         nHeightLeasingPct = 2;
      else
         nHeightLeasingPct = 1;

      nHeightLeasingPct *= _1pct;
   }

   void CalcLeasingSupplyPct() {
      nSupplyLeasingPct = 100;
      auto supply = nLeasingSupply / COIN;

      if (supply < 500000)
         nSupplyLeasingPct = 100;
      else if (supply < 1000000)
         nSupplyLeasingPct = 95;
      else if (supply < 2000000)
         nSupplyLeasingPct = 90;
      else if (supply < 3000000)
         nSupplyLeasingPct = 85;
      else if (supply < 4000000)
         nSupplyLeasingPct = 80;
      else if (supply < 5000000)
         nSupplyLeasingPct = 75;
      else if (supply < 6000000)
         nSupplyLeasingPct = 70;
      else if (supply < 7000000)
         nSupplyLeasingPct = 65;
      else if (supply < 8000000)
         nSupplyLeasingPct = 60;
      else if (supply < 9000000)
         nSupplyLeasingPct = 55;
      else if (supply < 10000000)
         nSupplyLeasingPct = 50;
      else
         nSupplyLeasingPct = 45;

      nSupplyLeasingPct *= _1pct;
   }

   CTxOut CalcLeasingReward(const CLeasingOutput& leasingOut) const {
      CAmount aAmount = leasingOut.nValue / COIN;

      int64_t nAmountPct = 100;
      if (aAmount < 5000)
         nAmountPct = 100;
      else if (aAmount < 10000)
         nAmountPct = 95;
      else if (aAmount < 20000)
         nAmountPct = 90;
      else if (aAmount < 50000)
         nAmountPct = 85;
      else if (aAmount < 100000)
         nAmountPct = 80;
      else if (aAmount < 200000)
         nAmountPct = 75;
      else if (aAmount < 300000)
         nAmountPct = 70;
      else if (aAmount < 400000)
         nAmountPct = 65;
      else if (aAmount < 500000)
         nAmountPct = 60;
      else if (aAmount < 1000000)
         nAmountPct = 55;
      else
         nAmountPct = 50;
      nAmountPct *= _1pct;

      int64_t nAge = GetBlockHeight() - leasingOut.nInitHeight;
      int64_t nAgePct = 100;
      if (nAge < 30000)
         nAgePct = 100;
      else if (nAge < 90000)
         nAgePct = 110;
      else if (nAge < 180000)
         nAgePct = 120;
      else if (nAge < 270000)
         nAgePct = 130;
      else if (nAge < 360000)
         nAgePct = 140;
      else if (nAge < 540000)
         nAgePct = 150;
      else if (nAge < 720000)
         nAgePct = 160;
      else if (nAge < 1080000)
         nAgePct = 170;
      else if (nAge < 1440000)
         nAgePct = 180;
      else if (nAge < 1800000)
         nAgePct = 190;
      else
         nAgePct = 200;
      nAgePct *= _1pct;

      int64_t nPct = nAmountPct * nAgePct * GetLeasingPct() / _100pct / _100pct;
      int64_t aResAmount = leasingOut.nValue * nPct / _100pct;
      int64_t nRewardAge = (GetBlockHeight() - leasingOut.nLastRewardHeight);

      aResAmount = aResAmount * nRewardAge / (365 * 24 * 60); // 1 year

      auto outPoint = COutPoint(leasingOut.nTrxHash, leasingOut.nPosition);
      auto outScript = GetScriptForLeasingReward(outPoint, leasingOut.kOwnerID);

      LeasingLogPrint("nPct=%d, aResAmount=%d, nRewardAge=%d", nPct, aResAmount, nRewardAge);
      LeasingLogPrint("outPoint=%s", outPoint.ToString());
      LeasingLogPrint("outScript=%s", outScript.ToString());

      return CTxOut(aResAmount, outScript);
   }

   CTxOut CalcLeasingReward(const LeaserType type, const CKeyID& leaserID, const CAmount aAmount) const {
      int64_t nPct = 0;

      if (LeaserType::ValidatorNode == type)
         nPct = GetValidatorNodeLeasingPct(aAmount);
      else if (LeaserType::MasterNode == type)
         nPct = GetMasterNodeLeasingPct(aAmount);

      uint256 trxHash;
      trxHash.SetNull();
      auto outPoint = COutPoint(trxHash, static_cast<int>(type));
      auto outScript = GetScriptForLeasingReward(outPoint, leaserID);

      LeasingLogPrint("CTxOut(aAmount(%d) * nPct(%d) / _100pct(%d), outScript(%s)", aAmount, nPct, _100pct, outScript.ToString());

      return CTxOut(aAmount * nPct / _100pct, outScript);
   }

   int64_t GetValidatorNodeLeasingPct(CAmount aAmount) const {
      aAmount /= COIN;

      if (aAmount < 30000)
         return 20;
      else if (aAmount < 90000)
         return 18;
      else if (aAmount < 180000)
         return 16;
      else if (aAmount < 270000)
         return 14;
      else if (aAmount < 360000)
         return 12;
      else if (aAmount < 540000)
         return 10;
      else if (aAmount < 720000)
         return 8;
      else if (aAmount < 1080000)
         return 6;
      else if (aAmount < 1440000)
         return 4;
      else if (aAmount < 1800000)
         return 2;

      return 0;
   }

   int64_t GetMasterNodeLeasingPct(CAmount aAmount) const {
      aAmount /= COIN;

      if (aAmount < 30000)
         return 10;
      else if (aAmount < 90000)
         return 9;
      else if (aAmount < 180000)
         return 8;
      else if (aAmount < 270000)
         return 7;
      else if (aAmount < 360000)
         return 6;
      else if (aAmount < 540000)
         return 5;
      else if (aAmount < 720000)
         return 4;
      else if (aAmount < 1080000)
         return 3;
      else if (aAmount < 1440000)
         return 2;
      else if (aAmount < 1800000)
         return 1;

      return 0;
   }

   bool ExtractLeasingPoint(const CTxOut& txout, COutPoint& point) const {
      std::vector<valtype> vSolutions;
      txnouttype whichType;
      if (Solver(txout.scriptPubKey, whichType, vSolutions) && TX_LEASINGREWARD == whichType) {
         point.hash = uint256(vSolutions[0]);
         point.n = CScriptNum(vSolutions[1], true).getint();
         return true;
      }
      return false;
   }

private:
   mutable CCriticalSection cs_leasing;

   CLeasingDB leasingDB;
   CLeasingOutputMap mapOutputs;
   std::map<CKeyID, CAmount> mapBalance;
   int nBlockHeight = 0;
   uint256 nBlockHash;
   CAmount nLeasingSupply = 0;
   int64_t nHeightLeasingPct = 15 * _1pct;
   int64_t nSupplyLeasingPct = _100pct;
};



CLeasingManager::CLeasingManager():
    pImpl(new CLeasingManager::CImpl()) {
}

CLeasingManager::~CLeasingManager() {
}

void CLeasingManager::UpdatedBlockTip(const CBlockIndex* pIndex) {
   pImpl->SetBlock(pIndex->nHeight, pIndex->GetBlockHash());
}

void CLeasingManager::SyncTransaction(const CTransaction& tx, const CBlock* pBlock) {
   if (tx.IsCoinBase() || tx.IsCoinStake()) {
      // there is no P2L or LR transactions
      return;
   }

   uint32_t nPosition;
   if (nullptr != pBlock) {
      if (tx.IsLeasingReward()) {
         nPosition = 0;
         for (auto& txOut: tx.vout)
            pImpl->AddLeasingReward(tx, txOut, nPosition++);
      } else {
         nPosition = 0;
         for (const CTxOut& txOut: tx.vout) {
            if (txOut.scriptPubKey.IsPayToLeasing())
               pImpl->AddLeasingOutput(tx, txOut, nPosition);
            ++nPosition;
         }

         nPosition = 0;
         for (const CTxIn& txIn: tx.vin)
            pImpl->AddLeasingSpend(tx, txIn.prevout, nPosition++);
      }
   } else {
      if (tx.IsLeasingReward()) {
         nPosition = 0;
         for (auto& txOut: tx.vout)
            pImpl->RemoveLeasingReward(tx, txOut, nPosition++);
      } else {
         nPosition = 0;
         for (const CTxOut& txOut: tx.vout) {
            if (txOut.scriptPubKey.IsPayToLeasing())
               pImpl->RemoveLeasingOutput(tx, nPosition);
            ++nPosition;
         }

         nPosition = 0;
         for (const CTxIn& txIn: tx.vin)
            pImpl->RemoveLeasingSpend(tx, txIn.prevout, nPosition++);
      }
   }
}

bool CLeasingManager::GetLeasingRewards(
    const LeaserType type,
    const CKeyID& leaserID,
    const size_t nLimit,
    std::vector<CTxOut>& vRewards
) const {
   return pImpl->GetLeasingRewards(type, leaserID, nLimit, vRewards);
}

CTxOut CLeasingManager::CalcLeasingReward(const COutPoint& point, const CKeyID& keyID) const {
   return pImpl->CalcLeasingReward(point, keyID);
}

void CLeasingManager::GetAllAmountsLeasedTo(CPubKey &pubKey, CAmount &amount) const {
   pImpl->GetAllAmountsLeasedTo(pubKey, amount);
}

const uint256& CLeasingManager::GetBlockHash() const {
    return pImpl->GetBlockHash();
}

#endif // ENABLE_LEASING_MANAGER
