#include <fstream>
#include <boost/filesystem/operations.hpp>
#include "primitives/transaction.h"
#include "clientversion.h"
#include "validators_state.h"

boost::filesystem::path GetValidatorsDir()
{
    auto path((GetDataDir() / "validators"));
    if(!boost::filesystem::exists(path)){
        boost::filesystem::create_directory(path);
    }
    return path;
}

bool CValidatorsState::remove_registration(const CValidatorRegister &validatorRegisterIn)
{
    LOCK(cs_ValidatorsRegistrationList);
    auto status = false;
    
    auto iter = std::find(validatorsRegistrationList.begin(), validatorsRegistrationList.end(), validatorRegisterIn);
    if(iter != validatorsRegistrationList.end())
    {
        validatorsRegistrationList.erase(iter);
        status = true;
    }
    return status;
}

bool CValidatorsState::remove_vote(const CValidatorVote &validatorVoteIn)
{
    LOCK(cs_ValidatorsVotesList);
    auto status = false;
    
    auto iter = std::find(validatorsVotesList.begin(), validatorsVotesList.end(), validatorVoteIn);
    if(iter != validatorsVotesList.end())
    {
        validatorsVotesList.erase(iter);
        status = true;
    }
    return status;
}

bool CValidatorsState::load(const boost::filesystem::path &fileName)
{
    bool status = false;
    
    std::fstream file(fileName.string(), std::fstream::in | std::ios::binary);
    if(file)
    {
        Unserialize(file, SER_DISK, CLIENT_VERSION);
        file.close();
        status = true;
    }
    return status;
}

bool CValidatorsState::flush(const boost::filesystem::path &fileName)
{
    bool status = false;

    std::fstream file(fileName.string(), std::fstream::out | std::ios::binary);
    if(file)
    {
        Serialize(file, SER_DISK, CLIENT_VERSION);
        file.flush();
        file.close();
        status = true;
    }
    return status;
}
