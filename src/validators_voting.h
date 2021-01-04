#ifndef BTCU_VALIDATORS_VOTING_H
#define BTCU_VALIDATORS_VOTING_H

#include <primitives/tx_in_out.h>
#include "coins.h"

void UpdateValidators(CBlockIndex *pBlockIndex, bool bForward = true);
void UpdateValidatorsVotingState(const CTransaction& tx, bool bForward = true);
bool CheckValidator(const CBlock& block, const CCoinsViewCache &view);
boost::optional<CValidatorInfo> GetValidatorInfo(const CTxIn &validatorVin);
boost::optional<CPubKey> GetValidatorPubKey(const CTxIn &validatorVin);
boost::optional<CPubKey> GetGenesisValidatorPubKey(const CTxIn &validatorVin);
bool VinIsGenesis(const CTxIn &vin);
bool VinIsUnspent(const CTxIn &vin, const CCoinsViewCache &view);

#endif // BTCU_VALIDATORS_VOTING_H