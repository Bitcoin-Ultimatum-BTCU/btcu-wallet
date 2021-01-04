// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2015-2019 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "chainparams.h"
#include "db.h"
#include "init.h"
#include "main.h"
#include "masternode-budget.h"
#include "masternode-payments.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "rpc/server.h"
#include "utilmoneystr.h"

#include <univalue.h>
#include <consensus/validator_tx_verify.h>
#include <boost/assign/list_of.hpp>
#include <leasing/leasing_tx_verify.h>

#include "leasing/leasingmanager.h"

void SendMoney(const CTxDestination& address, CAmount nValue, CWalletTx& wtxNew, bool fUseIX = false,
        const std::vector<CValidatorRegister> &validatorRegister = std::vector<CValidatorRegister>(),
        const std::vector<CValidatorVote> &validatorVote = std::vector<CValidatorVote>());

boost::optional<CKey> GetCollateralKey(CMasternode *pmn)
{
    boost::optional<CKey> keyOpt;
    CKey key;
    
    auto addr = pmn->pubKeyCollateralAddress.GetID(); // public key ID for MN's collateral address
    if (pwalletMain->GetKey(addr, key)) // get key (private and public parts) from wallet
    {
//        auto addr_str = CBTCUAddress(key.GetPubKey().GetID()).ToString();
        keyOpt.emplace(key);
    }
    return keyOpt;
}

boost::optional<std::pair<CTxIn, CKey>> GetVinKey(const std::string &strAlias)
{
    boost::optional<std::pair<CTxIn, CKey>> vinKeyOpt;
    
    for (CMasternodeConfig::CMasternodeEntry &mne : masternodeConfig.getEntries())
    {
        if( strAlias == mne.getAlias())
        {
            CKey keyMasternode;
            CPubKey pubKeyMasternode;
            
            if(CMessageSigner::GetKeysFromSecret(mne.getPrivKey(), keyMasternode, pubKeyMasternode))
            {
                CMasternode *pmn = mnodeman.Find(pubKeyMasternode);
                if(pmn != nullptr)
                {
                    auto keyOpt = GetCollateralKey(pmn);
                    if(keyOpt.is_initialized())
                    {
                       vinKeyOpt.emplace(std::pair<CTxIn, CKey>(pmn->vin, keyOpt.value()));
                       break;
                    }
                }
            }
        }
    }
    return vinKeyOpt;
}

// Tries to get secret key which corresponds to one of registered validators keys
boost::optional<std::pair<CTxIn, CKey>> GetRegisteredValidatorVinKey()
{
   boost::optional<std::pair<CTxIn, CKey>> vinKeyOpt;

   for (CMasternodeConfig::CMasternodeEntry &mne : masternodeConfig.getEntries())
   {
      CKey keyMasternode;
      CPubKey pubKeyMasternode;
      if(CMessageSigner::GetKeysFromSecret(mne.getPrivKey(), keyMasternode, pubKeyMasternode))
         {
            CMasternode *pmn = mnodeman.Find(pubKeyMasternode);
            if(pmn != nullptr)
            {
               auto keyOpt = GetCollateralKey(pmn);
               if(keyOpt.is_initialized())
               {
                  auto validatorsRegistrationList = g_ValidatorsState.get_validators();
                  for(auto &validator: validatorsRegistrationList)
                  {
                     if(validator.pubKey == keyOpt.value().GetPubKey())
                     {
                        vinKeyOpt.emplace(std::pair<CTxIn, CKey>(pmn->vin, keyOpt.value()));
                        return vinKeyOpt;
                     }
                  }
               }
            }
         }
   }
   return vinKeyOpt;
}

// Tries to get secret key which corresponds to one of registering validators keys
boost::optional<std::pair<CTxIn, CKey>> GetRegisteringValidatorVinKey()
{
   boost::optional<std::pair<CTxIn, CKey>> vinKeyOpt;

   for (CMasternodeConfig::CMasternodeEntry &mne : masternodeConfig.getEntries())
   {
      CKey keyMasternode;
      CPubKey pubKeyMasternode;
      if(CMessageSigner::GetKeysFromSecret(mne.getPrivKey(), keyMasternode, pubKeyMasternode))
      {
         CMasternode *pmn = mnodeman.Find(pubKeyMasternode);
         if(pmn != nullptr)
         {
            auto keyOpt = GetCollateralKey(pmn);
            if(keyOpt.is_initialized())
            {
               auto validatorsRegistrationList = g_ValidatorsState.get_registrations();
               for(auto &validator: validatorsRegistrationList)
               {
                  if(validator.pubKey == keyOpt.value().GetPubKey())
                  {
                     vinKeyOpt.emplace(std::pair<CTxIn, CKey>(pmn->vin, keyOpt.value()));
                     return vinKeyOpt;
                  }
               }
            }
         }
      }
   }
   return vinKeyOpt;
}

