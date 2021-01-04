// Copyright (c) 2012-2013 The Bitcoin Core developers
// Copyright (c) 2017-2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key.h"

#include "btcu_address.h"
#include "script/script.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include "test_btcu.h"

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>


static const std::string strSecret1     ("5JJb2QcaFRFoQiibPw78CfFJn72mz97EW3CqmKmdGjRZSBqDFJz");
static const std::string strSecret2     ("5Khom5rH11SV8yL3s48gzQm4a1vfeznPWikmbVPcsMAXCpDQx2C");
static const std::string strSecret1C    ("KyNdBDE1ke4Kqh8apDp2nTp9U5cqZCnWbAjGEdqy6SFQ9kH9iYH3");
static const std::string strSecret2C    ("L5Z7PaQVd2wVCBd5dvpwWPsUkj5sci3LNBJBqEJqKE12JkxXBtn1");

static const std::string strAddressBad("Xta1praZQjyELweyMByXyiREw1ZRsjXzVP");


#ifdef KEY_TESTS_DUMPINFO
void dumpKeyInfo(uint256 privkey)
{
    CKey key;
    key.resize(32);
    memcpy(&secret[0], &privkey, 32);
    std::vector<unsigned char> sec;
    sec.resize(32);
    memcpy(&sec[0], &secret[0], 32);
    printf("  * secret (hex): %s\n", HexStr(sec).c_str());

    for (int nCompressed=0; nCompressed<2; nCompressed++)
    {
        bool fCompressed = nCompressed == 1;
        printf("  * %s:\n", fCompressed ? "compressed" : "uncompressed");
        CBTCUSecret bsecret;
        bsecret.SetSecret(secret, fCompressed);
        printf("    * secret (base58): %s\n", bsecret.ToString().c_str());
        CKey key;
        key.SetSecret(secret, fCompressed);
        std::vector<unsigned char> vchPubKey = key.GetPubKey();
        printf("    * pubkey (hex): %s\n", HexStr(vchPubKey).c_str());
        printf("    * address (base58): %s\n", CBTCUAddress(vchPubKey).ToString().c_str());
    }
}
#endif

