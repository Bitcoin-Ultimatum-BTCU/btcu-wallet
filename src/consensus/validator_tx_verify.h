// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2015-2019 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCU_VALIDATOR_TX_VERIFY_H
#define BTCU_VALIDATOR_TX_VERIFY_H

#define VALIDATORS_VOTING_PERIOD_LENGTH 20
#define VALIDATORS_REGISTER_START 0
#define VALIDATORS_REGISTER_END 9
#define VALIDATORS_VOTING_START (VALIDATORS_REGISTER_END + 1)
#define VALIDATORS_VOTING_END (VALIDATORS_VOTING_PERIOD_LENGTH - 1)

class CCoinsViewCache;
class CTransaction;
class CValidationState;

/** Transaction validation functions */

/** Context-dependent validity checks for validator-voting related transactions */
bool CheckValidatorTransaction(
        const CTransaction &tx,
        CValidationState &state,
        const CCoinsViewCache &view,
        int nHeight,
        std::vector<CTransaction> &validatorTransactionsInCurrentBlock);

int GetCoinsAge(const CCoins *coins);

#endif //BTCU_VALIDATOR_TX_VERIFY_H