// Tries to get secret key which corresponds to one of registered validators keys
std::string GetMNAliasFromVin(CTxIn mnVin)
{
   std::string strAlias;

   for (CMasternodeConfig::CMasternodeEntry &mne : masternodeConfig.getEntries())
   {
      CKey keyMasternode;
      CPubKey pubKeyMasternode;
      if(CMessageSigner::GetKeysFromSecret(mne.getPrivKey(), keyMasternode, pubKeyMasternode))
      {
         CMasternode *pmn = mnodeman.Find(pubKeyMasternode);
         if(pmn != nullptr)
         {
            if(pmn->vin == mnVin)
            {
               strAlias = mne.getAlias();
               break;
            }
         }
      }
   }
   return strAlias;
}

// Tries to get secret key which corresponds to one of genesis validators keys
boost::optional<std::pair<CTxIn, CKey>> GetGenesisVinKey()
{
    boost::optional<std::pair<CTxIn, CKey>> vinKeyOpt;
    
    auto genesisValidators = Params().GenesisBlock().vtx[0].validatorRegister;
    for(auto &gv : genesisValidators)
    {
        CKey key;
        if(pwalletMain->GetKey(gv.pubKey.GetID(), key))
        {
            vinKeyOpt.emplace(std::pair<CTxIn, CKey>(gv.vin, key));
            break;
        }
    }
    return vinKeyOpt;
}

boost::optional<CValidatorRegister> CreateValidatorReg(const std::string &strAlias)
{
    boost::optional<CValidatorRegister> valRegOpt;
    
    auto keyOpt = GetVinKey(strAlias);
    if(keyOpt.is_initialized())
    {
        auto vinKey = keyOpt.value();
        
        CValidatorRegister valReg(vinKey.first, vinKey.second.GetPubKey());
        if(valReg.Sign(vinKey.second))
        {
            valRegOpt.emplace(valReg);
        }
    }
    return valRegOpt;
}

boost::optional<CValidatorVote> CreateValidatorVote(const std::vector<MNVote> &votes)
{
    boost::optional<CValidatorVote> valVoteOpt;
    
    auto keyOpt = GetRegisteringValidatorVinKey();
    if(keyOpt.is_initialized())
    {
        auto vinKey = keyOpt.value();
        
        CValidatorVote valVote(vinKey.first, vinKey.second.GetPubKey(), votes);
        if(valVote.Sign(vinKey.second))
        {
            valVoteOpt.emplace(valVote);
        }
    }
    return valVoteOpt;
}

// Votes generator for testing purposes
std::vector<MNVote> SetVotes()
{
    auto validatorsRegistrationList = g_ValidatorsState.get_registrations();
    
    std::vector<MNVote> votes;
    for(auto &valReg : validatorsRegistrationList)
    {
        votes.emplace_back(valReg.vin, VoteYes);
    }
    return votes;
}

UniValue CreateAndSendTransaction(const boost::optional<CValidatorRegister> &valRegOpt, const boost::optional<CValidatorVote> &valVoteOpt)
{
   CCoinsView viewDummy;
   CCoinsViewCache view(&viewDummy);
   {
      LOCK(mempool.cs);
      CCoinsViewMemPool viewMempool(pcoinsTip, mempool);
      view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

      if (valRegOpt.is_initialized() && valVoteOpt.is_initialized())
      {
         return UniValue("transaction can't be for registration and for voting at the same time");
      }

      std::vector<CValidatorRegister> valReg;
      std::vector<CValidatorVote> valVote;

      CTxIn vin;
      if (valRegOpt.is_initialized())
      {
         //check leased to candidate coins
#ifdef ENABLE_LEASING_MANAGER
         assert(pwalletMain != NULL);
         LOCK2(cs_main, pwalletMain->cs_wallet);
         if(pwalletMain->pLeasingManager)
         {
            CAmount amount;
            CPubKey pubkey = valRegOpt.value().pubKey;
            pwalletMain->pLeasingManager->GetAllAmountsLeasedTo(pubkey, amount);

            if(amount < LEASED_TO_VALIDATOR_MIN_AMOUNT)
               return UniValue("Not enough leased to validator candidate coins, min=" + std::to_string(LEASED_TO_VALIDATOR_MIN_AMOUNT) +
               ", current=" + std::to_string(amount) + ", validator pubkey=" + HexStr(pubkey));
         }
#endif
         valReg.push_back(valRegOpt.value());
         vin = valRegOpt.value().vin;
      }
      else if (valVoteOpt.is_initialized())
      {
         valVote.push_back(valVoteOpt.value());
         vin = valVoteOpt.value().vin;
      }

      //Check minimum age of mn vin
      const CCoins* unspentCoins = view.AccessCoins(vin.prevout.hash);
      int age = GetCoinsAge(unspentCoins);
      if (age < MASTERNODE_MIN_CONFIRMATIONS)
         return UniValue("Masternode vin minimum confirmation is:" + std::to_string(MASTERNODE_MIN_CONFIRMATIONS) +
                         ", but now vin age = " + std::to_string(age));


      // Get own address from wallet to send btcu to
      CReserveKey reservekey(pwalletMain);
      CPubKey vchPubKey;
      assert(reservekey.GetReservedKey(vchPubKey));
      CTxDestination myAddress = vchPubKey.GetID();

      CAmount nAmount = AmountFromValue(
      UniValue((double) 38 / COIN)); // send 38 satoshi (min tx fee per kb is 100 satoshi)
      CWalletTx wtx;

      EnsureWalletIsUnlocked();
      // Create and send transaction
      SendMoney(myAddress, nAmount, wtx, false, valReg, valVote);

      // Get hash of the created transaction
      return UniValue(wtx.GetHash().GetHex());
   }
}