BOOST_FIXTURE_TEST_SUITE(key_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(key_test1)
{
    const CBTCUAddress addr1 ("1J8Wz5gecX3Bh8VsQbe9KvQu8Ff3JHHTJZ");
    const CBTCUAddress addr2 ("1HCYAXkdTrzdCsuucxmGXpzdeujjcWc2ir");
    const CBTCUAddress addr1C("196eHcVw3frkWbvrWSYseqf3a3MgwYgNMg");
    const CBTCUAddress addr2C("1NwXuxAETu32zt6ZdXRRcVdFgvTLTf9Ud3");

    CBTCUSecret bsecret1, bsecret2, bsecret1C, bsecret2C, baddress1;
    BOOST_CHECK( bsecret1.SetString (strSecret1));
    BOOST_CHECK( bsecret2.SetString (strSecret2));
    BOOST_CHECK( bsecret1C.SetString(strSecret1C));
    BOOST_CHECK( bsecret2C.SetString(strSecret2C));
    BOOST_CHECK(!baddress1.SetString(strAddressBad));

    CKey key1  = bsecret1.GetKey();
    BOOST_CHECK(key1.IsCompressed() == false);
    CKey key2  = bsecret2.GetKey();
    BOOST_CHECK(key2.IsCompressed() == false);
    CKey key1C = bsecret1C.GetKey();
    BOOST_CHECK(key1C.IsCompressed() == true);
    CKey key2C = bsecret2C.GetKey();
    BOOST_CHECK(key2C.IsCompressed() == true);

    CPubKey pubkey1  = key1. GetPubKey();
    CPubKey pubkey2  = key2. GetPubKey();
    CPubKey pubkey1C = key1C.GetPubKey();
    CPubKey pubkey2C = key2C.GetPubKey();

    BOOST_CHECK(key1.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key1C.VerifyPubKey(pubkey1));
    BOOST_CHECK(key1C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey1C));
    BOOST_CHECK(key2.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey2));
    BOOST_CHECK(key2C.VerifyPubKey(pubkey2C));

    BOOST_CHECK(addr1.Get()  == CTxDestination(pubkey1.GetID()));
    BOOST_CHECK(addr2.Get()  == CTxDestination(pubkey2.GetID()));
    BOOST_CHECK(addr1C.Get() == CTxDestination(pubkey1C.GetID()));
    BOOST_CHECK(addr2C.Get() == CTxDestination(pubkey2C.GetID()));

    for (int n=0; n<16; n++)
    {
        std::string strMsg = strprintf("Very secret message %i: 11", n);
        uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

        // normal signatures

        std::vector<unsigned char> sign1, sign2, sign1C, sign2C;

        BOOST_CHECK(key1.Sign (hashMsg, sign1));
        BOOST_CHECK(key2.Sign (hashMsg, sign2));
        BOOST_CHECK(key1C.Sign(hashMsg, sign1C));
        BOOST_CHECK(key2C.Sign(hashMsg, sign2C));

        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1));
        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2));
        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1));
        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2));
        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1C));
        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2C));

        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1));
        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2));
        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1));
        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2));
        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1C));
        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2C));

        // compact signatures (with key recovery)

        std::vector<unsigned char> csign1, csign2, csign1C, csign2C;

        BOOST_CHECK(key1.SignCompact (hashMsg, csign1));
        BOOST_CHECK(key2.SignCompact (hashMsg, csign2));
        BOOST_CHECK(key1C.SignCompact(hashMsg, csign1C));
        BOOST_CHECK(key2C.SignCompact(hashMsg, csign2C));

        CPubKey rkey1, rkey2, rkey1C, rkey2C;

        BOOST_CHECK(rkey1.RecoverCompact (hashMsg, csign1));
        BOOST_CHECK(rkey2.RecoverCompact (hashMsg, csign2));
        BOOST_CHECK(rkey1C.RecoverCompact(hashMsg, csign1C));
        BOOST_CHECK(rkey2C.RecoverCompact(hashMsg, csign2C));

        BOOST_CHECK(rkey1  == pubkey1);
        BOOST_CHECK(rkey2  == pubkey2);
        BOOST_CHECK(rkey1C == pubkey1C);
        BOOST_CHECK(rkey2C == pubkey2C);
    }

    // test deterministic signing

    std::vector<unsigned char> detsig, detsigc;
    std::string strMsg = "Very deterministic message";
    uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());
    BOOST_CHECK(key1.Sign(hashMsg, detsig));
    BOOST_CHECK(key1C.Sign(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK_EQUAL(HexStr(detsig), "3044022017451585f3d7c36af7bd7378373ce6a6e9dbaf569bab50a8fa51f3e1773a4f200220510b98aae65a7fdc4cfc9255932d5c0e089c009669cdf85e0067b7a6ced5d65f");
    BOOST_CHECK(key2.Sign(hashMsg, detsig));
    BOOST_CHECK(key2C.Sign(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK_EQUAL(HexStr(detsig), "3045022100be8ac3495a6afcf0bccaac6e9104f033de6db8251ab4ec40ceab1955da1c725202202771519963deae292ab5b9e6d44cac9618fa6028802cc22954d6e86ab753f12d");
    BOOST_CHECK(key1.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key1C.SignCompact(hashMsg, detsigc));
    BOOST_CHECK_EQUAL(HexStr(detsig), "1b17451585f3d7c36af7bd7378373ce6a6e9dbaf569bab50a8fa51f3e1773a4f20510b98aae65a7fdc4cfc9255932d5c0e089c009669cdf85e0067b7a6ced5d65f");
    BOOST_CHECK_EQUAL(HexStr(detsigc), "1f17451585f3d7c36af7bd7378373ce6a6e9dbaf569bab50a8fa51f3e1773a4f20510b98aae65a7fdc4cfc9255932d5c0e089c009669cdf85e0067b7a6ced5d65f");
    BOOST_CHECK(key2.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key2C.SignCompact(hashMsg, detsigc));
    BOOST_CHECK_EQUAL(HexStr(detsig), "1cbe8ac3495a6afcf0bccaac6e9104f033de6db8251ab4ec40ceab1955da1c72522771519963deae292ab5b9e6d44cac9618fa6028802cc22954d6e86ab753f12d");
    BOOST_CHECK_EQUAL(HexStr(detsigc), "20be8ac3495a6afcf0bccaac6e9104f033de6db8251ab4ec40ceab1955da1c72522771519963deae292ab5b9e6d44cac9618fa6028802cc22954d6e86ab753f12d");
}

BOOST_AUTO_TEST_SUITE_END()
