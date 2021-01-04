#!/usr/bin/env python3
# Copyright (c) 2019 The PIVX developers
# Copyright (c) 2020 The BTCU developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# -*- coding: utf-8 -*-

from io import BytesIO
from time import sleep

from test_framework.mininode import network_thread_start
from test_framework.btcu_node import BtcuTestNode
from test_framework.script import CScript, OP_CHECKSIG, OP_LEASINGREWARD
from test_framework.test_framework import BtcuTestFramework
from test_framework.util import connect_nodes_bi, p2p_port, bytes_to_hex_str, set_node_times, \
    assert_equal, assert_greater_than, sync_blocks, sync_mempools, assert_raises_rpc_error

# filter utxos based on first 5 bytes of scriptPubKey
def getDelegatedUtxos(utxos):
    return [x for x in utxos if x["scriptPubKey"][:10] == '76a97b63d1']

# filter utxos based on first 5 bytes of scriptPubKey
def getLeasedUtxos(utxos):
    return [x for x in utxos if x["scriptPubKey"][:10] == '76a97b63d7']

# filter utxos based on opcodes
def getLeasingRewards(node):
    utxos = node.listunspent()
    rewards = []
    for x in utxos:
        script = CScript(bytes.fromhex(x["scriptPubKey"]))
        i = 0
        for op in script:
            if i == 0 and not isinstance(op, bytes):
                break
            # in this test no big number of outputs
            elif i == 1 and not (op, int):
                break
            elif i == 2:
                if op == OP_LEASINGREWARD:
                    rewards.append(x)
                break
            i += 1

    return rewards