UniValue mnregvalidator(const UniValue& params, bool fHelp)
{

   // Here we create transaction that contains CValidatorRegister and sends minimal amount of btcu to MN's own address.
   // This is needed to pay transaction fee to miner.
   if (fHelp || params.size() != 1)
      throw std::runtime_error(
      "mnregvalidator ( \"account-name\" )\n"
      "\nApply to register a validator\n"

      "\nArguments:\n"
      "1. \"account-name\"    (string, required) Masternode account name\n"

      "\nResult:\n"
      "[\n"
      "  {\n"
      "hash    (string) Return hash of the created voting transaction\n"
      "  }\n"
      "  ,...\n"
      "]\n"

      "\nExamples:\n" +
      HelpExampleCli("mnregvalidator", "MN") + HelpExampleRpc("mnregvalidator", "MN"));

   std::string strAlias = params[0].get_str();

   UniValue ret(UniValue::VOBJ);
   int currentPositionInVotingPeriod = (chainActive.Height() + 1) % VALIDATORS_VOTING_PERIOD_LENGTH;  // +1 due to current transaction should be included at least into the next block
    
   // Checking that it is corresponding phase in the current voting period
   if((currentPositionInVotingPeriod >= VALIDATORS_REGISTER_START) &&
   (currentPositionInVotingPeriod <= VALIDATORS_REGISTER_END))
    {
        boost::optional<CValidatorRegister> valRegOpt = CreateValidatorReg(strAlias);
        if (valRegOpt.is_initialized()){
            ret = CreateAndSendTransaction(valRegOpt, boost::optional<CValidatorVote>());
        } else {
           throw JSONRPCError(RPC_INVALID_PARAMETER, "CreateValidatorReg failed");
        }
    } else {
      ret = std::to_string(VALIDATORS_VOTING_PERIOD_LENGTH - currentPositionInVotingPeriod) + " blocks until start of the registration phase";
   }
   return ret;
}

