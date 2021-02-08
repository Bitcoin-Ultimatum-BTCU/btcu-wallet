#!/usr/bin/env python3
# Copyright (c) 2019 The PIVX developers
# Copyright (c) 2020 The BTCU developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# -*- coding: utf-8 -*-

from io import BytesIO
from time import sleep, time
import os

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
        self.num_nodes = 4
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

    def generatePosSync(self, miner_id):
        sync_mempools(self.nodes)
        self.mocktime = self.generate_pos(miner_id, self.mocktime)
        sync_blocks(self.nodes)


    def reprocessBlocks(self, num_blocks):
        self.log.info("   Reprocess %d blocks" % (num_blocks))
        for peer in range(self.num_nodes):
            self.nodes[peer].reprocess(num_blocks)


    def run_test(self):
        self.description = "Performs tests on the Leasing P2L implementation"
        self.init_test()
        NUM_OF_INPUTS = 20
        INPUT_VALUE = 124
        LEASING_REWARD_PERIOD = 10
        LEASING_REWARDS_PER_BLOCK = 10
        LEASING_MATURITY = 3
        MN_CONFIRMATIONS = 30
        MN_PAYMENT = 50
        self.noreward_period = LEASING_MATURITY + LEASING_REWARD_PERIOD
        self.expected_delegated_balance = 0
        self.expected_immature_delegated_balance = 0
        self.expected_validator_leasing_balance = 0
        self.expected_master_leasing_balance = 0
        self.expected_immature_leased_balance = 0

        self.miner1_id = 0
        self.miner2_id = 1
        self.leaser_id = 2
        self.staker_id = 2
        self.master_id = 3

        self.miner1 = self.nodes[self.miner1_id]
        self.miner2 = self.nodes[self.miner2_id]
        self.leaser = self.nodes[self.leaser_id]
        self.staker = self.nodes[self.staker_id]
        self.master = self.nodes[self.master_id]

        self.master_alias = 'mnode'
        self.master_priv_key = '9247iC59poZmqBYt9iDh9wDam6v9S1rW5XekjLGyPnDhrDkP4AK'
        self.master_port = 23666 # regtest port

        # miner1 - coin-owner
        # leaser - cold-staker and leaser

        # 1) miners mine 150 blocks each
        # --------------------------------------------
        print("*** 1 ***")
        self.log.info("Mining 150 Blocks...")
        for i in range(3):
            for peer in [self.miner2_id, self.miner1_id]:
                for j in range(25):
                    self.mocktime = self.generate_pow(peer, self.mocktime)
                sync_blocks(self.nodes)

        # 2) staker sends his entire balance (50 mature rewards) to miner2
        #  - miner2 stakes a block - staker locks the change
        # --------------------------------------------
        print("*** 2 ***")

        self.log.info("Emptying staker node balance")
        assert_equal(self.staker.getbalance(), 50 * 250)
        for i in range(49):
            assert(self.staker.sendtoaddress(self.miner2.getnewaddress(), 250) is not None)
        assert(self.staker.sendtoaddress(self.miner2.getnewaddress(), 250 - 0.01) is not None)
        self.generatePosSync(self.miner2_id)

        # lock the change output (so it's not used as stake input in generate_pos)
        for x in self.staker.listunspent():
            assert (self.staker.lockunspent(False, [{"txid": x['txid'], "vout": x['vout']}]))
        # check that it cannot stake
        sleep(1)
        assert_equal(self.staker.getstakingstatus()["stakeablecoins"], False)

        # 3) setup master node vin
        # ---------------------------------------------
        print("*** 3 ***")

        self.log.info("Setup masternode VIN..")

        self.log.info("  emptying masternode balance")
        assert_equal(self.master.getbalance(), 50 * 250)
        for i in range(49):
            assert(self.master.sendtoaddress(self.miner1.getnewaddress(), 250) is not None)
        assert(self.master.sendtoaddress(self.miner1.getnewaddress(), 250 - 0.01) is not None)
        self.generatePosSync(self.miner1_id)

        self.log.info("  adding balance for masternode..")
        master_address = self.master.getnewaddress(self.master_alias)

        # send to the owner the collateral tx cost
        master_collateral_tx_id = self.miner1.sendtoaddress(master_address, 1000)

        # confirm and verify reception
        self.generatePosSync(self.miner2_id)

        assert_equal(round(self.master.getbalance(), 1), 1000.0)
        assert_greater_than(self.master.getrawtransaction(master_collateral_tx_id, 1)["confirmations"], 0)

        self.log.info("  generate confirmations for masternode VIN..")
        for peer in [self.miner1_id, self.miner2_id]:
            for i in range(int(MN_CONFIRMATIONS / 2)):
                self.mocktime = self.generate_pos(peer, self.mocktime)
            sync_blocks(self.nodes)

        master_output = self.master.getmasternodeoutputs()
        assert_equal(len(master_output), 1)
        assert_equal(master_output[0]["txhash"], master_collateral_tx_id)

        self.log.info("done")

        # 4) miner1 generates a owner address
        #    staker generates a cold-staking and a leasing addresses
        #    master generates leasing address
        # ---------------------------------------------
        print("*** 4 ***")

        owner1_address = self.miner1.getnewaddress()
        self.log.info("Owner (miner1 -> validator) Address: %s" % owner1_address)
        owner2_address = self.miner2.getnewaddress()
        self.log.info("Owner (miner2 -> master) Address: %s" % owner2_address)
        staker_address = self.staker.getnewstakingaddress()
        self.log.info("Staking Address: %s" % staker_address)
        validator_leaser_address = self.leaser.getnewleasingaddress()
        self.log.info("Validator Leasing Address: %s" % validator_leaser_address)
        master_leaser_address = self.master.getnewleasingaddress()
        self.log.info("Masternode Leasing Address: %s" % master_leaser_address)

        # 5) Check enforcement of Leasing.
        # ---------------------
        print("*** 5 ***")

        # Check that SPORK 1017 is disabled
        assert (not self.isLeasingEnforced())
        self.log.info("Creating a leasing tx before leasing enforcement...")

        def assertDisabledSpork(miner, leaser_address, owner_address):
            assert_raises_rpc_error(-4, "The transaction was rejected!",
                                    miner.leasetoaddress, leaser_address, INPUT_VALUE, owner_address, False, True)

        assertDisabledSpork(self.miner1, validator_leaser_address, owner1_address)
        assertDisabledSpork(self.miner2, master_leaser_address, owner2_address)

        self.log.info("Good. Leasing NOT ACTIVE yet.")

        # Enable SPORK 1017
        self.setLeasingEnforcement()
        # double check
        assert (self.isLeasingEnforced())

        # 6) miner1 delegates a number of inputs for staker to stake em.
        # ------------------------------------------------------------------
        print("*** 6 ***")

        assert (not self.isColdStakingEnforced())
        # Enable SPORK 17
        self.setColdStakingEnforcement()
        # double check
        assert (self.isColdStakingEnforced())

        self.log.info("Creating %d real stake-delegation txes..." % NUM_OF_INPUTS)
        for i in range(NUM_OF_INPUTS):
            res = self.miner1.delegatestake(staker_address, INPUT_VALUE, owner1_address)
            assert(res != None and res["txid"] != None and res["txid"] != "")
            assert_equal(res["owner_address"], owner1_address)
            assert_equal(res["staker_address"], staker_address)
            
        self.generatePosSync(self.miner1_id)
        self.log.info("%d Txes created." % NUM_OF_INPUTS)
        # check balances:
        self.expected_delegated_balance = NUM_OF_INPUTS * INPUT_VALUE
        self.expected_immature_delegated_balance = 0
        self.checkBalances()

        self.log.info("Whitelisting the owner as delegator...")
        ret = self.staker.delegatoradd(owner1_address)
        assert(ret)
        self.log.info("Delegator address %s whitelisted" % owner1_address)

        # 7) miner1 leases a number of inputs for leaser for leasing em.
        #    miner2 leases a number of inputs for master for leasing em.
        # ------------------------------------------------------------------
        print("*** 7 ***")

        self.log.info("First check warning when using external addresses...")
        def assertLeaseOwnerAddress(miner, leaser_address):
            assert_raises_rpc_error(-5, "Only the owner of the key to owneraddress will be allowed to spend these coins",
                                    miner.leasetoaddress, leaser_address, INPUT_VALUE, "mviUPBMwVphTavWSvyyuT6ZtAgbqLjzKNU")

        assertLeaseOwnerAddress(self.miner1, validator_leaser_address)
        assertLeaseOwnerAddress(self.miner2, master_leaser_address)
        self.log.info("Good. Warning triggered.")

        self.log.info("Now force the use of external address creating (but not sending) the lease...")
        def forceLeaseToAddress(miner, leaser_address):
            res = miner.rawleasetoaddress(leaser_address, INPUT_VALUE, "mviUPBMwVphTavWSvyyuT6ZtAgbqLjzKNU", True)
            assert(res is not None and res != "")

        forceLeaseToAddress(self.miner1, validator_leaser_address)
        forceLeaseToAddress(self.miner2, master_leaser_address)
        self.log.info("Good. Warning NOT triggered.")

        self.log.info("Now lease with internal owner address..")
        self.log.info("Try first with a value (0.99) below the threshold")
        def assertLeaseInvalidAmount(miner, leaser_address, owner_address):
            assert_raises_rpc_error(-8, "Invalid amount",
                                    miner.leasetoaddress, leaser_address, 0.99, owner_address)
        assertLeaseInvalidAmount(self.miner1, validator_leaser_address, owner1_address)
        assertLeaseInvalidAmount(self.miner2, master_leaser_address, owner2_address)
        self.log.info("Nice. It was not possible.")

        self.log.info("Then try (creating but not sending) with the threshold value (1.00)")
        def successLease(miner, leaser_address, owner_address):
            res = miner.rawleasetoaddress(leaser_address, 1.00, owner_address)
            assert(res is not None and res != "")
            assert_equal(res["owner_address"], owner_address)
            assert_equal(res["leaser_address"], leaser_address)

        successLease(self.miner1, validator_leaser_address, owner1_address)
        successLease(self.miner2, master_leaser_address, owner2_address)
        self.log.info("Good. Warning NOT triggered.")

        def leaseToAddress(miner, leaser_address, owner_address):
            res = miner.leasetoaddress(leaser_address, INPUT_VALUE, owner_address)
            assert(res != None and res["txid"] != None and res["txid"] != "")
            assert_equal(res["owner_address"], owner_address)
            assert_equal(res["leaser_address"], leaser_address)

        self.log.info("Now creating %d real leasing txes..." % NUM_OF_INPUTS)
        for i in range(NUM_OF_INPUTS):
            leaseToAddress(self.miner1, validator_leaser_address, owner1_address)
            leaseToAddress(self.miner2, master_leaser_address, owner2_address)

        self.generatePosSync(self.miner2_id)
        self.noreward_period -= 1
        self.log.info("%d Txes created." % NUM_OF_INPUTS)
        # check balances:
        self.expected_validator_leasing_balance = NUM_OF_INPUTS * INPUT_VALUE
        self.expected_master_leasing_balance = NUM_OF_INPUTS * INPUT_VALUE
        self.expected_immature_leased_balance = 0
        self.checkBalances()

        # 8) Start master node (see step 3)
        # ---------------------------------------------
        print("*** 8 ***")

        def setupMN():
            self.log.info("  creating masternode..")

            master_output = self.master.getmasternodeoutputs()
            assert_equal(len(master_output), 1)
            master_index = master_output[0]["outputidx"]

            self.log.info("  updating masternode.conf...")

            conf_data = self.master_alias + " 127.0.0.1:" + str(self.master_port)
            conf_data += " " + str(self.master_priv_key)
            conf_data += " " + str(master_output[0]["txhash"])
            conf_data += " " + str(master_index)
            conf_path = os.path.join(self.options.tmpdir, "node{}".format(self.master_id), "regtest", "masternode.conf")
            with open(conf_path, "a+") as file_object:
                file_object.write("\n")
                file_object.write(conf_data)

        def initMN():
            self.log.info("  initialize masternode..")
            self.master.initmasternode(self.master_priv_key, "127.0.0.1:"+str(self.master_port))
            # self.generatePosSync(self.miner2_id)

            SYNC_FINISHED = [999] * self.num_nodes
            synced = [-1] * self.num_nodes
            timeout = time() + 45
            while synced != SYNC_FINISHED and time() < timeout:
                for i in range(self.num_nodes):
                    if synced[i] != SYNC_FINISHED[i]:
                        synced[i] = self.nodes[i].mnsync("status")["RequestedMasternodeAssets"]
                if synced != SYNC_FINISHED:
                    sleep(5)
            assert_equal(synced, SYNC_FINISHED)

        def startMN():
            self.log.info("  starting masternode..")
            ret = self.master.startmasternode("alias", "false", self.master_alias, True)
            assert_equal(ret["result"], "success")

        self.log.info("Creating masternode...")

        setupMN()
        initMN()
        startMN()

        self.log.info("  masternode enabled and running properly!")
        self.log.info("done")

        # 9) check that the owner miner1 can spend the leased coins.
        # -------------------------------------------------------
        print("*** 9 ***")
        self.log.info("Spending back one of the leased UTXOs...")

        def successLeaseSpend(miner, miner_id):
            leased_utxos = getLeasedUtxos(miner.listunspent())
            assert_equal(NUM_OF_INPUTS, len(leased_utxos))
            assert_equal(len(leased_utxos), len(miner.listleasingutxos()))
            u = leased_utxos[0]
            txhash = self.spendUTXOwithNode(u, miner_id)
            assert(txhash != None)
            return float(u['amount'])

        self.expected_validator_leasing_balance -= successLeaseSpend(self.miner1, self.miner1_id)
        self.expected_master_leasing_balance -= successLeaseSpend(self.miner2, self.miner2_id)

        self.generatePosSync(self.miner1_id)
        self.noreward_period -= 1
        # check balances after spend.
        self.checkBalances()
        self.log.info("Balances check out after spend")
        assert_equal(NUM_OF_INPUTS-1, len(self.miner1.listleasingutxos()))
        assert_equal(NUM_OF_INPUTS-1, len(self.miner2.listleasingutxos()))

        # 10) check that the leaser CANNOT spend the coins.
        # ------------------------------------------------
        print("*** 10 ***")
        self.log.info("Trying to spend one of the leasing UTXOs with the leaser key...")

        def assertLeaseVerify(miner, leaser_id):
            leased_utxos = getLeasedUtxos(miner.listunspent())
            assert_greater_than(len(leased_utxos), 0)
            u = leased_utxos[0]
            assert_raises_rpc_error(-26, "mandatory-script-verify-flag-failed (Script failed an OP_CHECKLEASEVERIFY operation",
                                    self.spendUTXOwithNode, u, leaser_id)

        assertLeaseVerify(self.miner1, self.leaser_id)
        assertLeaseVerify(self.miner2, self.master_id)

        self.log.info("Good. Leaser was NOT able to spend (failed OP_CHECKLEASEVERIFY)")
        self.generatePosSync(self.miner2_id)
        self.noreward_period -= 1
        self.checkBalances()

        # 11) generate blocks and check no leasing rewards.
        # --------------------------------------------------------------------------------
        print("*** 11 ***")
        assert_equal(self.staker.getstakingstatus()["stakeablecoins"], True)

        self.log.info("Generating %d valid cold-stake blocks without leasing rewards..." % self.noreward_period)
        for i in range(self.noreward_period):
            self.mocktime = self.generate_pos(self.staker_id, self.mocktime)

            # update balances after staked block.
            self.expected_delegated_balance -= INPUT_VALUE
            self.expected_immature_delegated_balance += (INPUT_VALUE + 250 - MN_PAYMENT)

        self.log.info("   Syncing %d blocks without leasing rewards" % self.noreward_period)
        sync_blocks(self.nodes)
        for peer in range(self.num_nodes):
            assert_equal(0, len(getLeasingRewards(self.nodes[peer])))
        self.log.info("done")

        self.checkBalances()
        self.log.info("Balances check out after staked blocks")

        # 12) generate blocks with leasing reward
        # ----------------------------------------------------------------------------------
        print("*** 12 ***")

        self.reward_period = int((NUM_OF_INPUTS - 1 + LEASING_REWARDS_PER_BLOCK) / LEASING_REWARDS_PER_BLOCK)
        self.log.info("Generating %d blocks with leasing rewards..." % self.reward_period)
        for i in range(self.reward_period):
            self.mocktime = self.generate_pos(self.leaser_id, self.mocktime)

            # update balances after staked block.
            self.expected_delegated_balance -= INPUT_VALUE
            self.expected_immature_delegated_balance += (INPUT_VALUE + 250 - MN_PAYMENT)

        self.log.info("   Syncing %d blocks with leasing rewards" % self.reward_period)
        sync_blocks(self.nodes)

        self.reprocessBlocks(self.reward_period + 5)

        self.log.info("done")

        # 13) validate limits on leasing rewards
        # ----------------------------------------------------------------------------------
        print("*** 13 ***")
        self.log.info("Validate limits on leasing reward...")

        def validateLeasingReward(miner, leaser, leaser_pct):
            leasing_rewards = getLeasingRewards(miner)
            value = sum([float(x["amount"]) for x in leasing_rewards])

            reward_per_block = float(miner.getwalletinfo()["leased_balance"])
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

            leasing_rewards = getLeasingRewards(leaser)
            value = sum([float(x["amount"]) for x in leasing_rewards])
            expected_value = float(leaser.getwalletinfo()["leasing_balance"])
            expected_value *= leaser_pct / 100
            expected_value *= self.reward_period

            self.log.info("  Leasing rewards for leaser: %f" % value)
            assert_equal(round(value, 6), round(expected_value,6))
            assert_equal(self.reward_period, len(leasing_rewards))

        validateLeasingReward(self.miner1, self.leaser, 0.2)
        validateLeasingReward(self.miner2, self.master, 0.1)
        self.log.info("done")

        # 14) check that the owner (nodes[0]) can spend the leasing reward coins.
        # ----------------------------------------------------------------------------------
        print("*** 14 ***")
        self.log.info("Spending one of the leasing reward UTXOs...")
        for peer in range(self.num_nodes):
            reward = getLeasingRewards(self.nodes[peer])[0]
            txhash = self.spendUTXOwithNode(reward, peer)
            assert(txhash != None)
            self.log.info("  Good. %s was able to spend - tx: %s" % ("Owner" if peer in [self.miner1_id, self.miner2] else "Leaser", str(txhash)))

        self.generatePosSync(self.miner2_id)
        self.reprocessBlocks(5)

        self.noreward_period -= 1
        # check balances after spend.
        self.checkBalances()
        self.log.info("Balances check out after spend")

        # 15) check.
        # ----------------------------------------------------------------------------------
        print("*** 15 ***")

        self.log.info("Generating 100 blocks without leasing rewards...")
        for i in range(2):
            for peer in [self.miner1_id, self.miner2_id]:
                for j in range(25):
                    self.mocktime = self.generate_pos(peer, self.mocktime)
                sync_blocks(self.nodes)

        self.log.info("Generating blocks %d with leasing rewards..." % self.reward_period)
        for i in range(self.reward_period):
            self.mocktime = self.generate_pos(self.leaser_id, self.mocktime)
        self.log.info("   Syncing %d blocks with leasing rewards" % self.reward_period)
        sync_blocks(self.nodes)

        assert_equal(NUM_OF_INPUTS*2-3, len(getLeasingRewards(self.miner1)))
        assert_equal(self.reward_period*2-1, len(getLeasingRewards(self.leaser)))
        assert_equal(NUM_OF_INPUTS*2-3, len(getLeasingRewards(self.miner2)))
        assert_equal(self.reward_period*2-1, len(getLeasingRewards(self.master)))

        self.reprocessBlocks(self.reward_period + 5)

        self.noreward_period = LEASING_REWARD_PERIOD - self.reward_period
        self.log.info("Generating %d blocks without leasing rewards..." % self.noreward_period)
        for i in range(self.noreward_period):
            self.mocktime = self.generate_pos(self.leaser_id, self.mocktime)

        self.log.info("   Syncing %d blocks without leasing rewards" % self.noreward_period)
        sync_blocks(self.nodes)
        assert_equal(NUM_OF_INPUTS*2-3, len(getLeasingRewards(self.miner1)))
        assert_equal(self.reward_period*2-1, len(getLeasingRewards(self.leaser)))
        assert_equal(NUM_OF_INPUTS*2-3, len(getLeasingRewards(self.miner2)))
        assert_equal(self.reward_period*2-1, len(getLeasingRewards(self.master)))
        self.log.info("done")

        self.reprocessBlocks(self.miner1.getblockcount() - 1)


    def checkBalances(self):
        w_info = self.miner1.getwalletinfo()
        self.log.info("OWNER - Delegated %f / Cold %f   [%f / %f]" % (
            float(w_info["delegated_balance"]), w_info["cold_staking_balance"],
            float(w_info["immature_delegated_balance"]), w_info["immature_cold_staking_balance"]))
        assert_equal(float(w_info["delegated_balance"]), self.expected_delegated_balance)
        assert_equal(float(w_info["immature_delegated_balance"]), self.expected_immature_delegated_balance)
        assert_equal(float(w_info["cold_staking_balance"]), 0)

        self.log.info("LEASEE (miner1) - Leased %f / Leasing %f   [%f / %f]" % (
            float(w_info["leased_balance"]), w_info["leasing_balance"],
            float(w_info["immature_leased_balance"]), w_info["immature_leasing_balance"]))
        assert_equal(float(w_info["leased_balance"]), self.expected_validator_leasing_balance)
        assert_equal(float(w_info["immature_leased_balance"]), self.expected_immature_leased_balance)
        assert_equal(float(w_info["leasing_balance"]), 0)

        self.log.info("LEASEE (miner2) - Leased %f / Leasing %f   [%f / %f]" % (
            float(w_info["leased_balance"]), w_info["leasing_balance"],
            float(w_info["immature_leased_balance"]), w_info["immature_leasing_balance"]))
        assert_equal(float(w_info["leased_balance"]), self.expected_master_leasing_balance)
        assert_equal(float(w_info["immature_leased_balance"]), self.expected_immature_leased_balance)
        assert_equal(float(w_info["leasing_balance"]), 0)

        w_info = self.staker.getwalletinfo()
        self.log.info("STAKER - Delegated %f / Cold %f   [%f / %f]" % (
            float(w_info["delegated_balance"]), w_info["cold_staking_balance"],
            float(w_info["immature_delegated_balance"]), w_info["immature_cold_staking_balance"]))
        assert_equal(float(w_info["delegated_balance"]), 0)
        assert_equal(float(w_info["cold_staking_balance"]), self.expected_delegated_balance)
        assert_equal(float(w_info["immature_cold_staking_balance"]), self.expected_immature_delegated_balance)

        w_info = self.leaser.getwalletinfo()
        self.log.info("LEASER (validator) - Leased %f / Leasing %f   [%f / %f]" % (
            float(w_info["leased_balance"]), w_info["leasing_balance"],
            float(w_info["immature_leased_balance"]), w_info["immature_leasing_balance"]))
        assert_equal(float(w_info["leased_balance"]), 0)
        assert_equal(float(w_info["leasing_balance"]), self.expected_validator_leasing_balance)
        assert_equal(float(w_info["immature_leasing_balance"]), self.expected_immature_leased_balance)

        w_info = self.master.getwalletinfo()
        self.log.info("LEASER (master) - Leased %f / Leasing %f   [%f / %f]" % (
            float(w_info["leased_balance"]), w_info["leasing_balance"],
            float(w_info["immature_leased_balance"]), w_info["immature_leasing_balance"]))
        assert_equal(float(w_info["leased_balance"]), 0)
        assert_equal(float(w_info["leasing_balance"]), self.expected_master_leasing_balance)
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