class BTCU_LeasingTest(BtcuTestFramework):

    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [[]] * self.num_nodes
        self.extra_args[0].append('-sporkkey=932HEevBSujW2ud7RfB1YF91AFygbBRQj3de3LyaCRqNzKKgWXi')

    def setup_chain(self):
        # Start with PoW cache: 200 blocks
        self._initialize_chain()
        self.enable_mocktime()


    def setup_network(self):
        ''' Can't rely on syncing all the nodes when staking=1
        '''
        self.setup_nodes()
        for i in range(self.num_nodes - 1):
            for j in range(i+1, self.num_nodes):
                connect_nodes_bi(self.nodes, i, j)

    def init_test(self):
        title = "*** Starting %s ***" % self.__class__.__name__
        underline = "-" * len(title)
        self.log.info("\n\n%s\n%s\n%s\n", title, underline, self.description)
        self.DEFAULT_FEE = 0.00005
        # Setup the p2p connections and start up the network thread.
        self.test_nodes = []
        for i in range(self.num_nodes):
            self.test_nodes.append(BtcuTestNode())
            self.test_nodes[i].peer_connect('127.0.0.1', p2p_port(i))

        network_thread_start()  # Start up network handling in another thread

        # Let the test nodes get in sync
        for i in range(self.num_nodes):
            self.test_nodes[i].wait_for_verack()

    def setColdStakingEnforcement(self, fEnable=True):
        sporkName = "SPORK_17_COLDSTAKING_ENFORCEMENT"
        # update spork 17 with node[0]
        if fEnable:
            self.log.info("Enabling cold staking with SPORK 17...")
            res = self.activate_spork(0, sporkName)
        else:
            self.log.info("Disabling cold staking with SPORK 17...")
            res = self.deactivate_spork(0, sporkName)
        assert_equal(res, "success")
        sleep(1)
        # check that node[1] receives it
        assert_equal(fEnable, self.is_spork_active(1, sporkName))
        self.log.info("done")

    def isColdStakingEnforced(self):
        # verify from node[1]
        return self.is_spork_active(1, "SPORK_17_COLDSTAKING_ENFORCEMENT")

    def setLeasingEnforcement(self, fEnable=True):
        sporkName = "SPORK_1017_LEASING_ENFORCEMENT"
        # update spork 17 with node[0]
        if fEnable:
            self.log.info("Enabling leasing with SPORK 1017...")
            res = self.activate_spork(0, sporkName)
        else:
            self.log.info("Disabling leasing with SPORK 1017...")
            res = self.deactivate_spork(0, sporkName)
        assert_equal(res, "success")
        sleep(1)
        # check that node[1] receives it
        assert_equal(fEnable, self.is_spork_active(1, sporkName))
        self.log.info("done")

    def isLeasingEnforced(self):
        # verify from node[1]
        return self.is_spork_active(1, "SPORK_1017_LEASING_ENFORCEMENT")



    def run_test(self):
        self.description = "Performs tests on the Leasing P2L implementation"
        self.init_test()
        NUM_OF_INPUTS = 20
        INPUT_VALUE = 249
        LEASING_REWARD_PERIOD = 10
        LEASING_REWARDS_PER_BLOCK = 10
        LEASING_MATURITY = 3
        self.noreward_period = LEASING_MATURITY + LEASING_REWARD_PERIOD
        self.expected_delegated_balance = 0
        self.expected_immature_delegated_balance = 0
        self.expected_leased_balance = 0
        self.expected_immature_leased_balance = 0

        # nodes[0] - coin-owner
        # nodes[1] - cold-staker and leaser

        # 1) nodes[0] and nodes[2] mine 25 blocks each
        # --------------------------------------------
        print("*** 1 ***")
        self.log.info("Mining 50 Blocks...")
        for peer in [0, 2]:
            for j in range(25):
                self.mocktime = self.generate_pow(peer, self.mocktime)
            sync_blocks(self.nodes)

        # 2) node[1] sends his entire balance (50 mature rewards) to node[2]
        #  - node[2] stakes a block - node[1] locks the change
        print("*** 2 ***")
        self.log.info("Emptying node1 balance")
        assert_equal(self.nodes[1].getbalance(), 50 * 250)
        txid = self.nodes[1].sendtoaddress(self.nodes[2].getnewaddress(), (50 * 250 - 0.01))
        assert (txid is not None)
        sync_mempools(self.nodes)
        self.mocktime = self.generate_pos(2, self.mocktime)
        sync_blocks(self.nodes)
        # lock the change output (so it's not used as stake input in generate_pos)
        for x in self.nodes[1].listunspent():
            assert (self.nodes[1].lockunspent(False, [{"txid": x['txid'], "vout": x['vout']}]))
        # check that it cannot stake
        sleep(1)
        assert_equal(self.nodes[1].getstakingstatus()["stakeablecoins"], False)

        # 3) nodes[0] generates a owner address
        #    nodes[1] generates a cold-staking and a leasing addresses
        # ---------------------------------------------
        print("*** 3 ***")
        owner_address = self.nodes[0].getnewaddress()
        self.log.info("Owner Address: %s" % owner_address)
        leaser_address = self.nodes[1].getnewleasingaddress()
        staker_address = self.nodes[1].getnewstakingaddress()
        staker_privkey = self.nodes[1].dumpprivkey(staker_address)
        self.log.info("Staking Address: %s" % staker_address)
        self.log.info("Leasing Address: %s" % leaser_address)

        # 4) Check enforcement of Leasing.
        # ---------------------
        print("*** 4 ***")
        # Check that SPORK 1017 is disabled
        assert (not self.isLeasingEnforced())
        self.log.info("Creating a leasing tx before leasing enforcement...")
        assert_raises_rpc_error(-4, "The transaction was rejected!",
                                self.nodes[0].leasetoaddress, leaser_address, INPUT_VALUE, owner_address, False, True)
        self.log.info("Good. Leasing NOT ACTIVE yet.")

        # Enable SPORK 1017
        self.setLeasingEnforcement()
        # double check
        assert (self.isLeasingEnforced())

        # 5) nodes[0] delegates a number of inputs for nodes[1] to stake em.
        # ------------------------------------------------------------------
        print("*** 5 ***")

        assert (not self.isColdStakingEnforced())
        # Enable SPORK 17
        self.setColdStakingEnforcement()
        # double check
        assert (self.isColdStakingEnforced())

        self.log.info("Creating %d real stake-delegation txes..." % NUM_OF_INPUTS)
        for i in range(NUM_OF_INPUTS):
            res = self.nodes[0].delegatestake(staker_address, INPUT_VALUE, owner_address)
            assert(res != None and res["txid"] != None and res["txid"] != "")
            assert_equal(res["owner_address"], owner_address)
            assert_equal(res["staker_address"], staker_address)
        sync_mempools(self.nodes)
        self.mocktime = self.generate_pos(2, self.mocktime)
        sync_blocks(self.nodes)
        self.log.info("%d Txes created." % NUM_OF_INPUTS)
        # check balances:
        self.expected_delegated_balance = NUM_OF_INPUTS * INPUT_VALUE
        self.expected_immature_delegated_balance = 0
        self.checkBalances()

        self.log.info("Whitelisting the owner as delegator...")
        ret = self.nodes[1].delegatoradd(owner_address)
        assert(ret)
        self.log.info("Delegator address %s whitelisted" % owner_address)

        # 6) nodes[0] leases a number of inputs for nodes[1] for leasing em.
        # ------------------------------------------------------------------
        print("*** 6 ***")
        self.log.info("First check warning when using external addresses...")
        assert_raises_rpc_error(-5, "Only the owner of the key to owneraddress will be allowed to spend these coins",
                                self.nodes[0].leasetoaddress, leaser_address, INPUT_VALUE, "mviUPBMwVphTavWSvyyuT6ZtAgbqLjzKNU")
        self.log.info("Good. Warning triggered.")

        self.log.info("Now force the use of external address creating (but not sending) the lease...")
        res = self.nodes[0].rawleasetoaddress(leaser_address, INPUT_VALUE, "mviUPBMwVphTavWSvyyuT6ZtAgbqLjzKNU", True)
        assert(res is not None and res != "")
        self.log.info("Good. Warning NOT triggered.")

        self.log.info("Now lease with internal owner address..")
        self.log.info("Try first with a value (0.99) below the threshold")
        assert_raises_rpc_error(-8, "Invalid amount",
                                self.nodes[0].leasetoaddress, leaser_address, 0.99, owner_address)
        self.log.info("Nice. It was not possible.")
        self.log.info("Then try (creating but not sending) with the threshold value (1.00)")
        res = self.nodes[0].rawleasetoaddress(leaser_address, 1.00, owner_address)
        assert(res is not None and res != "")
        self.log.info("Good. Warning NOT triggered.")

        self.log.info("Now creating %d real leasing txes..." % NUM_OF_INPUTS)
        for i in range(NUM_OF_INPUTS):
            res = self.nodes[0].leasetoaddress(leaser_address, INPUT_VALUE, owner_address)
            assert(res != None and res["txid"] != None and res["txid"] != "")
            assert_equal(res["owner_address"], owner_address)
            assert_equal(res["leaser_address"], leaser_address)
        sync_mempools(self.nodes)
        self.mocktime = self.generate_pos(2, self.mocktime)
        self.noreward_period -= 1
        sync_blocks(self.nodes)
        self.log.info("%d Txes created." % NUM_OF_INPUTS)
        # check balances:
        self.expected_leased_balance = NUM_OF_INPUTS * INPUT_VALUE
        self.expected_immature_leased_balance = 0
        self.checkBalances()

        # 7) check that the owner (nodes[0]) can spend the leased coins.
        # -------------------------------------------------------
        print("*** 7 ***")
        self.log.info("Spending back one of the leased UTXOs...")
        leased_utxos = getLeasedUtxos(self.nodes[0].listunspent())
        assert_equal(NUM_OF_INPUTS, len(leased_utxos))
        assert_equal(len(leased_utxos), len(self.nodes[0].listleasingutxos()))
        u = leased_utxos[0]
        txhash = self.spendUTXOwithNode(u, 0)
        assert(txhash != None)
        self.log.info("Good. Owner was able to spend - tx: %s" % str(txhash))

        self.mocktime = self.generate_pos(2, self.mocktime)
        self.noreward_period -= 1
        sync_blocks(self.nodes)
        # check balances after spend.
        self.expected_leased_balance -= float(u["amount"])
        self.checkBalances()
        self.log.info("Balances check out after spend")
        assert_equal(NUM_OF_INPUTS-1, len(self.nodes[0].listleasingutxos()))

        # 8) check that the leaser CANNOT spend the coins.
        # ------------------------------------------------
        print("*** 8 ***")
        self.log.info("Trying to spend one of the leasing UTXOs with the leaser key...")
        leased_utxos = getLeasedUtxos(self.nodes[0].listunspent())
        assert_greater_than(len(leased_utxos), 0)
        u = leased_utxos[0]
        assert_raises_rpc_error(-26, "mandatory-script-verify-flag-failed (Script failed an OP_CHECKLEASEVERIFY operation",
                                self.spendUTXOwithNode, u, 1)
        self.log.info("Good. Leaser was NOT able to spend (failed OP_CHECKLEASEVERIFY)")
        self.mocktime = self.generate_pos(2, self.mocktime)
        self.noreward_period -= 1
        sync_blocks(self.nodes)

        # 9) generate blocks and check no leasing rewards .
        # --------------------------------------------------------------------------------
        print("*** 9 ***")
        assert_equal(self.nodes[1].getstakingstatus()["stakeablecoins"], True)

        self.log.info("Generating %d valid cold-stake blocks without leasing rewards..." % self.noreward_period)
        for i in range(self.noreward_period):
            self.mocktime = self.generate_pos(1, self.mocktime)

            # update balances after staked block.
            self.expected_delegated_balance -= INPUT_VALUE
            self.expected_immature_delegated_balance += (INPUT_VALUE + 250)

        self.log.info("   Syncing %d blocks without leasing rewards" % self.noreward_period)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[0].getblockcount(), self.nodes[1].getblockcount())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[0].getbestblockhash())
        assert_equal(0, len(getLeasingRewards(self.nodes[0])))
        assert_equal(0, len(getLeasingRewards(self.nodes[1])))
        self.log.info("done")

        self.checkBalances()
        self.log.info("Balances check out after staked blocks")

        # 10) generate blocks with leasing reward
        # ----------------------------------------------------------------------------------
        print("*** 10 ***")

        self.reward_period = int((NUM_OF_INPUTS - 1 + LEASING_REWARDS_PER_BLOCK) / LEASING_REWARDS_PER_BLOCK)
        self.log.info("Generating %d blocks with leasing rewards..." % self.reward_period)
        for i in range(self.reward_period):
            self.mocktime = self.generate_pos(1, self.mocktime)

            # update balances after staked block.
            self.expected_delegated_balance -= INPUT_VALUE
            self.expected_immature_delegated_balance += (INPUT_VALUE + 250)

        self.log.info("   Syncing %d blocks with leasing rewards" % self.reward_period)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[0].getblockcount(), self.nodes[1].getblockcount())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[0].getbestblockhash())
        self.log.info("done")

        # 11) validate limits on leasing rewards
        # ----------------------------------------------------------------------------------
        print("*** 11 ***")
        self.log.info("Validate limits on leasing reward...")
        leasing_rewards = getLeasingRewards(self.nodes[0])
        value = sum([float(x["amount"]) for x in leasing_rewards])

        reward_per_block = float(self.nodes[0].getwalletinfo()["leased_balance"])
        reward_per_block *= 0.15 # 15% per year
        reward_per_block /= (365 * 24 * 60) # per 1 block
        reward_per_block /= NUM_OF_INPUTS-1 # per 1 trx

        expected_value = 0.0
        period = LEASING_REWARD_PERIOD
        inputs = NUM_OF_INPUTS-1
        for i in range(self.reward_period): # for each block its own period
            expected_value += (reward_per_block * period * min(LEASING_REWARDS_PER_BLOCK, inputs))
            period += 1
            inputs -= LEASING_REWARDS_PER_BLOCK

        self.log.info("  Leasing rewards for leasee: %f" % value)
        assert_equal(round(value, 6), round(expected_value, 6))
        assert_equal(NUM_OF_INPUTS-1, len(leasing_rewards))

        leasing_rewards = getLeasingRewards(self.nodes[1])
        value = sum([float(x["amount"]) for x in leasing_rewards])
        expected_value = float(self.nodes[1].getwalletinfo()["leasing_balance"])
        expected_value *= 0.002 # 0.2% per block
        expected_value *= self.reward_period

        self.log.info("  Leasing rewards for leaser: %f" % value)
        assert_equal(round(value, 6), round(expected_value,6))
        assert_equal(self.reward_period, len(leasing_rewards))
        self.log.info("done")

        # 12) check that the owner (nodes[0]) can spend the leasing reward coins.
        # ----------------------------------------------------------------------------------
        print("*** 12 ***")
        self.log.info("Spending one of the leasing reward UTXOs...")
        for i in [0, 1]:
            reward = getLeasingRewards(self.nodes[i])[0]
            txhash = self.spendUTXOwithNode(reward, i)
            assert(txhash != None)
            self.log.info("  Good. %s was able to spend - tx: %s" % ("Owner" if i == 0 else "Leaser", str(txhash)))

        self.mocktime = self.generate_pos(2, self.mocktime)
        sync_blocks(self.nodes)
        self.noreward_period -= 1
        # check balances after spend.
        self.checkBalances()
        self.log.info("Balances check out after spend")

        # 13) check.
        # ----------------------------------------------------------------------------------
        print("*** 13 ***")

        self.log.info("Generating 100 blocks without leasing rewards...")
        for i in range(2):
            for peer in [0, 2]:
                for j in range(25):
                    self.mocktime = self.generate_pos(peer, self.mocktime)
                sync_blocks(self.nodes)

        self.log.info("Generating blocks %d with leasing rewards..." % self.reward_period)
        for i in range(self.reward_period):
            self.mocktime = self.generate_pos(1, self.mocktime)

        self.log.info("   Syncing %d blocks with leasing rewards" % self.reward_period)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[0].getbestblockhash())
        assert_equal(self.nodes[0].getblockcount(), self.nodes[1].getblockcount())
        assert_equal(NUM_OF_INPUTS*2-3, len(getLeasingRewards(self.nodes[0])))
        assert_equal(self.reward_period*2-1, len(getLeasingRewards(self.nodes[1])))

        self.noreward_period = LEASING_REWARD_PERIOD - self.reward_period
        self.log.info("Generating %d blocks without leasing rewards..." % self.noreward_period)
        for i in range(self.noreward_period):
            self.mocktime = self.generate_pos(1, self.mocktime)

        self.log.info("   Syncing %d blocks without leasing rewards" % self.noreward_period)
        sync_blocks(self.nodes)
        assert_equal(self.nodes[0].getblockcount(), self.nodes[1].getblockcount())
        assert_equal(self.nodes[1].getbestblockhash(), self.nodes[0].getbestblockhash())
        assert_equal(NUM_OF_INPUTS*2-3, len(getLeasingRewards(self.nodes[0])))
        assert_equal(self.reward_period*2-1, len(getLeasingRewards(self.nodes[1])))
        self.log.info("done")




    def checkBalances(self):
        w_info = self.nodes[0].getwalletinfo()
        self.log.info("OWNER - Delegated %f / Cold %f   [%f / %f]" % (
            float(w_info["delegated_balance"]), w_info["cold_staking_balance"],
            float(w_info["immature_delegated_balance"]), w_info["immature_cold_staking_balance"]))
        assert_equal(float(w_info["delegated_balance"]), self.expected_delegated_balance)
        assert_equal(float(w_info["immature_delegated_balance"]), self.expected_immature_delegated_balance)
        assert_equal(float(w_info["cold_staking_balance"]), 0)

        self.log.info("LEASEE - Leased %f / Leasing %f   [%f / %f]" % (
            float(w_info["leased_balance"]), w_info["leasing_balance"],
            float(w_info["immature_leased_balance"]), w_info["immature_leasing_balance"]))
        assert_equal(float(w_info["leased_balance"]), self.expected_leased_balance)
        assert_equal(float(w_info["immature_leased_balance"]), self.expected_immature_leased_balance)
        assert_equal(float(w_info["leasing_balance"]), 0)

        w_info = self.nodes[1].getwalletinfo()
        self.log.info("STAKER - Delegated %f / Cold %f   [%f / %f]" % (
            float(w_info["delegated_balance"]), w_info["cold_staking_balance"],
            float(w_info["immature_delegated_balance"]), w_info["immature_cold_staking_balance"]))
        assert_equal(float(w_info["delegated_balance"]), 0)
        assert_equal(float(w_info["cold_staking_balance"]), self.expected_delegated_balance)
        assert_equal(float(w_info["immature_cold_staking_balance"]), self.expected_immature_delegated_balance)

        self.log.info("LEASER - Leased %f / Leasing %f   [%f / %f]" % (
            float(w_info["leased_balance"]), w_info["leasing_balance"],
            float(w_info["immature_leased_balance"]), w_info["immature_leasing_balance"]))
        assert_equal(float(w_info["leased_balance"]), 0)
        assert_equal(float(w_info["leasing_balance"]), self.expected_leased_balance)
        assert_equal(float(w_info["immature_leasing_balance"]), self.expected_immature_leased_balance)

    def spendUTXOwithNode(self, utxo, node_n):
        new_addy = self.nodes[node_n].getnewaddress()
        inputs = [{"txid": utxo["txid"], "vout": utxo["vout"]}]
        out_amount = (float(utxo["amount"]) - self.DEFAULT_FEE)
        outputs = {}
        outputs[new_addy] = out_amount
        spendingTx = self.nodes[node_n].createrawtransaction(inputs, outputs)
        spendingTx_signed = self.nodes[node_n].signrawtransaction(spendingTx, [], [], "ALL", True)
        return self.nodes[node_n].sendrawtransaction(spendingTx_signed["hex"])

    def spendUTXOsWithNode(self, utxos, node_n):
        new_addy = self.nodes[node_n].getnewaddress()
        inputs = []
        outputs = {}
        outputs[new_addy] = 0
        for utxo in utxos:
            inputs.append({"txid": utxo["txid"], "vout": utxo["vout"]})
            outputs[new_addy] += float(utxo["amount"])
        outputs[new_addy] -= self.DEFAULT_FEE
        spendingTx = self.nodes[node_n].createrawtransaction(inputs, outputs)
        spendingTx_signed = self.nodes[node_n].signrawtransaction(spendingTx)
        return self.nodes[node_n].sendrawtransaction(spendingTx_signed["hex"])





if __name__ == '__main__':
    BTCU_LeasingTest().main()
