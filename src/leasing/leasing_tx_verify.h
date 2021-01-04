// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BTCU_LEASING_TX_VERIFY_H
#define BTCU_LEASING_TX_VERIFY_H

#if defined(HAVE_CONFIG_H)
#include "config/btcu-config.h"
#endif

#ifdef ENABLE_LEASING_MANAGER

#define LEASED_TO_VALIDATOR_MIN_AMOUNT 10000

class CTransaction;
class CLeasingManager;
class CValidationState;

bool CheckLeasingRewardTransaction(const CTransaction& tx, CValidationState& state, const CLeasingManager& leasingManager);
bool CheckLeasedToValidatorTransaction(const CTransaction& tx, CValidationState& state, const CLeasingManager& leasingManager);

#endif // ENABLE_LEASING_MANAGER

#endif // BTCU_LEASING_TX_VERIFY_H