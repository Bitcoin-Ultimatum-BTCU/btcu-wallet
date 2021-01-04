#include "validators_voting.h"
#include "validators_state.h"
#include <consensus/validator_tx_verify.h>
#include "blocksignature.h"
#include "main.h"
#include "spork.h"

std::vector<std::pair<int, CTxIn>> CountVotes()
{
    auto validatorsVotesList = g_ValidatorsState.get_votes();
    
    std::vector<std::pair<int, CTxIn>> count;
    for(auto &vote : validatorsVotesList)
    {
        for(auto &v : vote.votes)
        {
            auto isVin = [v](std::pair<int, CTxIn> c) -> bool {
                return (c.second == v.vin);
            };
            auto countIter = std::find_if(count.begin(), count.end(), isVin);
            
            if(countIter == count.end()){
                count.emplace_back(v.vote, vote.vin);
            } else {
                countIter->first += v.vote;
            }
        }
    }
    return count;
}

// Compose validators list according to the predefined rules and voting results
std::vector<CValidatorInfo> ComposeValidatorsList(std::vector<std::pair<int, CTxIn>> count)
{
    // sort vector firstly by score and secondly by vin, if scores are equal
    std::sort(count.begin(), count.end());
    
    // get public key from registration list by vin
    auto registeredPubKey = [](const CTxIn &vin) -> CPubKey
    {
        auto validatorsRegistrationList = g_ValidatorsState.get_registrations();
        auto isVin = [vin](CValidatorRegister valReg) -> bool {
            return (valReg.vin == vin);
        };
        auto valRegIter = std::find_if(validatorsRegistrationList.begin(), validatorsRegistrationList.end(), isVin);
        // here vin should always be existing, due to all calculations are initially based on the g_ValidatorsRegistrationList
        return valRegIter->pubKey;
    };

     //get spork value max validators count
     int nMaxValidators = sporkManager.GetSporkValue(SPORK_1018_MAX_VALIDATORS);
     nMaxValidators = nMaxValidators > 0 ? nMaxValidators: MAX_VALIDATORS_DEFAULT;

    // get validators with highest score (first nMaxValidators from the sorted by score list)
    std::vector<CValidatorInfo> validators;
    for(auto &c : count)
    {
        auto vin = c.second;
        validators.emplace_back(vin, registeredPubKey(vin));
        
        if(validators.size() == nMaxValidators){ // if the list is full
            break;
        }
    }
    return validators;
}

boost::filesystem::path GetSnapshotName(const uint256 &blockHash)
{
    return (GetValidatorsDir() / blockHash.ToString());
}

void UpdateValidators(CBlockIndex *pBlockIndex, bool bForward)
{
    if(pBlockIndex->nHeight % VALIDATORS_VOTING_PERIOD_LENGTH == VALIDATORS_VOTING_END)
    {
        if(bForward)
        {
            // make snapshot of the current voting state and the validators list from a previous voting period before they are both cleared
            g_ValidatorsState.flush(GetSnapshotName(pBlockIndex->GetBlockHash()));
            
            // count votes, create and set new validators list
            g_ValidatorsState.set_validators(ComposeValidatorsList(CountVotes()));
    
            // the next block is the beginning of a new voting period, so reset the voting state beforehand
            g_ValidatorsState.clear_registrations();
            g_ValidatorsState.clear_votes();
    
            g_ValidatorsState.flush();
        }
        else
        {
            // restore snapshot of the specific voting state
            g_ValidatorsState.load(GetSnapshotName(pBlockIndex->GetBlockHash()));
        }
    }
}

void UpdateValidatorsVotingState(const CTransaction& tx, bool bForward)
{
    if(tx.IsValidatorRegister())
    {
        if(bForward){
            g_ValidatorsState.add_registration(tx.validatorRegister.front());
        } else {
            g_ValidatorsState.remove_registration(tx.validatorRegister.front());
        }
        g_ValidatorsState.flush();
    }
    else if(tx.IsValidatorVote())
    {
        if(bForward){
            g_ValidatorsState.add_vote(tx.validatorVote.front());
        } else {
            g_ValidatorsState.remove_vote(tx.validatorVote.front());
        }
        g_ValidatorsState.flush();
    }
}

boost::optional<CValidatorInfo> GetValidatorInfo(const CTxIn &validatorVin)
{
    auto validatorsList = g_ValidatorsState.get_validators();
    
    auto is_validatorVin = [validatorVin](CValidatorInfo valInfo) -> bool {
        return (valInfo.vin == validatorVin);
    };
    
    boost::optional<CValidatorInfo> infoOpt;
    
    auto valInfoIter = std::find_if(validatorsList.begin(), validatorsList.end(), is_validatorVin);
    if(valInfoIter != validatorsList.end()){
        infoOpt.emplace(*valInfoIter);
    }
    return infoOpt;
}

boost::optional<CPubKey> GetValidatorPubKey(const CTxIn &validatorVin)
{
    boost::optional<CPubKey> pubKeyOpt;
    
    auto infoOpt = GetValidatorInfo(validatorVin);
    if(infoOpt.is_initialized()){
        pubKeyOpt.emplace(infoOpt.value().pubKey);
    }
    return pubKeyOpt;
}

boost::optional<CPubKey> GetGenesisValidatorPubKey(const CTxIn &validatorVin)
{
    boost::optional<CPubKey> pubKeyOpt;
    
    if(VinIsGenesis(validatorVin))
    {
        auto genesisValidators = Params().GenesisBlock().vtx[0].validatorRegister;
        for(auto &gv : genesisValidators)
        {
            if(gv.vin == validatorVin)
            {
                pubKeyOpt.emplace(gv.pubKey);
                break;
            }
        }
    }
    return pubKeyOpt;
}

bool VinIsGenesis(const CTxIn &vin)
{
    return (vin.prevout.hash == uint256(0));
}

bool VinIsUnspent(const CTxIn &vin, const CCoinsViewCache &view)
{
    // check for presence it UTXO set
    return (view.AccessCoins(vin.prevout.hash) != nullptr);
}

bool CheckValidator(const CBlock& block, const CCoinsViewCache &view)
{
    auto vinIsUnspent = VinIsGenesis(block.validatorVin) ? true : VinIsUnspent(block.validatorVin, view);
    
    // for a regular validator a VIN should be unspent and signature can be checked only if validator is in g_ValidatorsList (public key is retrieved from it)
    return (vinIsUnspent && CheckValidatorSignature(block));
}
