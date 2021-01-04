#ifndef BTCU_VALIDATORS_STATE_H
#define BTCU_VALIDATORS_STATE_H

#include <vector>
#include "sync.h"
#include "masternode-validators.h"
#include "primitives/transaction.h"
#include "util.h"

boost::filesystem::path GetValidatorsDir();

class CValidatorsState
{
public:
    
    std::vector<CValidatorInfo> get_validators()
    {
        LOCK(cs_ValidatorsList);
        return validatorsList;
    }
    
    std::vector<CValidatorRegister> get_registrations()
    {
        LOCK(cs_ValidatorsRegistrationList);
        return validatorsRegistrationList;
    }
    
    std::vector<CValidatorVote> get_votes()
    {
        LOCK(cs_ValidatorsVotesList);
        return validatorsVotesList;
    }
    
    void set_validators(const std::vector<CValidatorInfo> &validatorsListIn)
    {
        LOCK(cs_ValidatorsList);
        validatorsList = validatorsListIn;
    }
    
    void add_registration(const CValidatorRegister &validatorRegisterIn)
    {
        LOCK(cs_ValidatorsRegistrationList);
        validatorsRegistrationList.push_back(validatorRegisterIn);
    }
    
    void add_vote(const CValidatorVote &validatorVoteIn)
    {
        LOCK(cs_ValidatorsVotesList);
        validatorsVotesList.push_back(validatorVoteIn);
    }
    
    bool remove_registration(const CValidatorRegister &validatorRegisterIn);
    bool remove_vote(const CValidatorVote &validatorVoteIn);
    
    void clear_registrations()
    {
        LOCK(cs_ValidatorsRegistrationList);
        validatorsRegistrationList.clear();
    }
    
    void clear_votes()
    {
        LOCK(cs_ValidatorsVotesList);
        validatorsVotesList.clear();
    }
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        LOCK(cs_ValidatorsRegistrationList);
        LOCK(cs_ValidatorsVotesList);
        LOCK(cs_ValidatorsList);
        
        READWRITE(validatorsRegistrationList);
        READWRITE(validatorsVotesList);
        READWRITE(validatorsList);
    }
    
    bool load (const boost::filesystem::path &fileName = (GetValidatorsDir() / "state"));
    bool flush(const boost::filesystem::path &fileName = (GetValidatorsDir() / "state"));
    
private:
    
    CCriticalSection cs_ValidatorsRegistrationList;
    std::vector<CValidatorRegister> validatorsRegistrationList;
    
    CCriticalSection cs_ValidatorsVotesList;
    std::vector<CValidatorVote> validatorsVotesList;
    
    CCriticalSection cs_ValidatorsList;
    std::vector<CValidatorInfo> validatorsList;
};

#endif // BTCU_VALIDATORS_STATE_H