UniValue mnvotevalidator(const UniValue& params, bool fHelp)
{

    UniValue ret(UniValue::VOBJ);
    int currentPositionInVotingPeriod = (chainActive.Height() + 1) % VALIDATORS_VOTING_PERIOD_LENGTH; // +1 due to current transaction should be included at least into the next block
    
    // Checking that it is corresponding phase in the current voting period
    if((currentPositionInVotingPeriod >= VALIDATORS_VOTING_START) &&
       (currentPositionInVotingPeriod <= VALIDATORS_VOTING_END))
    {

       if (fHelp || params.size() != 1)
          throw std::runtime_error(
          "mnvotevalidator [{\"pubkey\":\"pubkeyhex\",\"vote\":yes|no},...]\n"
          "\nVoting for validators from registered list.\n"
          "List of registered validators you can see by calling mnregvalidatorlist\n"

          "\nArguments:\n"
          "1. \"valvote\"  (string, required) A json array of objects. Each object the pubkey hex (string) yes|no vote (string)\n"
          "     [           (json array of json objects)\n"
          "       {\n"
          "         \"pubkey\":\"pubkeyhex\",    (string) Registered validators public key\n"
          "         \"votecast\": \"yes|no\"     (string) Your vote. 'yes' to vote for the validator, 'no' to vote againstr\n"
          "       }\n"
          "       ,...\n"
          "     ]\n"

          "\nResult:\n"
          "hash    (string) Return hash of the created voting transaction\n"

          "\nExamples:\n"
          "\nList the registered validators\n" +
          HelpExampleCli("mnregvalidatorlist", "") +
          "\nVote for validators\n" +
          HelpExampleCli("mnvotevalidator", "\"[{\\\"pubkey\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vote\\\":\\\"yes\\\"}]\"") +
          "\nAs a json rpc call\n" +
          HelpExampleRpc("mnvotevalidator", "\"[{\\\"pubkey\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vote\\\":\\\"yes\\\"}]\""));

       RPCTypeCheck(params, boost::assign::list_of(UniValue::VARR));
       UniValue vote_params = params[0].get_array();

       //Create and validate pukey and vot8es objects
       std::vector<MNVote> votes;
       votes.reserve(vote_params.size());
       auto validatorsRegistrationList = g_ValidatorsState.get_registrations();

       for (unsigned int idx = 0; idx < vote_params.size(); idx++) {
          const UniValue& vote = vote_params[idx];
          if (!vote.isObject())
             throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected object");
          const UniValue& o = vote.get_obj();

          RPCTypeCheckObj(o, boost::assign::map_list_of("pubkey", UniValue::VSTR)("vote", UniValue::VSTR));

          const std::string& strPubKey = find_value(o, "pubkey").get_str();
          if (!IsHex(strPubKey)) {
             throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected hex pubkey");
          }

          std::string strVote = find_value(o, "vote").get_str();

          if (strVote != "yes" && strVote != "no") throw JSONRPCError(RPC_INVALID_PARAMETER,  "You can only vote 'yes' or 'no'");
          int nVote = VoteAbstain;
          if (strVote == "yes") nVote = VoteYes;
          if (strVote == "no") nVote = VoteNo;

          const CPubKey vchPubKey(ParseHex(strPubKey));

          if (!vchPubKey.IsFullyValid())
          {
             throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, pubkey: " + strPubKey);
          }

          auto isValidatorExists = [vchPubKey](CValidatorRegister v_reg) -> bool {
             return (vchPubKey == v_reg.pubKey);
          };

          auto countIter = std::find_if(validatorsRegistrationList.begin(), validatorsRegistrationList.end(), isValidatorExists);

          if(countIter != validatorsRegistrationList.end())
             votes.emplace_back(countIter->vin, nVote);
       }
        
        if(!votes.empty()){
           // Here we create transaction that contains CValidatorVote and sends minimal amount of btcu to MN's own address.
           // This is needed to pay transaction fee to miner.
           boost::optional<CValidatorVote> valVoteOpt = CreateValidatorVote(votes);
           if (valVoteOpt.is_initialized()){
                ret = CreateAndSendTransaction(boost::optional<CValidatorRegister>(), valVoteOpt);
            } else {
              throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to get validator masternode key: CreateValidatorVote failed. Check your masternode status.");
           }
        } else {
           throw JSONRPCError(RPC_INVALID_PARAMETER, "Votes are empty");
        }
    } else {
        ret = std::to_string(VALIDATORS_VOTING_START - currentPositionInVotingPeriod) + " blocks until start of the voting phase";
    }
    return ret;
}

UniValue mnregvalidatorlist(const UniValue& params, bool fHelp)
{
    auto validatorsRegistrationList = g_ValidatorsState.get_registrations();
    std::string valRegStr;
    for(auto &valReg : validatorsRegistrationList)
       valRegStr += " 1. PubKey: " +  HexStr(valReg.pubKey) + "\nVin: " + valReg.vin.ToString() + "\n";
    return UniValue(valRegStr);
}

UniValue mnvotevalidatorlist(const UniValue& params, bool fHelp)
{
    auto validatorsVotesList = g_ValidatorsState.get_votes();
    std::string valVoteStr;
    for(auto &valVote : validatorsVotesList)
       valVoteStr += " 1. PubKey: " +  HexStr(valVote.pubKey) + "\nVin: " + valVote.vin.ToString() + "\n";
    return UniValue(valVoteStr);
}

UniValue mnvalidatorlist(const UniValue& params, bool fHelp)
{
    auto validatorsList = g_ValidatorsState.get_validators();
    
    std::string valStr;
    for(auto &val : validatorsList)
       valStr += " 1. PubKey: " +  HexStr(val.pubKey) + "\nVin: " + val.vin.ToString() + "\n";
    return UniValue(valStr);
}