#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BtcuTestFramework
from test_framework.util import *
from test_framework.script import *
from test_framework.mininode import *
from test_framework.address import *
from test_framework.qtum import *
import sys
import time

class CreatecontractTest(BtcuTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [['-txindex=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def createcontract_simple_test(self):
        """
        pragma solidity ^0.4.0;

        contract Test {
            address owner;
            function Test(){
                owner=msg.sender;
            }
            function setOwner(address test) payable{
                owner=msg.sender;
            }
            function getOwner() constant returns (address cOwner){
                return owner;
            }
            function getSender() constant returns (address cSender){
                return msg.sender;
            }
        }
        """
        ret = self.node.createcontract("6060604052341561000c57fe5b5b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b6101c88061005f6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806313af4035146100515780635e01eb5a1461007f578063893d20e8146100d1575bfe5b61007d600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050610123565b005b341561008757fe5b61008f610168565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156100d957fe5b6100e1610171565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b50565b60003390505b90565b6000600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690505b905600a165627a7a7230582050b454be91cd91e08099814fc2192c5fded81b632733f6c808e4d75e85a766a50029")
        assert('txid' in ret)
        assert('sender' in ret)
        assert('hash160' in ret)
        contract_address = ret['address']
        try:
            self.node.generate(1)
        except:
            print("Generation")
        self.node.generate(1)
        ret = self.node.getaccountinfo(contract_address)
        expected_account_info = {
          "address": contract_address,
          "balance": 0,
          "code": "60606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806313af4035146100515780635e01eb5a1461007f578063893d20e8146100d1575bfe5b61007d600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050610123565b005b341561008757fe5b61008f610168565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156100d957fe5b6100e1610171565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b50565b60003390505b90565b6000600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690505b905600a165627a7a7230582050b454be91cd91e08099814fc2192c5fded81b632733f6c808e4d75e85a766a50029"
        }
        assert_equal(ret['address'], expected_account_info['address'])
        assert_equal(ret['code'], expected_account_info['code'])

    def createcontract_with_sender_test(self):
        self.node.importprivkey("cTQ9SoEvbb41ctdf7Q7UFPogivXGcN88WBkowWQ5L1NWDiF9fahy")
        self.node.sendtoaddress("mgSvvYtnX9JhcXmUqfK2SrGFBfjqC7sQ8e", 0.1)
        self.node.generate(1)
        self.node.generate(1)
        """
        pragma solidity ^0.4.0;

        contract Test {
            address owner;
            function Test(){
                owner=msg.sender;
            }
            function setOwner(address test) payable{
                owner=msg.sender;
            }
            function getOwner() constant returns (address cOwner){
                return owner;
            }
            function getSender() constant returns (address cSender){
                return msg.sender;
            }
        }
        """
        ret = self.node.createcontract("6060604052341561000c57fe5b5b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b6101c88061005f6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806313af4035146100515780635e01eb5a1461007f578063893d20e8146100d1575bfe5b61007d600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050610123565b005b341561008757fe5b61008f610168565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156100d957fe5b6100e1610171565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b50565b60003390505b90565b6000600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690505b905600a165627a7a7230582050b454be91cd91e08099814fc2192c5fded81b632733f6c808e4d75e85a766a50029", 1000000, QTUM_MIN_GAS_PRICE_STR, "qabmqZk3re5b9UpUcznxDkCnCsnKdmPktT")
        assert('txid' in ret)
        assert('sender' in ret)
        assert('hash160' in ret)
        contract_address = ret['address']
        try:
            self.node.generate(1)
        except:
            print("Generation")
        self.node.generate(1)
        ret = self.node.getaccountinfo(contract_address)
        expected_account_info = {
          "address": contract_address,
          "balance": 0,
          "storage": {
            "290decd9548b62a8d60345a988386fc84ba6bc95484008f6362f93160ef3e563": {
              "0000000000000000000000000000000000000000000000000000000000000000": "000000000000000000000000baede766c1a4844efea1326fffc5cd8c1c3e42ae"
            }
          },
          "code": "60606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806313af4035146100515780635e01eb5a1461007f578063893d20e8146100d1575bfe5b61007d600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050610123565b005b341561008757fe5b61008f610168565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156100d957fe5b6100e1610171565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b50565b60003390505b90565b6000600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690505b905600a165627a7a7230582050b454be91cd91e08099814fc2192c5fded81b632733f6c808e4d75e85a766a50029"
        }
        assert(ret == expected_account_info)

    def createcontract_no_broadcast_test(self):
        self.node.sendtoaddress("qabmqZk3re5b9UpUcznxDkCnCsnKdmPktT", 0.1)
        self.node.generate(1)
        self.node.generate(1)
        """
        pragma solidity ^0.4.0;

        contract Test {
            address owner;
            function Test(){
                owner=msg.sender;
            }
            function setOwner(address test) payable{
                owner=msg.sender;
            }
            function getOwner() constant returns (address cOwner){
                return owner;
            }
            function getSender() constant returns (address cSender){
                return msg.sender;
            }
        }
        """
        ret = self.node.createcontract("6060604052341561000c57fe5b5b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b5b6101c88061005f6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806313af4035146100515780635e01eb5a1461007f578063893d20e8146100d1575bfe5b61007d600480803573ffffffffffffffffffffffffffffffffffffffff16906020019091905050610123565b005b341561008757fe5b61008f610168565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b34156100d957fe5b6100e1610171565b604051808273ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200191505060405180910390f35b33600060006101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505b50565b60003390505b90565b6000600060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff1690505b905600a165627a7a7230582050b454be91cd91e08099814fc2192c5fded81b632733f6c808e4d75e85a766a50029", 1000000, QTUM_MIN_GAS_PRICE_STR, "qabmqZk3re5b9UpUcznxDkCnCsnKdmPktT", False)
        assert('raw transaction' in ret)
        assert(len(ret.keys()) == 1)
        decoded_tx = self.node.decoderawtransaction(ret['raw transaction'])

        # verify that at least one output has a scriptPubKey of type create
        for out in decoded_tx['vout']:
            if out['scriptPubKey']['type'] == 'create' or out['scriptPubKey']['type'] == 'create_sender':
                return
        assert(False)

    def run_test(self):
        self.nodes[0].generate(COINBASE_MATURITY+50)
        self.node = self.nodes[0]
        self.createcontract_simple_test()
        #self.createcontract_with_sender_test()
        #self.createcontract_no_broadcast_test()

if __name__ == '__main__':
    CreatecontractTest().main()